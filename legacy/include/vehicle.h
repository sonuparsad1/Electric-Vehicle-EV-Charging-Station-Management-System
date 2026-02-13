#ifndef VEHICLE_H
#define VEHICLE_H

#include "common.h"

typedef struct {
    int id;
    char owner[OWNER_NAME_LEN];
    char vehicle_no[VEHICLE_NO_LEN];
    double battery_capacity;
    char charging_type[CHARGE_TYPE_LEN];
} Vehicle;

typedef struct {
    Vehicle vehicles[MAX_VEHICLES];
    int count;
    int next_id;
} VehicleStore;

void vehicle_store_init(VehicleStore *store);
int vehicle_register(VehicleStore *store, const char *owner, const char *vehicle_no, double battery_capacity, const char *type);
const Vehicle *vehicle_find_by_id(const VehicleStore *store, int id);
void vehicle_list_all(const VehicleStore *store);
int vehicle_total(const VehicleStore *store);

#endif
