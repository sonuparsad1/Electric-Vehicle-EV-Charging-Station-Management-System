#include "charger.h"
#include "billing.h"
#include "common.h"
#include "logger.h"

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static ChargingSlot slots[MAX_SLOTS];
static VehicleStore *g_store = NULL;
static VehicleQueue *g_queue = NULL;

static pthread_t dispatcher_thread;
static pthread_mutex_t ch_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t ch_cond = PTHREAD_COND_INITIALIZER;

static int running = 0;
static int next_session_id = 1;
static double rate_per_kwh = 15.0;

static const Vehicle *find_vehicle(int id) {
    return vehicle_find_by_id(g_store, id);
}

static int find_idle_slot(void) {
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (slots[i].status == SLOT_IDLE) return i;
    }
    return -1;
}

static void ensure_sessions_header(void) {
    FILE *fp = fopen(SESSION_DB_FILE, "r");
    if (fp) {
        fclose(fp);
        return;
    }
    fp = fopen(SESSION_DB_FILE, "w");
    if (fp) {
        fprintf(fp, "session_id,vehicle_id,slot_id,energy_kwh,base_cost,tax,final_cost,start_time,end_time\n");
        fclose(fp);
    }
}

static void append_session(int session_id, int vehicle_id, int slot_id, Bill b, const char *start_ts, const char *end_ts) {
    FILE *fp = fopen(SESSION_DB_FILE, "a");
    if (!fp) return;
    fprintf(fp, "%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%s,%s\n",
            session_id, vehicle_id, slot_id, b.energy_kwh, b.base_cost, b.tax, b.final_cost, start_ts, end_ts);
    fclose(fp);
}

static void progress_bar(int pct) {
    int bars = pct / 5;
    printf("[");
    for (int i = 0; i < 20; i++) {
        if (i < bars) printf("=");
        else printf(" ");
    }
    printf("] %3d%%", pct);
}

static void *slot_worker(void *arg) {
    ChargingSlot *slot = (ChargingSlot *)arg;
    while (1) {
        pthread_mutex_lock(&ch_mutex);
        while (running && !slot->has_assignment) {
            pthread_cond_wait(&ch_cond, &ch_mutex);
        }
        if (!running && !slot->has_assignment) {
            pthread_mutex_unlock(&ch_mutex);
            break;
        }

        int vehicle_id = slot->current_vehicle_id;
        slot->has_assignment = 0;
        slot->status = SLOT_CHARGING;
        slot->delivered_kwh = 0.0;
        slot->current_cost = 0.0;
        slot->progress_percent = 0;
        pthread_mutex_unlock(&ch_mutex);

        const Vehicle *v = find_vehicle(vehicle_id);
        if (!v) {
            pthread_mutex_lock(&ch_mutex);
            slot->status = SLOT_IDLE;
            slot->current_vehicle_id = -1;
            pthread_cond_broadcast(&ch_cond);
            pthread_mutex_unlock(&ch_mutex);
            continue;
        }

        double target = v->battery_capacity * ((strcmp(v->charging_type, "fast") == 0) ? 0.85 : 0.65);
        double per_sec = (strcmp(v->charging_type, "fast") == 0) ? 3.5 : 2.0;

        time_t start = time(NULL);
        char start_ts[32], end_ts[32];
        format_time(start_ts, sizeof(start_ts), start);

        while (slot->delivered_kwh < target) {
            sleep(1);

            pthread_mutex_lock(&ch_mutex);
            slot->delivered_kwh += per_sec;
            if (slot->delivered_kwh > target) slot->delivered_kwh = target;

            Bill temp = billing_calculate(slot->delivered_kwh, rate_per_kwh);
            slot->current_cost = temp.final_cost;
            slot->progress_percent = (int)((slot->delivered_kwh / target) * 100.0);
            if (slot->progress_percent > 100) slot->progress_percent = 100;

            printf(COLOR_BLUE "\rSlot %d charging vehicle %d " COLOR_RESET, slot->slot_id, vehicle_id);
            progress_bar(slot->progress_percent);
            fflush(stdout);
            pthread_mutex_unlock(&ch_mutex);
        }
        printf("\n");

        time_t end = time(NULL);
        format_time(end_ts, sizeof(end_ts), end);

        pthread_mutex_lock(&ch_mutex);
        Bill final_bill = billing_calculate(slot->delivered_kwh, rate_per_kwh);
        int session_id = next_session_id++;
        pthread_mutex_unlock(&ch_mutex);

        append_session(session_id, vehicle_id, slot->slot_id, final_bill, start_ts, end_ts);
        billing_print_invoice(session_id, vehicle_id, slot->slot_id, start_ts, end_ts, final_bill);
        log_session_event(session_id, vehicle_id, slot->slot_id, final_bill.energy_kwh, final_bill.final_cost);

        pthread_mutex_lock(&ch_mutex);
        if (slot->status != SLOT_OFFLINE) slot->status = SLOT_IDLE;
        slot->current_vehicle_id = -1;
        slot->delivered_kwh = 0.0;
        slot->current_cost = 0.0;
        slot->progress_percent = 0;
        pthread_cond_broadcast(&ch_cond);
        pthread_mutex_unlock(&ch_mutex);
    }
    return NULL;
}

