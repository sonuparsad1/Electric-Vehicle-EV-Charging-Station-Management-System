@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0\.."

echo ================================================
echo EV Charging Station Management System - One Click
echo ================================================
echo.

where gcc >nul 2>nul
if errorlevel 1 (
    echo [ERROR] GCC (MinGW) not found in PATH.
    echo Install MinGW and ensure gcc.exe is available in PATH.
    echo Press any key to exit...
    pause >nul
    exit /b 1
)

if not exist build mkdir build
if not exist dist mkdir dist

echo [1/4] Compiling WinAPI GUI application...
gcc app\src\main.c app\src\gui.c app\src\logic.c app\src\storage.c app\src\billing.c -Iapp\include -o build\EVChargingApp.exe -mwindows -lcomctl32
if errorlevel 1 (
    echo [WARN] Retry build without -lcomctl32...
    gcc app\src\main.c app\src\gui.c app\src\logic.c app\src\storage.c app\src\billing.c -Iapp\include -o build\EVChargingApp.exe -mwindows
)
if errorlevel 1 (
    echo [ERROR] Build failed.
    echo Press any key to exit...
    pause >nul
    exit /b 1
)

echo [2/4] Preparing runtime files...
if not exist build\vehicles.txt type nul > build\vehicles.txt
if not exist build\sessions.txt type nul > build\sessions.txt

if exist dist\EVChargingApp_Portable rmdir /s /q dist\EVChargingApp_Portable
mkdir dist\EVChargingApp_Portable
copy /Y build\EVChargingApp.exe dist\EVChargingApp_Portable\ >nul
copy /Y README.md dist\EVChargingApp_Portable\ >nul
copy /Y build\vehicles.txt dist\EVChargingApp_Portable\ >nul
copy /Y build\sessions.txt dist\EVChargingApp_Portable\ >nul

echo [3/4] Launching application...
pushd build
start "EV Charging Station Management System" "EVChargingApp.exe"
popd

echo [4/4] Done.
echo App EXE: build\EVChargingApp.exe
echo Portable folder: dist\EVChargingApp_Portable\
echo.
echo Login: admin / 1234
echo Press any key to close this window...
pause >nul
exit /b 0
