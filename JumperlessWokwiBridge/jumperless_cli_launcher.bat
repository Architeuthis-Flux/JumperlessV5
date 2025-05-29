@echo off
REM Cross-platform launcher for Jumperless CLI
REM Windows version

REM Function to kill existing instances
echo Checking for existing Jumperless instances...

REM Kill any existing instances of Jumperless processes
taskkill /F /IM "Jumperless_cli.exe" >NUL 2>&1
taskkill /F /IM "jumperless_cli.exe" >NUL 2>&1
taskkill /F /IM "JumperlessWokwiBridge.exe" >NUL 2>&1
taskkill /F /IM "Jumperless.exe" >NUL 2>&1

REM Give processes time to terminate
timeout /t 1 /nobreak >NUL

echo Cleared existing instances.

REM Get the directory where this batch file is located
set "HERE=%~dp0"

REM Launch Jumperless with larger window
echo Launching Jumperless on Windows...

REM Try to find the executable in common locations
if exist "%HERE%Jumperless_cli.exe" (
    start "Jumperless" cmd /c "mode con: cols=120 lines=40 && "%HERE%Jumperless_cli.exe" && pause"
) else if exist "%HERE%JumperlessWindows_x64.exe" (
    start "Jumperless" cmd /c "mode con: cols=120 lines=40 && "%HERE%JumperlessWindows_x64.exe" && pause"
) else if exist "%HERE%Jumperless.exe" (
    start "Jumperless" cmd /c "mode con: cols=120 lines=40 && "%HERE%Jumperless.exe" && pause"
) else (
    echo Error: Jumperless executable not found
    echo Please ensure Jumperless is installed or run this script from the Jumperless directory
    pause
    exit /b 1
)

echo Jumperless launched successfully! 