static void *dispatcher(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&ch_mutex);
        while (running) {
            int idx = find_idle_slot();
            if (idx >= 0 && queue_size(g_queue) > 0) break;
            pthread_cond_wait(&ch_cond, &ch_mutex);
        }
        if (!running) {
            pthread_mutex_unlock(&ch_mutex);
            break;
        }

        int vehicle_id = queue_dequeue(g_queue);
        int slot_idx = find_idle_slot();

        if (vehicle_id > 0 && slot_idx >= 0) {
            slots[slot_idx].status = SLOT_RESERVED;
            slots[slot_idx].current_vehicle_id = vehicle_id;
            slots[slot_idx].has_assignment = 1;
            pthread_cond_broadcast(&ch_cond);
        }
        pthread_mutex_unlock(&ch_mutex);
    }
    return NULL;
}

void charger_system_init(VehicleStore *store, VehicleQueue *queue) {
    g_store = store;
    g_queue = queue;
    running = 1;
    ensure_sessions_header();

    for (int i = 0; i < MAX_SLOTS; i++) {
        slots[i].slot_id = i + 1;
        slots[i].status = SLOT_IDLE;
        slots[i].current_vehicle_id = -1;
        slots[i].delivered_kwh = 0.0;
        slots[i].current_cost = 0.0;
        slots[i].progress_percent = 0;
        slots[i].has_assignment = 0;
        pthread_create(&slots[i].worker, NULL, slot_worker, &slots[i]);
    }
    pthread_create(&dispatcher_thread, NULL, dispatcher, NULL);
    log_event("SYSTEM", "Charger system initialized");
}

void charger_system_shutdown(void) {
    pthread_mutex_lock(&ch_mutex);
    running = 0;
    pthread_cond_broadcast(&ch_cond);
    pthread_mutex_unlock(&ch_mutex);

    pthread_join(dispatcher_thread, NULL);
    for (int i = 0; i < MAX_SLOTS; i++) pthread_join(slots[i].worker, NULL);
    log_event("SYSTEM", "Charger system shutdown complete");
}

int charger_request_vehicle(int vehicle_id) {
    if (!find_vehicle(vehicle_id)) return 0;
    int ok = queue_enqueue(g_queue, vehicle_id);
    pthread_mutex_lock(&ch_mutex);
    pthread_cond_broadcast(&ch_cond);
    pthread_mutex_unlock(&ch_mutex);
    return ok;
}

void charger_set_rate(double rate) {
    pthread_mutex_lock(&ch_mutex);
    if (rate > 0.0) rate_per_kwh = rate;
    pthread_mutex_unlock(&ch_mutex);
}

double charger_get_rate(void) {
    pthread_mutex_lock(&ch_mutex);
    double r = rate_per_kwh;
    pthread_mutex_unlock(&ch_mutex);
    return r;
}

int charger_active_sessions(void) {
    pthread_mutex_lock(&ch_mutex);
    int cnt = 0;
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (slots[i].status == SLOT_CHARGING || slots[i].status == SLOT_RESERVED) cnt++;
    }
    pthread_mutex_unlock(&ch_mutex);
    return cnt;
}

int charger_available_slots(void) {
    pthread_mutex_lock(&ch_mutex);
    int cnt = 0;
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (slots[i].status == SLOT_IDLE) cnt++;
    }
    pthread_mutex_unlock(&ch_mutex);
    return cnt;
}

void charger_get_slots_snapshot(ChargingSlot *out, int n) {
    if (!out || n < MAX_SLOTS) return;
    pthread_mutex_lock(&ch_mutex);
    for (int i = 0; i < MAX_SLOTS; i++) out[i] = slots[i];
    pthread_mutex_unlock(&ch_mutex);
}

int charger_toggle_slot_offline(int slot_id) {
    if (slot_id < 1 || slot_id > MAX_SLOTS) return 0;

    pthread_mutex_lock(&ch_mutex);
    ChargingSlot *s = &slots[slot_id - 1];
    int changed = 0;

    if (s->status == SLOT_IDLE) {
        s->status = SLOT_OFFLINE;
        changed = 1;
    } else if (s->status == SLOT_OFFLINE) {
        s->status = SLOT_IDLE;
        changed = 1;
    }

    pthread_cond_broadcast(&ch_cond);
    pthread_mutex_unlock(&ch_mutex);
    return changed;
}
