#include "logger.h"
#include "common.h"

#include <pthread.h>
#include <string.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void logger_init(void) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) fclose(fp);
}

void log_event(const char *level, const char *message) {
    pthread_mutex_lock(&log_mutex);
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        char ts[32];
        format_time(ts, sizeof(ts), time(NULL));
        fprintf(fp, "[%s] [%s] %s\n", ts, level, message);
        fclose(fp);
    }
    pthread_mutex_unlock(&log_mutex);
}

void log_login_attempt(const char *username, int success) {
    char msg[256];
    snprintf(msg, sizeof(msg), "Login attempt user=%s status=%s", username, success ? "SUCCESS" : "FAILED");
    log_event("AUTH", msg);
}

void log_session_event(int session_id, int vehicle_id, int slot_id, double energy_kwh, double final_cost) {
    char msg[256];
    snprintf(msg, sizeof(msg),
             "Session completed session_id=%d vehicle_id=%d slot=%d energy=%.2f cost=%.2f",
             session_id, vehicle_id, slot_id, energy_kwh, final_cost);
    log_event("SESSION", msg);
}
