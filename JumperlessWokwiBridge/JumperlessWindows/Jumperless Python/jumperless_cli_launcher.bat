@echo off
setlocal enabledelayedexpansion

:: Jumperless CLI Launcher
:: Run this script directly in your terminal emulator (Tabby, Hyper, ConEmu, etc.)

echo.
echo ===============================================
echo         Jumperless CLI Launcher
echo ===============================================
echo.

:: Change to the script directory
cd /d "%~dp0"

:: Check what Jumperless files are available
set "PYTHON_SCRIPT="
set "EXECUTABLE="
set "APP_CMD="

if exist "JumperlessWokwiBridge.py" (
    set "PYTHON_SCRIPT=JumperlessWokwiBridge.py"
    echo [✓] Found Python script: JumperlessWokwiBridge.py
)

if exist "Jumperless.exe" (
    set "EXECUTABLE=Jumperless.exe"
    echo [✓] Found executable: Jumperless.exe
)

if exist "JumperlessWokwiBridge.exe" (
    set "EXECUTABLE=JumperlessWokwiBridge.exe"
    echo [✓] Found executable: JumperlessWokwiBridge.exe
)

:: Determine what to run (prefer executable over Python script)
if defined EXECUTABLE (
    set "APP_CMD=%EXECUTABLE%"
    echo [→] Will run: %EXECUTABLE%
) else if defined PYTHON_SCRIPT (
    :: Check if Python is available
    python --version >nul 2>&1
    if !errorlevel! equ 0 (
        set "APP_CMD=python %PYTHON_SCRIPT%"
        echo [→] Will run: python %PYTHON_SCRIPT%
    ) else (
        echo [✗] ERROR: Python script found but Python is not installed or not in PATH
        echo [i] Please install Python or use the executable version
        echo.
        pause
        exit /b 1
    )
) else (
    echo [✗] ERROR: No Jumperless application found!
    echo [i] Expected files: JumperlessWokwiBridge.py, Jumperless.exe, or JumperlessWokwiBridge.exe
    echo [i] Make sure this script is in the same directory as your Jumperless files
    echo.
    pause
    exit /b 1
)

echo.

:: Detect current terminal environment for better user experience
set "TERMINAL_INFO=Unknown terminal"

if defined WT_SESSION (
    set "TERMINAL_INFO=Windows Terminal"
) else if defined TABBY_CONFIG_DIRECTORY (
    set "TERMINAL_INFO=Tabby Terminal"
) else if defined HYPER_VERSION (
    set "TERMINAL_INFO=Hyper Terminal"
) else if defined ConEmuPID (
    set "TERMINAL_INFO=ConEmu"
) else if defined ANSICON (
    set "TERMINAL_INFO=ANSICON-enabled terminal"
) else if defined TERM (
    set "TERMINAL_INFO=UNIX-style terminal (%TERM%)"
)

echo [i] Running in: %TERMINAL_INFO%
echo [i] Starting Jumperless...
echo.
echo ===============================================
echo.

:: Run the application directly in current terminal
%APP_CMD%

:: Handle exit
echo.
echo ===============================================
echo [i] Jumperless has exited
echo =============================================== 