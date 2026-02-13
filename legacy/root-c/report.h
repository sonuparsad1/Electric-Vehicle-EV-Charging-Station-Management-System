#ifndef REPORT_H
#define REPORT_H

typedef struct {
    int total_sessions;
    double total_revenue;
    double avg_session_cost;
} RevenueReport;

RevenueReport report_daily_revenue(void);
void report_export_to_file(RevenueReport report);

#endif
