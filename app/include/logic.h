#ifndef LOGIC_H
#define LOGIC_H

#define MAX_VEHICLES 500
#define MAX_QUEUE 500
#define SLOT_COUNT 3
#define RATE_PER_KWH 15.0

typedef struct {
    char owner[64];
    char vehicle_id[32];
    int battery_start_pct;
    int battery_target_pct;
    double battery_capacity_kwh;
    double requested_energy_kwh;
} Vehicle;

typedef struct {
    Vehicle items[MAX_QUEUE];
    int front;
    int rear;
    int count;
} VehicleQueue;

typedef enum {
    SLOT_AVAILABLE = 0,
    SLOT_CHARGING = 1
} SlotStatus;

typedef struct {
    int slot_id;
    SlotStatus status;
    Vehicle vehicle;
    int progress;
    double energy_kwh;
    double target_energy_kwh;
} ChargingSlot;

typedef struct {
    char vehicle_id[32];
    char owner[64];
    int slot_id;
    double energy_kwh;
    double bill;
} SessionRecord;

typedef struct {
    Vehicle vehicles[MAX_VEHICLES];
    int vehicle_count;
    VehicleQueue queue;
    ChargingSlot slots[SLOT_COUNT];
    double total_revenue;
    int total_sessions;
} SystemState;

typedef struct {
    char messages[16][180];
    int count;
} LogicEvents;

void logic_init(SystemState *state);
int logic_authenticate(const char *username, const char *password);
int logic_register_vehicle(SystemState *state, const char *owner, const char *vehicle_id, char *err, int err_len);
int logic_enqueue_vehicle(SystemState *state,
                         const char *vehicle_id,
                         int battery_start_pct,
                         int battery_target_pct,
                         double battery_capacity_kwh,
                         char *err,
                         int err_len);
void logic_tick(SystemState *state, LogicEvents *events);
int logic_generate_report(SystemState *state, char *err, int err_len);
int logic_active_sessions(const SystemState *state);

double logic_calculate_energy_request(int battery_start_pct, int battery_target_pct, double battery_capacity_kwh);
double logic_calculate_estimated_bill(double requested_energy_kwh);
int logic_calculate_estimated_minutes(double requested_energy_kwh);

#endif
