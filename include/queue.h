#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include "common.h"

typedef struct {
    int data[MAX_QUEUE];
    int front;
    int rear;
    int size;
    pthread_mutex_t mutex;
} VehicleQueue;

void queue_init(VehicleQueue *q);
int queue_enqueue(VehicleQueue *q, int vehicle_id);
int queue_dequeue(VehicleQueue *q);
int queue_size(VehicleQueue *q);
void queue_visual_print(VehicleQueue *q);

#endif
