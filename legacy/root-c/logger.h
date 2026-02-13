#ifndef LOGGER_H
#define LOGGER_H

void logger_init(void);
void log_event(const char *level, const char *message);
void log_login_attempt(const char *username, int success);
void log_session_event(int session_id, int vehicle_id, int slot_id, double energy_kwh, double final_cost);

#endif
