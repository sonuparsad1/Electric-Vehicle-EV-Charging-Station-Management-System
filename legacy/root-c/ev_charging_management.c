#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#define MAX_VEHICLES 1000
#define MAX_QUEUE 1000
#define MAX_SLOTS 5
#define OWNER_LEN 80
#define VEHICLE_NO_LEN 32
#define PREF_LEN 8
#define LINE_BUF 512

#define VEHICLES_FILE "vehicles.csv"
#define SESSIONS_FILE "sessions.csv"

typedef struct {
    int id;
    char owner[OWNER_LEN];
    char vehicle_no[VEHICLE_NO_LEN];
    double battery_capacity;
    char preference[PREF_LEN]; // slow / fast
} Vehicle;

typedef enum {
    SLOT_IDLE = 0,
    SLOT_RESERVED,
    SLOT_CHARGING,
    SLOT_OFFLINE
} SlotState;

typedef struct {
    int id;
    SlotState state;
    int current_vehicle_id;
    int has_assignment;
    pthread_t thread;
} ChargingSlot;

typedef struct {
    int items[MAX_QUEUE];
    int front;
    int rear;
    int size;
} VehicleQueue;

static Vehicle vehicles[MAX_VEHICLES];
static int vehicle_count = 0;
static int next_vehicle_id = 1;
static int next_session_id = 1;

static ChargingSlot slots[MAX_SLOTS];
static VehicleQueue wait_queue = {.front = 0, .rear = 0, .size = 0};

static double rate_per_kwh = 15.0;
static int system_running = 1;

static pthread_mutex_t system_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t system_cond = PTHREAD_COND_INITIALIZER;
static pthread_t dispatcher_thread;

static void trim_newline(char *s) {
    size_t n = strlen(s);
    if (n > 0 && s[n - 1] == '\n') s[n - 1] = '\0';
}

