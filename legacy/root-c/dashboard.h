#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "vehicle.h"
#include "queue.h"

void dashboard_show_once(VehicleStore *store, VehicleQueue *queue);
void dashboard_live_view(VehicleStore *store, VehicleQueue *queue, int seconds);

#endif
