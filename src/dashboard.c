#include "dashboard.h"
#include "charger.h"
#include "report.h"
#include "common.h"

#include <unistd.h>
#include <stdio.h>

static const char *status_text(SlotStatus s) {
    if (s == SLOT_IDLE) return COLOR_GREEN "AVAILABLE" COLOR_RESET;
    if (s == SLOT_RESERVED) return COLOR_YELLOW "RESERVED" COLOR_RESET;
    if (s == SLOT_CHARGING) return COLOR_RED "BUSY" COLOR_RESET;
    return COLOR_WHITE "OFFLINE" COLOR_RESET;
}

void dashboard_show_once(VehicleStore *store, VehicleQueue *queue) {
    RevenueReport rpt = report_daily_revenue();
    ChargingSlot snaps[MAX_SLOTS];
    charger_get_slots_snapshot(snaps, MAX_SLOTS);

    clear_screen();
    printf(COLOR_BOLD COLOR_CYAN "+================================================================+\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_CYAN "|             EV CHARGING STATION MANAGEMENT DASHBOARD          |\n" COLOR_RESET);
    printf(COLOR_BOLD COLOR_CYAN "+================================================================+\n" COLOR_RESET);

    printf(BG_DARK COLOR_WHITE " Vehicles: %-5d " COLOR_RESET "  ", vehicle_total(store));
    printf(BG_DARK COLOR_WHITE " Active Sessions: %-3d " COLOR_RESET "  ", charger_active_sessions());
    printf(BG_DARK COLOR_WHITE " Available Slots: %-3d " COLOR_RESET "\n", charger_available_slots());

    printf(BG_DARK COLOR_WHITE " Queue Size: %-5d " COLOR_RESET "  ", queue_size(queue));
    printf(BG_DARK COLOR_WHITE " Revenue Today: %-10.2f " COLOR_RESET "  ", rpt.total_revenue);
    printf(BG_DARK COLOR_WHITE " Avg Session: %-8.2f " COLOR_RESET "\n", rpt.avg_session_cost);

    printf("\n" COLOR_BOLD "Slot Status" COLOR_RESET "\n");
    printf("---------------------------------------------------------------\n");
    printf("%-8s %-12s %-12s %-12s %-10s\n", "SlotID", "Status", "VehicleID", "Energy(kWh)", "Cost");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < MAX_SLOTS; i++) {
        printf("%-8d %-12s %-12d %-12.2f %-10.2f\n",
               snaps[i].slot_id,
               status_text(snaps[i].status),
               snaps[i].current_vehicle_id,
               snaps[i].delivered_kwh,
               snaps[i].current_cost);
    }

    queue_visual_print(queue);
    printf("\nRate: %.2f /kWh\n", charger_get_rate());
    printf("\nPress Enter to continue...\n");
}

void dashboard_live_view(VehicleStore *store, VehicleQueue *queue, int seconds) {
    for (int i = 0; i < seconds; i++) {
        dashboard_show_once(store, queue);
        sleep_ms(500);
    }
}