static void read_line(const char *prompt, char *buffer, size_t len) {
    printf("%s", prompt);
    if (fgets(buffer, (int)len, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    trim_newline(buffer);
}

static int read_int(const char *prompt) {
    char buf[64];
    read_line(prompt, buf, sizeof(buf));
    return atoi(buf);
}

static double read_double(const char *prompt) {
    char buf[64];
    read_line(prompt, buf, sizeof(buf));
    return atof(buf);
}

static const char *slot_state_to_string(SlotState state) {
    switch (state) {
        case SLOT_IDLE: return "IDLE";
        case SLOT_RESERVED: return "RESERVED";
        case SLOT_CHARGING: return "CHARGING";
        case SLOT_OFFLINE: return "OFFLINE";
        default: return "UNKNOWN";
    }
}

static Vehicle *find_vehicle_by_id(int id) {
    for (int i = 0; i < vehicle_count; i++) {
        if (vehicles[i].id == id) return &vehicles[i];
    }
    return NULL;
}

static int is_vehicle_number_exists(const char *vehicle_no) {
    for (int i = 0; i < vehicle_count; i++) {
        if (strcmp(vehicles[i].vehicle_no, vehicle_no) == 0) return 1;
    }
    return 0;
}

static void ensure_sessions_file_header(void) {
    FILE *fp = fopen(SESSIONS_FILE, "r");
    if (fp != NULL) {
        fclose(fp);
        return;
    }
    fp = fopen(SESSIONS_FILE, "w");
    if (fp) {
        fprintf(fp, "session_id,vehicle_id,vehicle_no,slot_id,energy_kwh,start_time,end_time,cost\n");
        fclose(fp);
    }
}

static void ensure_vehicles_file_header(void) {
    FILE *fp = fopen(VEHICLES_FILE, "r");
    if (fp != NULL) {
        fclose(fp);
        return;
    }
    fp = fopen(VEHICLES_FILE, "w");
    if (fp) {
        fprintf(fp, "id,owner,vehicle_no,battery_capacity,preference\n");
        fclose(fp);
    }
}

static void load_vehicles(void) {
    ensure_vehicles_file_header();
    FILE *fp = fopen(VEHICLES_FILE, "r");
    if (!fp) return;

    char line[LINE_BUF];
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (vehicle_count >= MAX_VEHICLES) break;

        Vehicle v;
        char *token = strtok(line, ",");
        if (!token) continue;
        v.id = atoi(token);

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(v.owner, token, OWNER_LEN - 1);
        v.owner[OWNER_LEN - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(v.vehicle_no, token, VEHICLE_NO_LEN - 1);
        v.vehicle_no[VEHICLE_NO_LEN - 1] = '\0';

        token = strtok(NULL, ",");
        if (!token) continue;
        v.battery_capacity = atof(token);

        token = strtok(NULL, ",\n");
        if (!token) continue;
        strncpy(v.preference, token, PREF_LEN - 1);
        v.preference[PREF_LEN - 1] = '\0';

        vehicles[vehicle_count++] = v;
        if (v.id >= next_vehicle_id) next_vehicle_id = v.id + 1;
    }

    fclose(fp);
}

static void initialize_next_session_id(void) {
    ensure_sessions_file_header();
    FILE *fp = fopen(SESSIONS_FILE, "r");
    if (!fp) return;

    char line[LINE_BUF];
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return;
    }

    int max_id = 0;
    while (fgets(line, sizeof(line), fp)) {
        int id = atoi(line);
        if (id > max_id) max_id = id;
    }
    next_session_id = max_id + 1;
    fclose(fp);
}

static void save_vehicle(const Vehicle *v) {
    FILE *fp = fopen(VEHICLES_FILE, "a");
    if (!fp) {
        perror("Unable to write vehicle record");
        return;
    }
    fprintf(fp, "%d,%s,%s,%.2f,%s\n", v->id, v->owner, v->vehicle_no, v->battery_capacity, v->preference);
    fclose(fp);
}

static void log_session(int session_id, const Vehicle *v, int slot_id, double energy_kwh,
                        const char *start_time, const char *end_time, double cost) {
    FILE *fp = fopen(SESSIONS_FILE, "a");
    if (!fp) {
        perror("Unable to log session");
        return;
    }
    fprintf(fp, "%d,%d,%s,%d,%.2f,%s,%s,%.2f\n",
            session_id, v->id, v->vehicle_no, slot_id, energy_kwh, start_time, end_time, cost);
    fclose(fp);
}

static int enqueue_vehicle(int vehicle_id) {
    if (wait_queue.size >= MAX_QUEUE) return 0;
    wait_queue.items[wait_queue.rear] = vehicle_id;
    wait_queue.rear = (wait_queue.rear + 1) % MAX_QUEUE;
    wait_queue.size++;
    return 1;
}

static int dequeue_vehicle(void) {
    if (wait_queue.size == 0) return -1;
    int id = wait_queue.items[wait_queue.front];
    wait_queue.front = (wait_queue.front + 1) % MAX_QUEUE;
    wait_queue.size--;
    return id;
}

static int find_idle_slot_index(void) {
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (slots[i].state == SLOT_IDLE) return i;
    }
    return -1;
}

static void format_time(time_t t, char *buffer, size_t len) {
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    strftime(buffer, len, "%Y-%m-%d %H:%M:%S", &tm_info);
}

static void print_session_bill(int session_id, const Vehicle *v, int slot_id, double energy_kwh,
                               const char *start, const char *end, double cost) {
    printf("\n=========== CHARGING SESSION BILL ===========\n");
    printf("Session ID      : %d\n", session_id);
    printf("Vehicle ID      : %d\n", v->id);
    printf("Vehicle Number  : %s\n", v->vehicle_no);
    printf("Owner           : %s\n", v->owner);
    printf("Charging Slot   : %d\n", slot_id);
    printf("Energy Consumed : %.2f kWh\n", energy_kwh);
    printf("Rate            : Rs %.2f per kWh\n", rate_per_kwh);
    printf("Total Cost      : Rs %.2f\n", cost);
    printf("Start Time      : %s\n", start);
    printf("End Time        : %s\n", end);
    printf("============================================\n\n");
}

