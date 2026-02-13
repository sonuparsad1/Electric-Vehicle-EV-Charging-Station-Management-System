#include "storage.h"

#include <stdio.h>
#include <string.h>

int storage_load_vehicles(const char *path, Vehicle vehicles[], int max_count) {
    FILE *fp = fopen(path, "r");
    int count = 0;

    if (!fp) {
        return 0;
    }

    while (count < max_count && fscanf(fp, "%63[^,],%31[^\n]\n", vehicles[count].owner, vehicles[count].vehicle_id) == 2) {
        count++;
    }

    fclose(fp);
    return count;
}

int storage_append_vehicle(const char *path, const Vehicle *vehicle) {
    FILE *fp = fopen(path, "a");
    if (!fp) {
        return 0;
    }

    fprintf(fp, "%s,%s\n", vehicle->owner, vehicle->vehicle_id);
    fclose(fp);
    return 1;
}

int storage_append_session(const char *path, const SessionRecord *session) {
    FILE *fp = fopen(path, "a");
    if (!fp) {
        return 0;
    }

    fprintf(fp, "%s,%s,%d,%.2f,%.2f\n",
            session->vehicle_id,
            session->owner,
            session->slot_id,
            session->energy_kwh,
            session->bill);
    fclose(fp);
    return 1;
}

int storage_load_revenue_and_sessions(const char *path, double *revenue, int *sessions) {
    FILE *fp = fopen(path, "r");
    SessionRecord rec;

    *revenue = 0.0;
    *sessions = 0;

    if (!fp) {
        return 0;
    }

    while (fscanf(fp, "%31[^,],%63[^,],%d,%lf,%lf\n",
                  rec.vehicle_id,
                  rec.owner,
                  &rec.slot_id,
                  &rec.energy_kwh,
                  &rec.bill) == 5) {
        *revenue += rec.bill;
        (*sessions)++;
    }

    fclose(fp);
    return 1;
}

int storage_generate_report(const char *path, const SystemState *state) {
    FILE *fp = fopen(path, "w");
    int i;

    if (!fp) {
        return 0;
    }

    fprintf(fp, "EV Charging Station Revenue Report\n");
    fprintf(fp, "==================================\n");
    fprintf(fp, "Total Registered Vehicles: %d\n", state->vehicle_count);
    fprintf(fp, "Total Sessions Completed: %d\n", state->total_sessions);
    fprintf(fp, "Active Charging Sessions: %d\n", logic_active_sessions(state));
    fprintf(fp, "Total Revenue: %.2f\n\n", state->total_revenue);

    fprintf(fp, "Slot Status\n");
    fprintf(fp, "-----------\n");
    for (i = 0; i < SLOT_COUNT; i++) {
        fprintf(fp, "Slot %d: %s", state->slots[i].slot_id,
                state->slots[i].status == SLOT_AVAILABLE ? "Available" : "Charging");
        if (state->slots[i].status == SLOT_CHARGING) {
            fprintf(fp, " (%s - %d%%)", state->slots[i].vehicle.vehicle_id, state->slots[i].progress);
        }
        fprintf(fp, "\n");
    }

    fclose(fp);
    return 1;
}
