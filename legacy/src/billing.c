#include "billing.h"
#include "common.h"

#include <stdio.h>

#define TAX_RATE 0.18

Bill billing_calculate(double energy_kwh, double rate_per_kwh) {
    Bill b;
    b.energy_kwh = energy_kwh;
    b.rate_per_kwh = rate_per_kwh;
    b.base_cost = b.energy_kwh * b.rate_per_kwh;
    b.tax = b.base_cost * TAX_RATE;
    b.final_cost = b.base_cost + b.tax;
    return b;
}

void billing_print_invoice(int session_id, int vehicle_id, int slot_id, const char *start_ts, const char *end_ts, Bill bill) {
    printf(COLOR_CYAN "\n+--------------------------------------------------+\n" COLOR_RESET);
    printf(COLOR_CYAN "|                 SESSION INVOICE                  |\n" COLOR_RESET);
    printf(COLOR_CYAN "+--------------------------------------------------+\n" COLOR_RESET);
    printf("Session ID      : %d\n", session_id);
    printf("Vehicle ID      : %d\n", vehicle_id);
    printf("Slot ID         : %d\n", slot_id);
    printf("Start Time      : %s\n", start_ts);
    printf("End Time        : %s\n", end_ts);
    printf("Energy          : %.2f kWh\n", bill.energy_kwh);
    printf("Rate            : %.2f / kWh\n", bill.rate_per_kwh);
    printf("Base Cost       : %.2f\n", bill.base_cost);
    printf("Tax (18%%)       : %.2f\n", bill.tax);
    printf(COLOR_GREEN "Final Cost       : %.2f\n" COLOR_RESET, bill.final_cost);
    printf(COLOR_CYAN "+--------------------------------------------------+\n\n" COLOR_RESET);
}