static void *slot_worker(void *arg) {
    ChargingSlot *slot = (ChargingSlot *)arg;

    while (1) {
        pthread_mutex_lock(&system_mutex);
        while (system_running && !slot->has_assignment) {
            pthread_cond_wait(&system_cond, &system_mutex);
        }

        if (!system_running) {
            pthread_mutex_unlock(&system_mutex);
            break;
        }

        int vehicle_id = slot->current_vehicle_id;
        slot->has_assignment = 0;
        slot->state = SLOT_CHARGING;
        pthread_mutex_unlock(&system_mutex);

        Vehicle *v = find_vehicle_by_id(vehicle_id);
        if (!v) {
            pthread_mutex_lock(&system_mutex);
            slot->state = SLOT_IDLE;
            slot->current_vehicle_id = -1;
            pthread_cond_broadcast(&system_cond);
            pthread_mutex_unlock(&system_mutex);
            continue;
        }

        double min_frac = 0.20;
        double max_frac = strcmp(v->preference, "fast") == 0 ? 0.90 : 0.70;
        double random_frac = min_frac + ((double)rand() / RAND_MAX) * (max_frac - min_frac);
        double energy_kwh = v->battery_capacity * random_frac;

        time_t start_time_t = time(NULL);
        char start_str[32], end_str[32];
        format_time(start_time_t, start_str, sizeof(start_str));

        int sleep_seconds = 2 + rand() % 5;
        sleep(sleep_seconds);

        time_t end_time_t = time(NULL);
        format_time(end_time_t, end_str, sizeof(end_str));

        double cost = energy_kwh * rate_per_kwh;

        pthread_mutex_lock(&system_mutex);
        int session_id = next_session_id++;
        pthread_mutex_unlock(&system_mutex);

        log_session(session_id, v, slot->id, energy_kwh, start_str, end_str, cost);
        print_session_bill(session_id, v, slot->id, energy_kwh, start_str, end_str, cost);

        pthread_mutex_lock(&system_mutex);
        slot->state = SLOT_IDLE;
        slot->current_vehicle_id = -1;
        pthread_cond_broadcast(&system_cond);
        pthread_mutex_unlock(&system_mutex);
    }

    return NULL;
}

static void *dispatcher_worker(void *arg) {
    (void)arg;
    while (1) {
        pthread_mutex_lock(&system_mutex);
        while (system_running) {
            int slot_index = find_idle_slot_index();
            if (wait_queue.size > 0 && slot_index != -1) break;
            pthread_cond_wait(&system_cond, &system_mutex);
        }

        if (!system_running) {
            pthread_mutex_unlock(&system_mutex);
            break;
        }

        int vehicle_id = dequeue_vehicle();
        int slot_index = find_idle_slot_index();
        if (vehicle_id == -1 || slot_index == -1) {
            pthread_mutex_unlock(&system_mutex);
            continue;
        }

        slots[slot_index].state = SLOT_RESERVED;
        slots[slot_index].current_vehicle_id = vehicle_id;
        slots[slot_index].has_assignment = 1;

        printf("Vehicle ID %d reserved for Slot %d.\n", vehicle_id, slots[slot_index].id);

        pthread_cond_broadcast(&system_cond);
        pthread_mutex_unlock(&system_mutex);
    }
    return NULL;
}

static void register_vehicle(void) {
    if (vehicle_count >= MAX_VEHICLES) {
        printf("Vehicle storage full.\n");
        return;
    }

    Vehicle v;
    v.id = next_vehicle_id++;

    read_line("Enter owner name: ", v.owner, sizeof(v.owner));
    if (strlen(v.owner) == 0) {
        printf("Invalid owner name.\n");
        return;
    }

    read_line("Enter vehicle number: ", v.vehicle_no, sizeof(v.vehicle_no));
    if (strlen(v.vehicle_no) == 0 || is_vehicle_number_exists(v.vehicle_no)) {
        printf("Invalid or duplicate vehicle number.\n");
        return;
    }

    v.battery_capacity = read_double("Enter battery capacity (kWh): ");
    if (v.battery_capacity <= 0) {
        printf("Battery capacity must be positive.\n");
        return;
    }

    read_line("Charging preference (slow/fast): ", v.preference, sizeof(v.preference));
    if (strcmp(v.preference, "slow") != 0 && strcmp(v.preference, "fast") != 0) {
        printf("Preference must be 'slow' or 'fast'.\n");
        return;
    }

    vehicles[vehicle_count++] = v;
    save_vehicle(&v);

    printf("Vehicle registered successfully. Vehicle ID: %d\n", v.id);
}

