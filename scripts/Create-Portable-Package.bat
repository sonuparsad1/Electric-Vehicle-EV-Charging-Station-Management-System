@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0\.."

echo ================================================
echo Create Portable ZIP Package - EVChargingApp
echo ================================================
echo.

call scripts\OneClick-Build-Run.bat
if errorlevel 1 (
    echo [ERROR] Build failed. Package not created.
    exit /b 1
)

if not exist dist\EVChargingApp_Portable (
    echo [ERROR] dist\EVChargingApp_Portable not found.
    exit /b 1
)

echo Creating dist\EVChargingApp_Portable.zip ...
powershell -NoProfile -ExecutionPolicy Bypass -Command "if (Test-Path 'dist\\EVChargingApp_Portable.zip') { Remove-Item 'dist\\EVChargingApp_Portable.zip' -Force }; Compress-Archive -Path 'dist\\EVChargingApp_Portable\\*' -DestinationPath 'dist\\EVChargingApp_Portable.zip' -Force"
if errorlevel 1 (
    echo [ERROR] Failed to create zip package.
    exit /b 1
)

echo.
echo Package created: dist\EVChargingApp_Portable.zip
exit /b 0
