#include "logic.h"

#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "billing.h"
#include "storage.h"

#define VEHICLES_FILE "vehicles.txt"
#define SESSIONS_FILE "sessions.txt"
#define REPORT_FILE "report.txt"

static void queue_init(VehicleQueue *q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

static int queue_push(VehicleQueue *q, const Vehicle *vehicle) {
    if (q->count >= MAX_QUEUE) {
        return 0;
    }

    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->items[q->rear] = *vehicle;
    q->count++;
    return 1;
}

static int queue_pop(VehicleQueue *q, Vehicle *vehicle) {
    if (q->count == 0) {
        return 0;
    }

    *vehicle = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->count--;
    return 1;
}

static Vehicle *find_vehicle(SystemState *state, const char *vehicle_id) {
    int i;
    for (i = 0; i < state->vehicle_count; i++) {
        if (strcmp(state->vehicles[i].vehicle_id, vehicle_id) == 0) {
            return &state->vehicles[i];
        }
    }
    return NULL;
}

static void add_event(LogicEvents *events, const char *msg) {
    if (events && events->count < 16) {
        strncpy(events->messages[events->count], msg, sizeof(events->messages[events->count]) - 1);
        events->messages[events->count][sizeof(events->messages[events->count]) - 1] = '\0';
        events->count++;
    }
}

static void assign_slots(SystemState *state, LogicEvents *events) {
    int i;
    for (i = 0; i < SLOT_COUNT; i++) {
        if (state->slots[i].status == SLOT_AVAILABLE && state->queue.count > 0) {
            char msg[180];
            queue_pop(&state->queue, &state->slots[i].vehicle);
            state->slots[i].status = SLOT_CHARGING;
            state->slots[i].progress = 0;
            state->slots[i].energy_kwh = 0.0;
            state->slots[i].target_energy_kwh = state->slots[i].vehicle.requested_energy_kwh;
            if (state->slots[i].target_energy_kwh <= 0.0) {
                state->slots[i].target_energy_kwh = 10.0;
            }

            snprintf(msg,
                     sizeof(msg),
                     "Vehicle %s assigned to Slot %d (Battery %d%% -> %d%%, %.2f kWh).",
                     state->slots[i].vehicle.vehicle_id,
                     state->slots[i].slot_id,
                     state->slots[i].vehicle.battery_start_pct,
                     state->slots[i].vehicle.battery_target_pct,
                     state->slots[i].target_energy_kwh);
            add_event(events, msg);
        }
    }
}

void logic_init(SystemState *state) {
    int i;
    memset(state, 0, sizeof(*state));
    queue_init(&state->queue);

    for (i = 0; i < SLOT_COUNT; i++) {
        state->slots[i].slot_id = i + 1;
        state->slots[i].status = SLOT_AVAILABLE;
    }

    state->vehicle_count = storage_load_vehicles(VEHICLES_FILE, state->vehicles, MAX_VEHICLES);
    storage_load_revenue_and_sessions(SESSIONS_FILE, &state->total_revenue, &state->total_sessions);

    srand((unsigned int)GetTickCount());
}

int logic_authenticate(const char *username, const char *password) {
    return strcmp(username, "admin") == 0 && strcmp(password, "1234") == 0;
}

int logic_register_vehicle(SystemState *state, const char *owner, const char *vehicle_id, char *err, int err_len) {
    Vehicle *existing;

    if (!owner || !vehicle_id || owner[0] == '\0' || vehicle_id[0] == '\0') {
        snprintf(err, err_len, "Owner name and Vehicle ID are required.");
        return 0;
    }

    if (state->vehicle_count >= MAX_VEHICLES) {
        snprintf(err, err_len, "Vehicle capacity reached.");
        return 0;
    }

    existing = find_vehicle(state, vehicle_id);
    if (existing) {
        snprintf(err, err_len, "Vehicle ID already exists.");
        return 0;
    }

    memset(&state->vehicles[state->vehicle_count], 0, sizeof(Vehicle));
    strncpy(state->vehicles[state->vehicle_count].owner,
            owner,
            sizeof(state->vehicles[state->vehicle_count].owner) - 1);
    strncpy(state->vehicles[state->vehicle_count].vehicle_id,
            vehicle_id,
            sizeof(state->vehicles[state->vehicle_count].vehicle_id) - 1);

    if (!storage_append_vehicle(VEHICLES_FILE, &state->vehicles[state->vehicle_count])) {
        snprintf(err, err_len, "Failed to write vehicle file.");
        return 0;
    }

    state->vehicle_count++;
    return 1;
}

int logic_enqueue_vehicle(SystemState *state,
                         const char *vehicle_id,
                         int battery_start_pct,
                         int battery_target_pct,
                         double battery_capacity_kwh,
                         char *err,
                         int err_len) {
    Vehicle *v = find_vehicle(state, vehicle_id);
    Vehicle queued;
    LogicEvents events = {0};

    if (!v) {
        snprintf(err, err_len, "Vehicle ID not found. Register first.");
        return 0;
    }

    if (battery_start_pct < 0 || battery_start_pct > 100 || battery_target_pct < 1 || battery_target_pct > 100) {
        snprintf(err, err_len, "Battery percentages must be in 0-100 range.");
        return 0;
    }

    if (battery_start_pct >= battery_target_pct) {
        snprintf(err, err_len, "Target battery %% must be greater than current battery %%.");
        return 0;
    }

    if (battery_capacity_kwh < 10.0 || battery_capacity_kwh > 200.0) {
        snprintf(err, err_len, "Battery capacity must be between 10 and 200 kWh.");
        return 0;
    }

    queued = *v;
    queued.battery_start_pct = battery_start_pct;
    queued.battery_target_pct = battery_target_pct;
    queued.battery_capacity_kwh = battery_capacity_kwh;
    queued.requested_energy_kwh = logic_calculate_energy_request(battery_start_pct, battery_target_pct, battery_capacity_kwh);

    if (!queue_push(&state->queue, &queued)) {
        snprintf(err, err_len, "Queue is full.");
        return 0;
    }

    assign_slots(state, &events);
    return 1;
}

void logic_tick(SystemState *state, LogicEvents *events) {
    int i;
    if (events) {
        events->count = 0;
    }

    for (i = 0; i < SLOT_COUNT; i++) {
        if (state->slots[i].status == SLOT_CHARGING) {
            SessionRecord rec;
            char msg[180];

            state->slots[i].progress += 10;
            if (state->slots[i].progress > 100) {
                state->slots[i].progress = 100;
            }

            state->slots[i].energy_kwh = state->slots[i].target_energy_kwh * (state->slots[i].progress / 100.0);

            if (state->slots[i].progress >= 100) {
                memset(&rec, 0, sizeof(rec));
                rec.slot_id = state->slots[i].slot_id;
                strncpy(rec.vehicle_id, state->slots[i].vehicle.vehicle_id, sizeof(rec.vehicle_id) - 1);
                strncpy(rec.owner, state->slots[i].vehicle.owner, sizeof(rec.owner) - 1);
                rec.energy_kwh = state->slots[i].target_energy_kwh;
                rec.bill = billing_calculate(rec.energy_kwh, RATE_PER_KWH);

                state->total_revenue += rec.bill;
                state->total_sessions++;
                storage_append_session(SESSIONS_FILE, &rec);

                snprintf(msg,
                         sizeof(msg),
                         "Complete %s: %.2f kWh, Bill %.2f",
                         rec.vehicle_id,
                         rec.energy_kwh,
                         rec.bill);
                add_event(events, msg);

                state->slots[i].status = SLOT_AVAILABLE;
                state->slots[i].progress = 0;
                state->slots[i].energy_kwh = 0.0;
                state->slots[i].target_energy_kwh = 0.0;
                memset(&state->slots[i].vehicle, 0, sizeof(Vehicle));
            }
        }
    }

    assign_slots(state, events);
}

int logic_generate_report(SystemState *state, char *err, int err_len) {
    if (!storage_generate_report(REPORT_FILE, state)) {
        snprintf(err, err_len, "Unable to generate report file.");
        return 0;
    }
    return 1;
}

int logic_active_sessions(const SystemState *state) {
    int i;
    int count = 0;

    for (i = 0; i < SLOT_COUNT; i++) {
        if (state->slots[i].status == SLOT_CHARGING) {
            count++;
        }
    }

    return count;
}

double logic_calculate_energy_request(int battery_start_pct, int battery_target_pct, double battery_capacity_kwh) {
    double delta = (double)(battery_target_pct - battery_start_pct) / 100.0;
    return battery_capacity_kwh * delta;
}

double logic_calculate_estimated_bill(double requested_energy_kwh) {
    return billing_calculate(requested_energy_kwh, RATE_PER_KWH);
}

int logic_calculate_estimated_minutes(double requested_energy_kwh) {
    const double station_power_kw = 22.0;
    double hours = requested_energy_kwh / station_power_kw;
    int minutes = (int)(hours * 60.0);
    if (minutes < 5) {
        minutes = 5;
    }
    return minutes;
}


double logic_calculate_average_bill(const SystemState *state) {
    if (!state || state->total_sessions <= 0) {
        return 0.0;
    }
    return state->total_revenue / state->total_sessions;
}
