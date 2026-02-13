#ifndef CHARGER_H
#define CHARGER_H

#include <pthread.h>
#include "vehicle.h"
#include "queue.h"

typedef enum {
    SLOT_IDLE = 0,
    SLOT_RESERVED,
    SLOT_CHARGING,
    SLOT_OFFLINE
} SlotStatus;

typedef struct {
    int slot_id;
    SlotStatus status;
    int current_vehicle_id;
    double delivered_kwh;
    double current_cost;
    int progress_percent;
    int has_assignment;
    pthread_t worker;
} ChargingSlot;

void charger_system_init(VehicleStore *store, VehicleQueue *queue);
void charger_system_shutdown(void);
int charger_request_vehicle(int vehicle_id);
void charger_set_rate(double rate);
double charger_get_rate(void);
int charger_active_sessions(void);
int charger_available_slots(void);
void charger_get_slots_snapshot(ChargingSlot *out, int n);
int charger_toggle_slot_offline(int slot_id);

#endif