static void list_registered_vehicles(void) {
    printf("\n------ Registered Vehicles ------\n");
    if (vehicle_count == 0) {
        printf("No vehicles registered.\n");
        return;
    }

    for (int i = 0; i < vehicle_count; i++) {
        printf("ID:%d | Owner:%s | Number:%s | Battery:%.2f kWh | Pref:%s\n",
               vehicles[i].id, vehicles[i].owner, vehicles[i].vehicle_no,
               vehicles[i].battery_capacity, vehicles[i].preference);
    }
}

static void request_charging(void) {
    int vehicle_id = read_int("Enter vehicle ID for charging request: ");

    Vehicle *v = find_vehicle_by_id(vehicle_id);
    if (!v) {
        printf("Vehicle ID not found.\n");
        return;
    }

    pthread_mutex_lock(&system_mutex);
    if (!enqueue_vehicle(vehicle_id)) {
        pthread_mutex_unlock(&system_mutex);
        printf("Queue is full. Try again later.\n");
        return;
    }
    printf("Vehicle %s (ID %d) added to charging queue.\n", v->vehicle_no, v->id);
    pthread_cond_broadcast(&system_cond);
    pthread_mutex_unlock(&system_mutex);
}

static void view_queue_status(void) {
    pthread_mutex_lock(&system_mutex);
    printf("\n------ Queue Status ------\n");
    printf("Vehicles waiting: %d\n", wait_queue.size);
    if (wait_queue.size == 0) {
        printf("Queue is empty.\n");
    } else {
        int idx = wait_queue.front;
        for (int i = 0; i < wait_queue.size; i++) {
            int vehicle_id = wait_queue.items[idx];
            Vehicle *v = find_vehicle_by_id(vehicle_id);
            printf("%d. Vehicle ID %d (%s)\n", i + 1, vehicle_id, v ? v->vehicle_no : "Unknown");
            idx = (idx + 1) % MAX_QUEUE;
        }
    }
    pthread_mutex_unlock(&system_mutex);
}

static void view_slot_status(void) {
    pthread_mutex_lock(&system_mutex);
    printf("\n------ Slot Status ------\n");
    for (int i = 0; i < MAX_SLOTS; i++) {
        printf("Slot %d: %-9s", slots[i].id, slot_state_to_string(slots[i].state));
        if (slots[i].current_vehicle_id != -1) {
            printf(" | Vehicle ID: %d", slots[i].current_vehicle_id);
        }
        printf("\n");
    }
    pthread_mutex_unlock(&system_mutex);
}

static void toggle_slot_offline(void) {
    int slot_id = read_int("Enter slot ID (1-5): ");
    if (slot_id < 1 || slot_id > MAX_SLOTS) {
        printf("Invalid slot ID.\n");
        return;
    }

    pthread_mutex_lock(&system_mutex);
    ChargingSlot *slot = &slots[slot_id - 1];
    if (slot->state == SLOT_OFFLINE) {
        slot->state = SLOT_IDLE;
        printf("Slot %d is now ONLINE (IDLE).\n", slot->id);
    } else if (slot->state == SLOT_IDLE) {
        slot->state = SLOT_OFFLINE;
        printf("Slot %d is now OFFLINE.\n", slot->id);
    } else {
        printf("Slot %d is busy (%s). Cannot toggle now.\n", slot->id, slot_state_to_string(slot->state));
    }
    pthread_cond_broadcast(&system_cond);
    pthread_mutex_unlock(&system_mutex);
}

static void view_all_sessions(void) {
    FILE *fp = fopen(SESSIONS_FILE, "r");
    if (!fp) {
        printf("No sessions file found.\n");
        return;
    }

    printf("\n------ Charging Session History ------\n");
    char line[LINE_BUF];
    int line_no = 0;
    while (fgets(line, sizeof(line), fp)) {
        trim_newline(line);
        if (line_no++ == 0) {
            printf("%s\n", line);
            continue;
        }
        printf("%s\n", line);
    }
    fclose(fp);
}

