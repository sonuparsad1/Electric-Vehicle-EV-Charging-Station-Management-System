# EV Charging Station Management System (WinAPI, C)

A desktop **Windows GUI** application written in pure C using the WinAPI.

## Requirements

- Windows 10/11
- MinGW (GCC for Windows) or Code::Blocks with MinGW compiler
- No internet required at runtime (offline app)

## 1) Download the project

### Option A: Download ZIP from GitHub
1. Open the repository page in your browser.
2. Click **Code** -> **Download ZIP**.
3. Extract the ZIP to a folder, for example:
   - `C:\Projects\Electric-Vehicle-EV-Charging-Station-Management-System`

### Option B: Clone with Git
```bash
git clone <YOUR_REPO_URL>
cd Electric-Vehicle-EV-Charging-Station-Management-System
```

## 2) Build with MinGW (Command Prompt)

Open **Command Prompt** in the project folder, then run:

```bash
gcc main.c gui.c logic.c storage.c billing.c -o EVChargingApp.exe -mwindows -lcomctl32
```

> If your setup already links common controls automatically, this also works:

```bash
gcc main.c gui.c logic.c storage.c billing.c -o EVChargingApp.exe -mwindows
```

## 3) Run the app

Double-click:

- `EVChargingApp.exe`

Or run from terminal:

```bash
EVChargingApp.exe
```

## 4) Login credentials

- Username: `admin`
- Password: `1234`
- Maximum login attempts: `3`

## 5) Data files generated during use

The app creates/uses files in the same folder as the EXE:

- `vehicles.txt` (registered vehicles)
- `sessions.txt` (completed charging sessions and bills)
- `report.txt` (generated revenue report)

## 6) How to use the app quickly

1. Login with admin credentials.
2. Register vehicle (Owner + Vehicle ID).
3. Add vehicle to queue by Vehicle ID.
4. Observe slot progress bars (3 charging slots).
5. Click **Generate Report** to export `report.txt`.

## 7) Build with Code::Blocks (alternative)

1. Open Code::Blocks.
2. Create an empty C project or import sources.
3. Add files:
   - `main.c`, `gui.c`, `logic.c`, `storage.c`, `billing.c`
4. In linker settings, ensure `comctl32` is linked.
5. Build and run.

## Notes

- This is a GUI app (no console window) because it uses `WinMain` and `-mwindows`.
- The application is designed to run fully offline.
