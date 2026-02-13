@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0"

echo ================================================
echo EV Charging Station Management System - One Click
echo ================================================
echo.

where gcc >nul 2>nul
if errorlevel 1 (
    echo [ERROR] GCC (MinGW) not found in PATH.
    echo Install MinGW and ensure gcc.exe is available in PATH.
    echo.
    echo Press any key to exit...
    pause >nul
    exit /b 1
)

echo [1/4] Compiling WinAPI GUI application...
gcc main.c gui.c logic.c storage.c billing.c -o EVChargingApp.exe -mwindows -lcomctl32
if errorlevel 1 (
    echo [WARN] First compile command failed, retrying without -lcomctl32...
    gcc main.c gui.c logic.c storage.c billing.c -o EVChargingApp.exe -mwindows
)
if errorlevel 1 (
    echo [ERROR] Build failed.
    echo Make sure you are using MinGW GCC for Windows.
    echo.
    echo Press any key to exit...
    pause >nul
    exit /b 1
)

echo [2/4] Preparing runtime files...
if not exist vehicles.txt type nul > vehicles.txt
if not exist sessions.txt type nul > sessions.txt

if not exist EVChargingApp_Portable mkdir EVChargingApp_Portable
copy /Y EVChargingApp.exe EVChargingApp_Portable\ >nul
copy /Y README.md EVChargingApp_Portable\ >nul
copy /Y vehicles.txt EVChargingApp_Portable\ >nul
copy /Y sessions.txt EVChargingApp_Portable\ >nul

echo [3/4] Launching application...
start "EV Charging Station Management System" "EVChargingApp.exe"

echo [4/4] Done.
echo Built: EVChargingApp.exe
echo Portable folder: EVChargingApp_Portable\
echo.
echo Login:
echo   Username: admin
echo   Password: 1234
echo.
echo Press any key to close this window...
pause >nul
exit /b 0
