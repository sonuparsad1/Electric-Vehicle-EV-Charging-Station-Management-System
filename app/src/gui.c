#include "gui.h"

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

#include "logic.h"

#define TIMER_ID 1
#define TIMER_MS 1000

#define ID_LOGIN_USER 100
#define ID_LOGIN_PASS 101
#define ID_LOGIN_BTN 102

#define ID_OWNER_EDIT 200
#define ID_VEHICLE_EDIT 201
#define ID_REGISTER_BTN 202
#define ID_QUEUE_ID_EDIT 203
#define ID_QUEUE_BTN 204
#define ID_REPORT_BTN 205
#define ID_VEHICLE_LIST 206
#define ID_LOG_EDIT 207

#define ID_BATT_START_EDIT 208
#define ID_BATT_TARGET_EDIT 209
#define ID_BATT_CAP_EDIT 210

#define ID_STAT_TOTAL_VEHICLES 300
#define ID_STAT_ACTIVE 301
#define ID_STAT_REVENUE 302
#define ID_STAT_QUEUE 303

#define ID_EST_ENERGY 310
#define ID_EST_BILL 311
#define ID_EST_TIME 312

#define ID_SLOT_STATUS_BASE 400
#define ID_SLOT_PROGRESS_BASE 500

static SystemState g_state;
static int g_login_attempts = 0;

static HWND hUser, hPass, hLoginBtn;
static HWND hOwner, hVehicleId, hRegisterBtn, hQueueId, hQueueBtn, hReportBtn;
static HWND hBattStart, hBattTarget, hBattCap;
static HWND hVehicleList, hLog;
static HWND hTotalVehicles, hActiveSessions, hTotalRevenue, hQueueSize;
static HWND hEstEnergy, hEstBill, hEstTime;
static HWND hSlotStatus[SLOT_COUNT], hSlotProgress[SLOT_COUNT];

