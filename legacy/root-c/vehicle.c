#include "vehicle.h"
#include "logger.h"

#include <string.h>
#include <stdlib.h>

static void ensure_vehicle_file_header(void) {
    FILE *fp = fopen(VEHICLE_DB_FILE, "r");
    if (fp) {
        fclose(fp);
        return;
    }
    fp = fopen(VEHICLE_DB_FILE, "w");
    if (fp) {
        fprintf(fp, "id,owner,vehicle_no,battery_capacity,charging_type\n");
        fclose(fp);
    }
}

void vehicle_store_init(VehicleStore *store) {
    memset(store, 0, sizeof(*store));
    store->next_id = 1;
    ensure_vehicle_file_header();

    FILE *fp = fopen(VEHICLE_DB_FILE, "r");
    if (!fp) return;

    char line[512];
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp) && store->count < MAX_VEHICLES) {
        Vehicle v;
        char *t = strtok(line, ",");
        if (!t) continue;
        v.id = atoi(t);

        t = strtok(NULL, ","); if (!t) continue;
        strncpy(v.owner, t, sizeof(v.owner) - 1); v.owner[sizeof(v.owner) - 1] = '\0';

        t = strtok(NULL, ","); if (!t) continue;
        strncpy(v.vehicle_no, t, sizeof(v.vehicle_no) - 1); v.vehicle_no[sizeof(v.vehicle_no) - 1] = '\0';

        t = strtok(NULL, ","); if (!t) continue;
        v.battery_capacity = atof(t);

        t = strtok(NULL, ",\n"); if (!t) continue;
        strncpy(v.charging_type, t, sizeof(v.charging_type) - 1); v.charging_type[sizeof(v.charging_type) - 1] = '\0';

        store->vehicles[store->count++] = v;
        if (v.id >= store->next_id) store->next_id = v.id + 1;
    }

    fclose(fp);
}

static int vehicle_number_exists(const VehicleStore *store, const char *vehicle_no) {
    for (int i = 0; i < store->count; i++) {
        if (strcmp(store->vehicles[i].vehicle_no, vehicle_no) == 0) return 1;
    }
    return 0;
}

int vehicle_register(VehicleStore *store, const char *owner, const char *vehicle_no, double battery_capacity, const char *type) {
    if (!owner || !vehicle_no || !type) return -1;
    if (store->count >= MAX_VEHICLES || battery_capacity <= 0.0) return -1;
    if (vehicle_number_exists(store, vehicle_no)) return -2;

    Vehicle v;
    v.id = store->next_id++;
    strncpy(v.owner, owner, sizeof(v.owner) - 1); v.owner[sizeof(v.owner) - 1] = '\0';
    strncpy(v.vehicle_no, vehicle_no, sizeof(v.vehicle_no) - 1); v.vehicle_no[sizeof(v.vehicle_no) - 1] = '\0';
    v.battery_capacity = battery_capacity;
    strncpy(v.charging_type, type, sizeof(v.charging_type) - 1); v.charging_type[sizeof(v.charging_type) - 1] = '\0';

    store->vehicles[store->count++] = v;

    FILE *fp = fopen(VEHICLE_DB_FILE, "a");
    if (fp) {
        fprintf(fp, "%d,%s,%s,%.2f,%s\n", v.id, v.owner, v.vehicle_no, v.battery_capacity, v.charging_type);
        fclose(fp);
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Vehicle registered id=%d no=%s", v.id, v.vehicle_no);
    log_event("VEHICLE", msg);

    return v.id;
}

const Vehicle *vehicle_find_by_id(const VehicleStore *store, int id) {
    for (int i = 0; i < store->count; i++) {
        if (store->vehicles[i].id == id) return &store->vehicles[i];
    }
    return NULL;
}

void vehicle_list_all(const VehicleStore *store) {
    printf(COLOR_BOLD "\nRegistered Vehicles\n" COLOR_RESET);
    printf("--------------------------------------------------------------\n");
    printf("%-5s %-20s %-12s %-10s %-8s\n", "ID", "Owner", "VehicleNo", "Battery", "Type");
    printf("--------------------------------------------------------------\n");
    for (int i = 0; i < store->count; i++) {
        const Vehicle *v = &store->vehicles[i];
        printf("%-5d %-20s %-12s %-10.2f %-8s\n", v->id, v->owner, v->vehicle_no, v->battery_capacity, v->charging_type);
    }
    if (store->count == 0) printf("No vehicles registered.\n");
}

int vehicle_total(const VehicleStore *store) {
    return store->count;
}
