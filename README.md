# EV Charging Station Management System (WinAPI, C)

Professional Windows desktop GUI app written in pure C using WinAPI.

## Project Structure

```text
.
├─ app/
│  ├─ include/
│  │  ├─ gui.h
│  │  ├─ logic.h
│  │  ├─ storage.h
│  │  └─ billing.h
│  └─ src/
│     ├─ main.c
│     ├─ gui.c
│     ├─ logic.c
│     ├─ storage.c
│     └─ billing.c
├─ scripts/
│  ├─ OneClick-Build-Run.bat
│  └─ Create-Portable-Package.bat
├─ Build-App-OneClick.bat
├─ Package-App-OneClick.bat
├─ build/   (created at build time)
├─ dist/    (created at build/package time)
└─ legacy/  (older console project files archived)
```

---

## Repository organization

For a clean repo map, see: `docs/REPO_STRUCTURE.md`.

- Active code: `app/src`, `app/include`
- Scripts: `scripts/` + root one-click wrappers
- Legacy archived code: `legacy/`

---

## One-click (Create App)

1. Download ZIP from GitHub (**Code -> Download ZIP**).
2. Extract it on Windows.
3. Double-click **`Build-App-OneClick.bat`**.

This will automatically:
- compile the GUI app,
- create `build\EVChargingApp.exe`,
- prepare runtime files,
- launch the app.

Default login:
- **Username:** `admin`
- **Password:** `1234`

---

## One-click (Create Downloadable Package)

Double-click **`Package-App-OneClick.bat`**.

This generates:
- `dist\EVChargingApp_Portable.zip`

Share this ZIP. Anyone can extract and run `EVChargingApp.exe`.

---

## Manual Compile (MinGW)

```bat
gcc app\src\main.c app\src\gui.c app\src\logic.c app\src\storage.c app\src\billing.c -Iapp\include -o build\EVChargingApp.exe -mwindows -lcomctl32
```

Fallback (if `-lcomctl32` not needed):

```bat
gcc app\src\main.c app\src\gui.c app\src\logic.c app\src\storage.c app\src\billing.c -Iapp\include -o build\EVChargingApp.exe -mwindows
```

---

## Runtime Data Files

In `build\` folder:
- `vehicles.txt`
- `sessions.txt`
- `report.txt` (after Generate Report)

---

## If build fails with GCC not found

- Install MinGW (GCC for Windows).
- Add MinGW `bin` folder to Windows `PATH`.
- Re-run `Build-App-OneClick.bat`.

## Conflict-safe entry files

If your branch still references old root file paths (`main.c`, `billing.c`, `billing.h`),
these are now compatibility shims that point to canonical files in `app/src` and `app/include`.
Use the `app/` paths as source of truth.

## Realistic charging fields in UI

Queue screen now captures:
- Vehicle Number
- Battery Now %%
- Target %%
- Battery Capacity (kWh)

App shows:
- Estimated Energy (kWh)
- Estimated Bill
- Estimated Time
- Live graph panel for slot progress