static void revenue_report_today(void) {
    FILE *fp = fopen(SESSIONS_FILE, "r");
    if (!fp) {
        printf("No sessions available.\n");
        return;
    }

    time_t now = time(NULL);
    struct tm today_tm;
    localtime_r(&now, &today_tm);
    char today_prefix[11];
    strftime(today_prefix, sizeof(today_prefix), "%Y-%m-%d", &today_tm);

    char line[LINE_BUF];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        printf("No sessions available.\n");
        return;
    }

    double total_revenue = 0.0;
    int total_sessions = 0;

    while (fgets(line, sizeof(line), fp)) {
        char copy[LINE_BUF];
        strncpy(copy, line, sizeof(copy) - 1);
        copy[sizeof(copy) - 1] = '\0';

        char *token = strtok(copy, ","); // session_id
        if (!token) continue;
        strtok(NULL, ","); // vehicle_id
        strtok(NULL, ","); // vehicle_no
        strtok(NULL, ","); // slot_id
        strtok(NULL, ","); // energy_kwh
        char *start_time = strtok(NULL, ",");
        strtok(NULL, ","); // end_time
        char *cost_str = strtok(NULL, ",\n");

        if (!start_time || !cost_str) continue;

        if (strncmp(start_time, today_prefix, 10) == 0) {
            total_revenue += atof(cost_str);
            total_sessions++;
        }
    }

    fclose(fp);

    double avg_cost = (total_sessions > 0) ? total_revenue / total_sessions : 0.0;
    printf("\n------ Revenue Report (Today: %s) ------\n", today_prefix);
    printf("Total Sessions : %d\n", total_sessions);
    printf("Total Revenue  : Rs %.2f\n", total_revenue);
    printf("Average Cost   : Rs %.2f\n", avg_cost);
}

static void set_rate(void) {
    double new_rate = read_double("Enter new rate per kWh: ");
    if (new_rate <= 0) {
        printf("Rate must be positive.\n");
        return;
    }

    pthread_mutex_lock(&system_mutex);
    rate_per_kwh = new_rate;
    pthread_mutex_unlock(&system_mutex);

    printf("Rate updated to Rs %.2f per kWh\n", new_rate);
}

static void initialize_slots_and_threads(void) {
    for (int i = 0; i < MAX_SLOTS; i++) {
        slots[i].id = i + 1;
        slots[i].state = SLOT_IDLE;
        slots[i].current_vehicle_id = -1;
        slots[i].has_assignment = 0;
        pthread_create(&slots[i].thread, NULL, slot_worker, &slots[i]);
    }
    pthread_create(&dispatcher_thread, NULL, dispatcher_worker, NULL);
}

static void shutdown_system(void) {
    pthread_mutex_lock(&system_mutex);
    system_running = 0;
    pthread_cond_broadcast(&system_cond);
    pthread_mutex_unlock(&system_mutex);

    pthread_join(dispatcher_thread, NULL);
    for (int i = 0; i < MAX_SLOTS; i++) {
        pthread_join(slots[i].thread, NULL);
    }
}

static void print_menu(void) {
    printf("\n======== EV Charging Station Management ========\n");
    printf("1. Register New Vehicle\n");
    printf("2. Request Charging (Add to Queue)\n");
    printf("3. View Registered Vehicles\n");
    printf("4. View Queue Status\n");
    printf("5. View Slot Status\n");
    printf("6. Toggle Slot Offline/Online (Admin)\n");
    printf("7. View Charging Session History (Admin)\n");
    printf("8. Revenue Report for Today (Admin)\n");
    printf("9. Set Charging Rate (Admin)\n");
    printf("10. Exit\n");
    printf("Current rate: Rs %.2f / kWh\n", rate_per_kwh);
}

int main(void) {
    srand((unsigned int)time(NULL));
    load_vehicles();
    initialize_next_session_id();
    initialize_slots_and_threads();

    int choice;
    while (1) {
        print_menu();
        choice = read_int("Enter choice: ");

        switch (choice) {
            case 1:
                register_vehicle();
                break;
            case 2:
                request_charging();
                break;
            case 3:
                list_registered_vehicles();
                break;
            case 4:
                view_queue_status();
                break;
            case 5:
                view_slot_status();
                break;
            case 6:
                toggle_slot_offline();
                break;
            case 7:
                view_all_sessions();
                break;
            case 8:
                revenue_report_today();
                break;
            case 9:
                set_rate();
                break;
            case 10:
                shutdown_system();
                printf("System shut down. Goodbye!\n");
                return 0;
            default:
                printf("Invalid choice. Please try again.\n");
                break;
        }
    }

    return 0;
}
