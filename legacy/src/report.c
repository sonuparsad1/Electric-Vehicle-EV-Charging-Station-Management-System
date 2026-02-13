#include "report.h"
#include "common.h"
#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void ensure_session_header(void) {
    FILE *fp = fopen(SESSION_DB_FILE, "r");
    if (fp) {
        fclose(fp);
        return;
    }
    fp = fopen(SESSION_DB_FILE, "w");
    if (fp) {
        fprintf(fp, "session_id,vehicle_id,slot_id,energy_kwh,base_cost,tax,final_cost,start_time,end_time\n");
        fclose(fp);
    }
}

RevenueReport report_daily_revenue(void) {
    ensure_session_header();
    RevenueReport r = {0, 0.0, 0.0};

    FILE *fp = fopen(SESSION_DB_FILE, "r");
    if (!fp) return r;

    char today[11];
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(today, sizeof(today), "%Y-%m-%d", &tm_info);

    char line[512];
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        char copy[512];
        strncpy(copy, line, sizeof(copy) - 1);
        copy[sizeof(copy) - 1] = '\0';

        strtok(copy, ",");
        strtok(NULL, ",");
        strtok(NULL, ",");
        strtok(NULL, ",");
        strtok(NULL, ",");
        strtok(NULL, ",");
        char *final_cost = strtok(NULL, ",");
        char *start_time = strtok(NULL, ",");

        if (!final_cost || !start_time) continue;
        if (strncmp(start_time, today, 10) == 0) {
            r.total_sessions++;
            r.total_revenue += atof(final_cost);
        }
    }

    fclose(fp);

    if (r.total_sessions > 0)
        r.avg_session_cost = r.total_revenue / r.total_sessions;

    return r;
}

void report_export_to_file(RevenueReport report) {
    FILE *fp = fopen(REPORT_EXPORT_FILE, "w");
    if (!fp) return;

    char ts[32];
    format_time(ts, sizeof(ts), time(NULL));
    fprintf(fp, "EV Charging Daily Revenue Report\n");
    fprintf(fp, "Generated At: %s\n", ts);
    fprintf(fp, "Total Sessions: %d\n", report.total_sessions);
    fprintf(fp, "Total Revenue: %.2f\n", report.total_revenue);
    fprintf(fp, "Average Session Cost: %.2f\n", report.avg_session_cost);
    fclose(fp);

    log_event("REPORT", "Daily report exported");
}
