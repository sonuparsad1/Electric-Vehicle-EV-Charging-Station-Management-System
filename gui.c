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

#define ID_STAT_TOTAL_VEHICLES 300
#define ID_STAT_ACTIVE 301
#define ID_STAT_REVENUE 302

#define ID_SLOT_STATUS_BASE 400
#define ID_SLOT_PROGRESS_BASE 500

static SystemState g_state;
static int g_login_attempts = 0;

static HWND hUser, hPass, hLoginBtn;
static HWND hOwner, hVehicleId, hRegisterBtn, hQueueId, hQueueBtn, hReportBtn;
static HWND hVehicleList, hLog;
static HWND hTotalVehicles, hActiveSessions, hTotalRevenue;
static HWND hSlotStatus[SLOT_COUNT], hSlotProgress[SLOT_COUNT];

static void append_log(const char *msg) {
    int len = GetWindowTextLengthA(hLog);
    SendMessageA(hLog, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessageA(hLog, EM_REPLACESEL, FALSE, (LPARAM)msg);
    SendMessageA(hLog, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
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

static void refresh_dashboard(void) {
    char buf[128];
    int i;

    snprintf(buf, sizeof(buf), "Total Vehicles: %d", g_state.vehicle_count);
    SetWindowTextA(hTotalVehicles, buf);

    snprintf(buf, sizeof(buf), "Active Sessions: %d", logic_active_sessions(&g_state));
    SetWindowTextA(hActiveSessions, buf);

    snprintf(buf, sizeof(buf), "Total Revenue: %.2f", g_state.total_revenue);
    SetWindowTextA(hTotalRevenue, buf);

    for (i = 0; i < SLOT_COUNT; i++) {
        if (g_state.slots[i].status == SLOT_AVAILABLE) {
            snprintf(buf, sizeof(buf), "Slot %d: Available", i + 1);
        } else {
            snprintf(buf, sizeof(buf), "Slot %d: Charging %s (%d%%)", i + 1,
                     g_state.slots[i].vehicle.vehicle_id,
                     g_state.slots[i].progress);
        }
        SetWindowTextA(hSlotStatus[i], buf);
        SendMessageA(hSlotProgress[i], PBM_SETPOS, (WPARAM)g_state.slots[i].progress, 0);
    }
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
    ShowWindow(hVehicleList, cmd);
    ShowWindow(hLog, cmd);
    ShowWindow(hTotalVehicles, cmd);
    ShowWindow(hActiveSessions, cmd);
    ShowWindow(hTotalRevenue, cmd);

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
        refresh_dashboard();
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
        refresh_dashboard();
    } else {
        MessageBoxA(hwnd, err, "Vehicle Management", MB_OK | MB_ICONERROR);
    }
}

static void handle_enqueue(HWND hwnd) {
    char id[32], err[128];

    GetWindowTextA(hQueueId, id, sizeof(id));

    if (logic_enqueue_vehicle(&g_state, id, err, sizeof(err))) {
        append_log("Vehicle added to FIFO queue.");
        SetWindowTextA(hQueueId, "");
        refresh_dashboard();
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

            CreateWindowA("STATIC", "Username", WS_CHILD | WS_VISIBLE, 40, 40, 120, 24, hwnd, NULL, NULL, NULL);
            hUser = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 160, 40, 220, 24, hwnd, (HMENU)ID_LOGIN_USER, NULL, NULL);

            CreateWindowA("STATIC", "Password", WS_CHILD | WS_VISIBLE, 40, 80, 120, 24, hwnd, NULL, NULL, NULL);
            hPass = CreateWindowA("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_PASSWORD, 160, 80, 220, 24, hwnd, (HMENU)ID_LOGIN_PASS, NULL, NULL);

            hLoginBtn = CreateWindowA("BUTTON", "Login", WS_CHILD | WS_VISIBLE, 160, 120, 100, 30, hwnd, (HMENU)ID_LOGIN_BTN, NULL, NULL);

            CreateWindowA("BUTTON", "Vehicle Registration", WS_CHILD | BS_GROUPBOX, 20, 20, 360, 120, hwnd, NULL, NULL, NULL);
            CreateWindowA("STATIC", "Owner", WS_CHILD, 40, 50, 80, 24, hwnd, NULL, NULL, NULL);
            hOwner = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER, 130, 50, 220, 24, hwnd, (HMENU)ID_OWNER_EDIT, NULL, NULL);
            CreateWindowA("STATIC", "Vehicle ID", WS_CHILD, 40, 80, 80, 24, hwnd, NULL, NULL, NULL);
            hVehicleId = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER, 130, 80, 140, 24, hwnd, (HMENU)ID_VEHICLE_EDIT, NULL, NULL);
            hRegisterBtn = CreateWindowA("BUTTON", "Register", WS_CHILD, 280, 80, 70, 24, hwnd, (HMENU)ID_REGISTER_BTN, NULL, NULL);

            CreateWindowA("BUTTON", "Queue Management", WS_CHILD | BS_GROUPBOX, 20, 150, 360, 85, hwnd, NULL, NULL, NULL);
            CreateWindowA("STATIC", "Queue Vehicle ID", WS_CHILD, 40, 180, 100, 24, hwnd, NULL, NULL, NULL);
            hQueueId = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER, 150, 180, 120, 24, hwnd, (HMENU)ID_QUEUE_ID_EDIT, NULL, NULL);
            hQueueBtn = CreateWindowA("BUTTON", "Add to Queue", WS_CHILD, 280, 180, 90, 24, hwnd, (HMENU)ID_QUEUE_BTN, NULL, NULL);

            hReportBtn = CreateWindowA("BUTTON", "Generate Report", WS_CHILD, 20, 245, 140, 28, hwnd, (HMENU)ID_REPORT_BTN, NULL, NULL);

            CreateWindowA("BUTTON", "Dashboard", WS_CHILD | BS_GROUPBOX, 390, 20, 380, 255, hwnd, NULL, NULL, NULL);
            hTotalVehicles = CreateWindowA("STATIC", "Total Vehicles: 0", WS_CHILD, 410, 50, 220, 24, hwnd, (HMENU)ID_STAT_TOTAL_VEHICLES, NULL, NULL);
            hActiveSessions = CreateWindowA("STATIC", "Active Sessions: 0", WS_CHILD, 410, 80, 220, 24, hwnd, (HMENU)ID_STAT_ACTIVE, NULL, NULL);
            hTotalRevenue = CreateWindowA("STATIC", "Total Revenue: 0", WS_CHILD, 410, 110, 220, 24, hwnd, (HMENU)ID_STAT_REVENUE, NULL, NULL);

            for (i = 0; i < SLOT_COUNT; i++) {
                hSlotStatus[i] = CreateWindowA("STATIC", "", WS_CHILD, 410, 145 + i * 35, 220, 22, hwnd, (HMENU)(ID_SLOT_STATUS_BASE + i), NULL, NULL);
                hSlotProgress[i] = CreateWindowExA(0, PROGRESS_CLASSA, "", WS_CHILD, 630, 145 + i * 35, 120, 20,
                                                   hwnd, (HMENU)(ID_SLOT_PROGRESS_BASE + i), NULL, NULL);
                SendMessageA(hSlotProgress[i], PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            }

            CreateWindowA("BUTTON", "Registered Vehicles", WS_CHILD | BS_GROUPBOX, 20, 285, 360, 240, hwnd, NULL, NULL, NULL);
            hVehicleList = CreateWindowA("LISTBOX", "", WS_CHILD | WS_BORDER | LBS_NOTIFY, 35, 315, 330, 195,
                                         hwnd, (HMENU)ID_VEHICLE_LIST, NULL, NULL);

            CreateWindowA("BUTTON", "System Log", WS_CHILD | BS_GROUPBOX, 390, 285, 380, 240, hwnd, NULL, NULL, NULL);
            hLog = CreateWindowA("EDIT", "", WS_CHILD | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | ES_READONLY,
                                 405, 315, 350, 195, hwnd, (HMENU)ID_LOG_EDIT, NULL, NULL);

            show_dashboard(FALSE);
            refresh_dashboard();
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
                refresh_dashboard();
            }
            return 0;

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
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        810,
        590,
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
