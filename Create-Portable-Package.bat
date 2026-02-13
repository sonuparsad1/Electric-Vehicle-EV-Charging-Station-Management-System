@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0"

echo ================================================
echo Create Portable ZIP Package - EVChargingApp
echo ================================================
echo.

call OneClick-Build-Run.bat
if errorlevel 1 (
    echo [ERROR] Cannot package because build failed.
    exit /b 1
)

if not exist EVChargingApp_Portable (
    echo [ERROR] EVChargingApp_Portable folder not found.
    exit /b 1
)

echo Creating EVChargingApp_Portable.zip ...
powershell -NoProfile -ExecutionPolicy Bypass -Command "if (Test-Path 'EVChargingApp_Portable.zip') { Remove-Item 'EVChargingApp_Portable.zip' -Force }; Compress-Archive -Path 'EVChargingApp_Portable\\*' -DestinationPath 'EVChargingApp_Portable.zip' -Force"
if errorlevel 1 (
    echo [ERROR] Failed to create zip package.
    exit /b 1
)

echo.
echo Package created successfully: EVChargingApp_Portable.zip
echo Share this ZIP for one-click run after extraction.
exit /b 0
