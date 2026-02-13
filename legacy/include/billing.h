#ifndef BILLING_H
#define BILLING_H

typedef struct {
    double energy_kwh;
    double rate_per_kwh;
    double base_cost;
    double tax;
    double final_cost;
} Bill;

Bill billing_calculate(double energy_kwh, double rate_per_kwh);
void billing_print_invoice(int session_id, int vehicle_id, int slot_id, const char *start_ts, const char *end_ts, Bill bill);

#endif
