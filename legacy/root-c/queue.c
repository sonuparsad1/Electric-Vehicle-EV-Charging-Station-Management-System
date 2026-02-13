#include "queue.h"
#include <stdio.h>

void queue_init(VehicleQueue *q) {
    q->front = q->rear = q->size = 0;
    pthread_mutex_init(&q->mutex, NULL);
}

int queue_enqueue(VehicleQueue *q, int vehicle_id) {
    pthread_mutex_lock(&q->mutex);
    if (q->size >= MAX_QUEUE) {
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }
    q->data[q->rear] = vehicle_id;
    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->size++;
    pthread_mutex_unlock(&q->mutex);
    return 1;
}

int queue_dequeue(VehicleQueue *q) {
    pthread_mutex_lock(&q->mutex);
    if (q->size == 0) {
        pthread_mutex_unlock(&q->mutex);
        return -1;
    }
    int value = q->data[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->size--;
    pthread_mutex_unlock(&q->mutex);
    return value;
}

int queue_size(VehicleQueue *q) {
    pthread_mutex_lock(&q->mutex);
    int n = q->size;
    pthread_mutex_unlock(&q->mutex);
    return n;
}

void queue_visual_print(VehicleQueue *q) {
    pthread_mutex_lock(&q->mutex);
    printf(COLOR_BOLD "\nQueue (FIFO): " COLOR_RESET);
    if (q->size == 0) {
        printf("[ empty ]\n");
    } else {
        int idx = q->front;
        for (int i = 0; i < q->size; i++) {
            printf("[%d]", q->data[idx]);
            if (i < q->size - 1) printf(" -> ");
            idx = (idx + 1) % MAX_QUEUE;
        }
        printf("\n");
    }
    pthread_mutex_unlock(&q->mutex);
}
