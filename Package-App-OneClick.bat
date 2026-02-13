@echo off
setlocal EnableExtensions EnableDelayedExpansion
cd /d "%~dp0"
call scripts\Create-Portable-Package.bat
endlocal
