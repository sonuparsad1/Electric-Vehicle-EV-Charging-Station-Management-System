#include "billing.h"

double billing_calculate(double energy_kwh, double rate_per_kwh) {
    return energy_kwh * rate_per_kwh;
}
