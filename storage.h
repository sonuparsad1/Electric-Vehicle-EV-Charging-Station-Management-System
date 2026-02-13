#ifndef STORAGE_H
#define STORAGE_H

#include "logic.h"

int storage_load_vehicles(const char *path, Vehicle vehicles[], int max_count);
int storage_append_vehicle(const char *path, const Vehicle *vehicle);
int storage_append_session(const char *path, const SessionRecord *session);
int storage_load_revenue_and_sessions(const char *path, double *revenue, int *sessions);
int storage_generate_report(const char *path, const SystemState *state);

#endif
