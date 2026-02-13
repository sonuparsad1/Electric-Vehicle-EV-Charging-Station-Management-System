#include "auth.h"
#include "vehicle.h"
#include "queue.h"
#include "charger.h"
#include "dashboard.h"
#include "report.h"
#include "logger.h"
#include "common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void read_line(const char *prompt, char *buf, size_t n) {
    printf("%s", prompt);
    if (!fgets(buf, (int)n, stdin)) {
        buf[0] = '\0';
        return;
    }
    buf[strcspn(buf, "\r\n")] = '\0';
}

static int read_int(const char *prompt) {
    char b[64];
    read_line(prompt, b, sizeof(b));
    return atoi(b);
}

static double read_double(const char *prompt) {
    char b[64];
    read_line(prompt, b, sizeof(b));
    return atof(b);
}

static void show_menu(void) {
    printf(COLOR_BOLD COLOR_BLUE "\n================ MAIN MENU ================\n" COLOR_RESET);
    printf("1. Dashboard\n");
    printf("2. Register Vehicle\n");
    printf("3. View All Vehicles\n");
    printf("4. Request Charging\n");
    printf("5. Toggle Slot Offline/Online\n");
    printf("6. Set Charging Rate\n");
    printf("7. Revenue Analytics\n");
    printf("8. Export Daily Report\n");
    printf("9. Exit\n");
}

int main(void) {
    logger_init();

    if (!auth_login_screen()) {
        return 1;
    }

    VehicleStore store;
    VehicleQueue queue;

    vehicle_store_init(&store);
    queue_init(&queue);
    charger_system_init(&store, &queue);

    log_event("SYSTEM", "Application started");

    for (;;) {
        show_menu();
        int c = read_int("Select option: ");

        if (c == 1) {
            dashboard_show_once(&store, &queue);
            getchar();
        } else if (c == 2) {
            char owner[OWNER_NAME_LEN], vno[VEHICLE_NO_LEN], type[CHARGE_TYPE_LEN];
            double cap;

            read_line("Owner name: ", owner, sizeof(owner));
            read_line("Vehicle number: ", vno, sizeof(vno));
            cap = read_double("Battery capacity (kWh): ");
            read_line("Charging type (slow/fast): ", type, sizeof(type));

            if (strcmp(type, "slow") != 0 && strcmp(type, "fast") != 0) {
                printf(COLOR_RED "Invalid charging type.\n" COLOR_RESET);
                continue;
            }

            int id = vehicle_register(&store, owner, vno, cap, type);
            if (id > 0) printf(COLOR_GREEN "Vehicle registered with ID %d\n" COLOR_RESET, id);
            else if (id == -2) printf(COLOR_RED "Vehicle number already exists.\n" COLOR_RESET);
            else printf(COLOR_RED "Failed to register vehicle.\n" COLOR_RESET);
        } else if (c == 3) {
            vehicle_list_all(&store);
        } else if (c == 4) {
            int id = read_int("Vehicle ID to charge: ");
            if (charger_request_vehicle(id)) printf(COLOR_GREEN "Added to charging queue.\n" COLOR_RESET);
            else printf(COLOR_RED "Could not enqueue (invalid ID or queue full).\n" COLOR_RESET);
        } else if (c == 5) {
            int sid = read_int("Slot ID (1-4): ");
            if (charger_toggle_slot_offline(sid)) printf("Slot state toggled.\n");
            else printf("Unable to toggle slot (busy or invalid).\n");
        } else if (c == 6) {
            double rate = read_double("New rate per kWh: ");
            charger_set_rate(rate);
            printf("Rate updated to %.2f\n", charger_get_rate());
        } else if (c == 7) {
            RevenueReport r = report_daily_revenue();
            printf(COLOR_BOLD "\nRevenue Analytics\n" COLOR_RESET);
            printf("Total Sessions : %d\n", r.total_sessions);
            printf("Total Revenue  : %.2f\n", r.total_revenue);
            printf("Average Cost   : %.2f\n", r.avg_session_cost);
        } else if (c == 8) {
            RevenueReport r = report_daily_revenue();
            report_export_to_file(r);
            printf(COLOR_GREEN "Report exported to %s\n" COLOR_RESET, REPORT_EXPORT_FILE);
        } else if (c == 9) {
            break;
        } else {
            printf(COLOR_RED "Invalid choice.\n" COLOR_RESET);
        }
    }

    charger_system_shutdown();
    log_event("SYSTEM", "Application exiting");
    printf(COLOR_CYAN "Goodbye.\n" COLOR_RESET);
    return 0;
}
