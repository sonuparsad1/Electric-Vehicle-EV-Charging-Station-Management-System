#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <time.h>

#define MAX_VEHICLES 2000
#define MAX_QUEUE 2000
#define MAX_SLOTS 4

#define OWNER_NAME_LEN 64
#define VEHICLE_NO_LEN 24
#define CHARGE_TYPE_LEN 16

#define VEHICLE_DB_FILE "vehicles.csv"
#define SESSION_DB_FILE "sessions.csv"
#define LOG_FILE "system.log"
#define AUTH_FILE "admin.auth"
#define REPORT_EXPORT_FILE "daily_report.txt"

#define COLOR_RESET "\x1b[0m"
#define COLOR_BOLD "\x1b[1m"
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_BLUE "\x1b[34m"
#define COLOR_CYAN "\x1b[36m"
#define COLOR_WHITE "\x1b[37m"
#define BG_DARK "\x1b[48;5;236m"


static inline void sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static inline void clear_screen(void) {
    printf("\x1b[2J\x1b[H");
}

static inline void format_time(char *out, size_t n, time_t t) {
    struct tm tm_info;
    localtime_r(&t, &tm_info);
    strftime(out, n, "%Y-%m-%d %H:%M:%S", &tm_info);
}

#endif