static void append_log(const char *msg) {
    int len = GetWindowTextLengthA(hLog);
    SendMessageA(hLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageA(hLog, EM_REPLACESEL, FALSE, (LPARAM)msg);
    SendMessageA(hLog, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
}

static void draw_graph_panel(HWND hwnd, HDC hdc) {
    RECT rc;
    int i;
    int gx;
    int gy;
    int gw;
    int gh;

    GetClientRect(hwnd, &rc);
    gx = 760;
    gy = 30;
    gw = 310;
    gh = 360;

    Rectangle(hdc, gx, gy, gx + gw, gy + gh);
    TextOutA(hdc, gx + 10, gy + 10, "Live Charging Graph", 18);

    MoveToEx(hdc, gx + 30, gy + gh - 40, NULL);
    LineTo(hdc, gx + gw - 20, gy + gh - 40);
    MoveToEx(hdc, gx + 30, gy + 40, NULL);
    LineTo(hdc, gx + 30, gy + gh - 40);

    for (i = 0; i < SLOT_COUNT; i++) {
        int barX = gx + 60 + i * 80;
        int baseY = gy + gh - 40;
        int barH = (g_state.slots[i].progress * 220) / 100;
        RECT bar = {barX, baseY - barH, barX + 45, baseY};
        HBRUSH brush = CreateSolidBrush(g_state.slots[i].status == SLOT_CHARGING ? RGB(0, 180, 0) : RGB(180, 180, 180));
        char label[32];
        char pct[16];

        FillRect(hdc, &bar, brush);
        DeleteObject(brush);

        snprintf(label, sizeof(label), "S%d", i + 1);
        snprintf(pct, sizeof(pct), "%d%%", g_state.slots[i].progress);

        TextOutA(hdc, barX + 12, baseY + 5, label, (int)strlen(label));
        TextOutA(hdc, barX + 8, baseY - barH - 18, pct, (int)strlen(pct));
    }
}

static void show_login(BOOL show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    ShowWindow(hUser, cmd);
    ShowWindow(hPass, cmd);
    ShowWindow(hLoginBtn, cmd);
}

static void refresh_vehicle_list(void) {
    int i;
    char line[128];
    SendMessageA(hVehicleList, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < g_state.vehicle_count; i++) {
        snprintf(line, sizeof(line), "%s (%s)", g_state.vehicles[i].owner, g_state.vehicles[i].vehicle_id);
        SendMessageA(hVehicleList, LB_ADDSTRING, 0, (LPARAM)line);
    }
}

static void refresh_dashboard(HWND hwnd) {
    char buf[128];
    int i;

    snprintf(buf, sizeof(buf), "Total Vehicles: %d", g_state.vehicle_count);
    SetWindowTextA(hTotalVehicles, buf);

    snprintf(buf, sizeof(buf), "Active Sessions: %d", logic_active_sessions(&g_state));
    SetWindowTextA(hActiveSessions, buf);

    snprintf(buf, sizeof(buf), "Queue Size: %d", g_state.queue.count);
    SetWindowTextA(hQueueSize, buf);

    snprintf(buf, sizeof(buf), "Total Revenue: %.2f", g_state.total_revenue);
    SetWindowTextA(hTotalRevenue, buf);

    for (i = 0; i < SLOT_COUNT; i++) {
        if (g_state.slots[i].status == SLOT_AVAILABLE) {
            snprintf(buf, sizeof(buf), "Slot %d: Available", i + 1);
        } else {
            snprintf(buf,
                     sizeof(buf),
                     "Slot %d: %s | %d%% | %.2f/%.2f kWh",
                     i + 1,
                     g_state.slots[i].vehicle.vehicle_id,
                     g_state.slots[i].progress,
                     g_state.slots[i].energy_kwh,
                     g_state.slots[i].target_energy_kwh);
        }
        SetWindowTextA(hSlotStatus[i], buf);
        SendMessageA(hSlotProgress[i], PBM_SETPOS, (WPARAM)g_state.slots[i].progress, 0);
    }

    InvalidateRect(hwnd, NULL, FALSE);
}

static void show_dashboard(BOOL show) {
    int cmd = show ? SW_SHOW : SW_HIDE;
    int i;

    ShowWindow(hOwner, cmd);
    ShowWindow(hVehicleId, cmd);
    ShowWindow(hRegisterBtn, cmd);
    ShowWindow(hQueueId, cmd);
    ShowWindow(hQueueBtn, cmd);
    ShowWindow(hReportBtn, cmd);
    ShowWindow(hBattStart, cmd);
    ShowWindow(hBattTarget, cmd);
    ShowWindow(hBattCap, cmd);
    ShowWindow(hVehicleList, cmd);
    ShowWindow(hLog, cmd);
    ShowWindow(hTotalVehicles, cmd);
    ShowWindow(hActiveSessions, cmd);
    ShowWindow(hTotalRevenue, cmd);
    ShowWindow(hQueueSize, cmd);
    ShowWindow(hEstEnergy, cmd);
    ShowWindow(hEstBill, cmd);
    ShowWindow(hEstTime, cmd);

    for (i = 0; i < SLOT_COUNT; i++) {
        ShowWindow(hSlotStatus[i], cmd);
        ShowWindow(hSlotProgress[i], cmd);
    }
}

static void handle_login(HWND hwnd) {
    char user[64], pass[64];

    GetWindowTextA(hUser, user, sizeof(user));
    GetWindowTextA(hPass, pass, sizeof(pass));

    if (logic_authenticate(user, pass)) {
        MessageBoxA(hwnd, "Login successful.", "Authentication", MB_OK | MB_ICONINFORMATION);
        show_login(FALSE);
        show_dashboard(TRUE);
        SetTimer(hwnd, TIMER_ID, TIMER_MS, NULL);
        append_log("Administrator logged in.");
        refresh_vehicle_list();
        refresh_dashboard(hwnd);
    } else {
        g_login_attempts++;
        if (g_login_attempts >= 3) {
            MessageBoxA(hwnd, "Maximum login attempts reached. Application will close.", "Authentication", MB_OK | MB_ICONERROR);
            PostQuitMessage(0);
        } else {
            MessageBoxA(hwnd, "Invalid username or password.", "Authentication", MB_OK | MB_ICONERROR);
        }
    }
}

static void handle_register(HWND hwnd) {
    char owner[64], id[32], err[128];

    GetWindowTextA(hOwner, owner, sizeof(owner));
    GetWindowTextA(hVehicleId, id, sizeof(id));

    if (logic_register_vehicle(&g_state, owner, id, err, sizeof(err))) {
        MessageBoxA(hwnd, "Vehicle registered successfully.", "Vehicle Management", MB_OK | MB_ICONINFORMATION);
        SetWindowTextA(hOwner, "");
        SetWindowTextA(hVehicleId, "");
        refresh_vehicle_list();
        refresh_dashboard(hwnd);
    } else {
        MessageBoxA(hwnd, err, "Vehicle Management", MB_OK | MB_ICONERROR);
    }
}

static void update_estimation_labels(void) {
    char sStart[16], sTarget[16], sCap[16], line[128];
    int startPct, targetPct;
    double cap, energy, bill;
    int mins;

    GetWindowTextA(hBattStart, sStart, sizeof(sStart));
    GetWindowTextA(hBattTarget, sTarget, sizeof(sTarget));
    GetWindowTextA(hBattCap, sCap, sizeof(sCap));

    startPct = atoi(sStart);
    targetPct = atoi(sTarget);
    cap = atof(sCap);

    energy = logic_calculate_energy_request(startPct, targetPct, cap);
    if (energy < 0.0) {
        energy = 0.0;
    }
    bill = logic_calculate_estimated_bill(energy);
    mins = logic_calculate_estimated_minutes(energy);

    snprintf(line, sizeof(line), "Estimated Energy: %.2f kWh", energy);
    SetWindowTextA(hEstEnergy, line);
    snprintf(line, sizeof(line), "Estimated Bill: %.2f", bill);
    SetWindowTextA(hEstBill, line);
    snprintf(line, sizeof(line), "Estimated Time: %d min", mins);
    SetWindowTextA(hEstTime, line);
}

static void handle_enqueue(HWND hwnd) {
    char id[32], err[128];
    char sStart[16], sTarget[16], sCap[16];
    int startPct, targetPct;
    double cap;
    char logLine[180];

    GetWindowTextA(hQueueId, id, sizeof(id));
    GetWindowTextA(hBattStart, sStart, sizeof(sStart));
    GetWindowTextA(hBattTarget, sTarget, sizeof(sTarget));
    GetWindowTextA(hBattCap, sCap, sizeof(sCap));

    startPct = atoi(sStart);
    targetPct = atoi(sTarget);
    cap = atof(sCap);

    if (logic_enqueue_vehicle(&g_state, id, startPct, targetPct, cap, err, sizeof(err))) {
        double energy = logic_calculate_energy_request(startPct, targetPct, cap);
        double bill = logic_calculate_estimated_bill(energy);

        snprintf(logLine,
                 sizeof(logLine),
                 "Queued %s | Battery %d%%->%d%% | %.2f kWh | Est %.2f",
                 id,
                 startPct,
                 targetPct,
                 energy,
                 bill);
        append_log(logLine);

        SetWindowTextA(hQueueId, "");
        SetWindowTextA(hBattStart, "20");
        SetWindowTextA(hBattTarget, "80");
        SetWindowTextA(hBattCap, "50");

        update_estimation_labels();
        refresh_dashboard(hwnd);
    } else {
        MessageBoxA(hwnd, err, "Queue", MB_OK | MB_ICONERROR);
    }
}

static void handle_generate_report(HWND hwnd) {
    char err[128];
    if (logic_generate_report(&g_state, err, sizeof(err))) {
        MessageBoxA(hwnd, "Report generated: report.txt", "Revenue Report", MB_OK | MB_ICONINFORMATION);
        append_log("Revenue report generated.");
    } else {
        MessageBoxA(hwnd, err, "Revenue Report", MB_OK | MB_ICONERROR);
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            int i;

            logic_init(&g_state);

            CreateWindowA("STATIC", "Username", WS_CHILD | WS_VISIBLE, 50, 60, 120, 24, hwnd, NULL, NULL, NULL);
            hUser = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 170, 60, 240, 24, hwnd, (HMENU)ID_LOGIN_USER, NULL, NULL);

            CreateWindowA("STATIC", "Password", WS_CHILD | WS_VISIBLE, 50, 100, 120, 24, hwnd, NULL, NULL, NULL);
            hPass = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 170, 100, 240, 24, hwnd, (HMENU)ID_LOGIN_PASS, NULL, NULL);

            hLoginBtn = CreateWindowA("BUTTON", "Login", WS_CHILD | WS_VISIBLE, 170, 140, 120, 32, hwnd, (HMENU)ID_LOGIN_BTN, NULL, NULL);

            CreateWindowA("BUTTON", "Vehicle Registration", WS_CHILD | BS_GROUPBOX, 20, 20, 360, 130, hwnd, NULL, NULL, NULL);
            CreateWindowA("STATIC", "Owner", WS_CHILD, 35, 50, 70, 22, hwnd, NULL, NULL, NULL);
            hOwner = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER, 110, 50, 220, 24, hwnd, (HMENU)ID_OWNER_EDIT, NULL, NULL);
            CreateWindowA("STATIC", "Vehicle Number", WS_CHILD, 35, 83, 90, 22, hwnd, NULL, NULL, NULL);
            hVehicleId = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER, 130, 82, 130, 24, hwnd, (HMENU)ID_VEHICLE_EDIT, NULL, NULL);
            hRegisterBtn = CreateWindowA("BUTTON", "Register", WS_CHILD, 270, 82, 70, 24, hwnd, (HMENU)ID_REGISTER_BTN, NULL, NULL);

            CreateWindowA("BUTTON", "Charging Request / Queue", WS_CHILD | BS_GROUPBOX, 20, 160, 360, 220, hwnd, NULL, NULL, NULL);
            CreateWindowA("STATIC", "Vehicle Number", WS_CHILD, 35, 190, 95, 22, hwnd, NULL, NULL, NULL);
            hQueueId = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER, 140, 190, 190, 24, hwnd, (HMENU)ID_QUEUE_ID_EDIT, NULL, NULL);
            CreateWindowA("STATIC", "Battery Now %", WS_CHILD, 35, 220, 95, 22, hwnd, NULL, NULL, NULL);
            hBattStart = CreateWindowA("EDIT", "20", WS_CHILD | WS_BORDER, 140, 220, 70, 24, hwnd, (HMENU)ID_BATT_START_EDIT, NULL, NULL);
            CreateWindowA("STATIC", "Target %", WS_CHILD, 220, 220, 60, 22, hwnd, NULL, NULL, NULL);
            hBattTarget = CreateWindowA("EDIT", "80", WS_CHILD | WS_BORDER, 280, 220, 50, 24, hwnd, (HMENU)ID_BATT_TARGET_EDIT, NULL, NULL);
            CreateWindowA("STATIC", "Battery kWh", WS_CHILD, 35, 250, 95, 22, hwnd, NULL, NULL, NULL);
            hBattCap = CreateWindowA("EDIT", "50", WS_CHILD | WS_BORDER, 140, 250, 100, 24, hwnd, (HMENU)ID_BATT_CAP_EDIT, NULL, NULL);

            hEstEnergy = CreateWindowA("STATIC", "Estimated Energy: 0", WS_CHILD, 35, 283, 290, 20, hwnd, (HMENU)ID_EST_ENERGY, NULL, NULL);
            hEstBill = CreateWindowA("STATIC", "Estimated Bill: 0", WS_CHILD, 35, 305, 290, 20, hwnd, (HMENU)ID_EST_BILL, NULL, NULL);
            hEstTime = CreateWindowA("STATIC", "Estimated Time: 0", WS_CHILD, 35, 327, 290, 20, hwnd, (HMENU)ID_EST_TIME, NULL, NULL);

            hQueueBtn = CreateWindowA("BUTTON", "Add to Queue", WS_CHILD, 140, 350, 95, 24, hwnd, (HMENU)ID_QUEUE_BTN, NULL, NULL);
            hReportBtn = CreateWindowA("BUTTON", "Generate Report", WS_CHILD, 245, 350, 95, 24, hwnd, (HMENU)ID_REPORT_BTN, NULL, NULL);

            CreateWindowA("BUTTON", "Dashboard", WS_CHILD | BS_GROUPBOX, 390, 20, 360, 370, hwnd, NULL, NULL, NULL);
            hTotalVehicles = CreateWindowA("STATIC", "Total Vehicles: 0", WS_CHILD, 410, 48, 220, 22, hwnd, (HMENU)ID_STAT_TOTAL_VEHICLES, NULL, NULL);
            hActiveSessions = CreateWindowA("STATIC", "Active Sessions: 0", WS_CHILD, 410, 72, 220, 22, hwnd, (HMENU)ID_STAT_ACTIVE, NULL, NULL);
            hQueueSize = CreateWindowA("STATIC", "Queue Size: 0", WS_CHILD, 410, 96, 220, 22, hwnd, (HMENU)ID_STAT_QUEUE, NULL, NULL);
            hTotalRevenue = CreateWindowA("STATIC", "Total Revenue: 0", WS_CHILD, 410, 120, 240, 22, hwnd, (HMENU)ID_STAT_REVENUE, NULL, NULL);

            for (i = 0; i < SLOT_COUNT; i++) {
                hSlotStatus[i] = CreateWindowA("STATIC", "", WS_CHILD, 410, 145 + i * 35, 220, 22, hwnd, (HMENU)(INT_PTR)(ID_SLOT_STATUS_BASE + i), NULL, NULL);
                hSlotProgress[i] = CreateWindowExA(0, PROGRESS_CLASSA, "", WS_CHILD, 630, 145 + i * 35, 120, 20,
                                                   hwnd, (HMENU)(INT_PTR)(ID_SLOT_PROGRESS_BASE + i), NULL, NULL);
                SendMessageA(hSlotProgress[i], PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            }

            CreateWindowA("BUTTON", "Registered Vehicles", WS_CHILD | BS_GROUPBOX, 20, 390, 360, 260, hwnd, NULL, NULL, NULL);
            hVehicleList = CreateWindowA("LISTBOX", "", WS_CHILD | WS_BORDER | LBS_NOTIFY, 35, 420, 330, 215,
                                         hwnd, (HMENU)ID_VEHICLE_LIST, NULL, NULL);

            CreateWindowA("BUTTON", "System Log", WS_CHILD | BS_GROUPBOX, 390, 400, 680, 250, hwnd, NULL, NULL, NULL);
            hLog = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                                 405, 430, 650, 205, hwnd, (HMENU)ID_LOG_EDIT, NULL, NULL);

            show_dashboard(FALSE);
            update_estimation_labels();
            refresh_dashboard(hwnd);
            return 0;
        }

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_LOGIN_BTN:
                    handle_login(hwnd);
                    break;
                case ID_REGISTER_BTN:
                    handle_register(hwnd);
                    break;
                case ID_QUEUE_BTN:
                    handle_enqueue(hwnd);
                    break;
                case ID_REPORT_BTN:
                    handle_generate_report(hwnd);
                    break;
                case ID_BATT_START_EDIT:
                case ID_BATT_TARGET_EDIT:
                case ID_BATT_CAP_EDIT:
                    if (HIWORD(wParam) == EN_CHANGE) {
                        update_estimation_labels();
                    }
                    break;
            }
            return 0;

        case WM_TIMER:
            if (wParam == TIMER_ID) {
                LogicEvents events;
                int i;
                logic_tick(&g_state, &events);
                for (i = 0; i < events.count; i++) {
                    append_log(events.messages[i]);
                }
                refresh_dashboard(hwnd);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            draw_graph_panel(hwnd, hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            KillTimer(hwnd, TIMER_ID);
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int gui_run(HINSTANCE hInstance, int nCmdShow) {
    WNDCLASSA wc;
    HWND hwnd;
    MSG msg;

    InitCommonControls();

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "EVChargeAppClass";

    RegisterClassA(&wc);

    hwnd = CreateWindowA(
        wc.lpszClassName,
        "EV Charging Station Management System",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1110,
        720,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hwnd) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
