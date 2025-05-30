@echo off
setlocal enabledelayedexpansion

:: Jumperless Terminal Launcher
:: This script allows you to run the Jumperless app in your preferred terminal emulator

echo.
echo ===============================================
echo    Jumperless Terminal Launcher
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
    echo Found Python script: JumperlessWokwiBridge.py
)

if exist "Jumperless.exe" (
    set "EXECUTABLE=Jumperless.exe"
    echo Found executable: Jumperless.exe
)

if exist "JumperlessWokwiBridge.exe" (
    set "EXECUTABLE=JumperlessWokwiBridge.exe"
    echo Found executable: JumperlessWokwiBridge.exe
)

:: Determine what to run
if defined EXECUTABLE (
    set "APP_CMD=%EXECUTABLE%"
    echo Will run: %EXECUTABLE%
) else if defined PYTHON_SCRIPT (
    :: Check if Python is available
    python --version >nul 2>&1
    if !errorlevel! equ 0 (
        set "APP_CMD=python %PYTHON_SCRIPT%"
        echo Will run: python %PYTHON_SCRIPT%
    ) else (
        echo ERROR: Python script found but Python is not installed or not in PATH
        echo Please install Python or use the executable version
        pause
        exit /b 1
    )
) else (
    echo ERROR: No Jumperless application found!
    echo Expected files: JumperlessWokwiBridge.py, Jumperless.exe, or JumperlessWokwiBridge.exe
    pause
    exit /b 1
)

echo.

:: Detect available terminal emulators
set "terminals_found=0"
set "terminal_list="

:: Check for Windows Terminal
where wt >nul 2>&1
if !errorlevel! equ 0 (
    set /a terminals_found+=1
    set "terminal_list=!terminal_list!!terminals_found!. Windows Terminal (wt)^

"
    set "terminal_!terminals_found!_name=Windows Terminal"
    set "terminal_!terminals_found!_cmd=wt"
)

:: Check for PowerShell 7+
where pwsh >nul 2>&1
if !errorlevel! equ 0 (
    set /a terminals_found+=1
    set "terminal_list=!terminal_list!!terminals_found!. PowerShell 7+ (pwsh)^

"
    set "terminal_!terminals_found!_name=PowerShell 7+"
    set "terminal_!terminals_found!_cmd=pwsh"
)

:: Check for Windows PowerShell
where powershell >nul 2>&1
if !errorlevel! equ 0 (
    set /a terminals_found+=1
    set "terminal_list=!terminal_list!!terminals_found!. Windows PowerShell^

"
    set "terminal_!terminals_found!_name=Windows PowerShell"
    set "terminal_!terminals_found!_cmd=powershell"
)

:: Check for Git Bash (common locations)
if exist "C:\Program Files\Git\bin\bash.exe" (
    set /a terminals_found+=1
    set "terminal_list=!terminal_list!!terminals_found!. Git Bash^

"
    set "terminal_!terminals_found!_name=Git Bash"
    set "terminal_!terminals_found!_cmd=C:\Program Files\Git\bin\bash.exe"
)

if exist "C:\Program Files (x86)\Git\bin\bash.exe" (
    set /a terminals_found+=1
    set "terminal_list=!terminal_list!!terminals_found!. Git Bash (x86)^

"
    set "terminal_!terminals_found!_name=Git Bash (x86)"
    set "terminal_!terminals_found!_cmd=C:\Program Files (x86)\Git\bin\bash.exe"
)

:: Always include Command Prompt as an option
set /a terminals_found+=1
set "terminal_list=!terminal_list!!terminals_found!. Command Prompt (default)^

"
set "terminal_!terminals_found!_name=Command Prompt"
set "terminal_!terminals_found!_cmd=cmd"

:: Show available terminals
echo Available terminal emulators:
echo !terminal_list!

:: Get user choice
:choose_terminal
set /p "choice=Choose terminal (1-!terminals_found!) or press ENTER for auto-detect: "

if "!choice!"=="" (
    :: Auto-detect best terminal
    echo Auto-detecting best terminal...
    where wt >nul 2>&1
    if !errorlevel! equ 0 (
        echo Using Windows Terminal (auto-detected)
        set "chosen_cmd=wt"
        set "chosen_name=Windows Terminal"
        goto :run_app
    )
    
    where pwsh >nul 2>&1
    if !errorlevel! equ 0 (
        echo Using PowerShell 7+ (auto-detected)
        set "chosen_cmd=pwsh"
        set "chosen_name=PowerShell 7+"
        goto :run_app
    )
    
    echo Using Command Prompt (fallback)
    set "chosen_cmd=cmd"
    set "chosen_name=Command Prompt"
    goto :run_app
)

:: Validate choice
if !choice! lss 1 goto :invalid_choice
if !choice! gtr !terminals_found! goto :invalid_choice

:: Set chosen terminal
set "chosen_cmd=!terminal_%choice%_cmd!"
set "chosen_name=!terminal_%choice%_name!"
goto :run_app

:invalid_choice
echo Invalid choice. Please enter a number between 1 and !terminals_found!
goto :choose_terminal

:run_app
echo.
echo Starting Jumperless in !chosen_name!...
echo.

:: Launch based on terminal type
if "!chosen_cmd!"=="wt" (
    :: Windows Terminal
    start "" wt -p "Command Prompt" cmd /k "!APP_CMD!"
) else if "!chosen_cmd!"=="pwsh" (
    :: PowerShell 7+
    start "" pwsh -NoExit -Command "& { !APP_CMD! }"
) else if "!chosen_cmd!"=="powershell" (
    :: Windows PowerShell
    start "" powershell -NoExit -Command "& { !APP_CMD! }"
) else if "!chosen_cmd!"=="cmd" (
    :: Command Prompt
    start "" cmd /k "!APP_CMD!"
) else if "!chosen_cmd:~-8!"=="bash.exe" (
    :: Git Bash
    start "" "!chosen_cmd!" -c "cd '!CD!' && !APP_CMD!; read -p 'Press Enter to exit...'"
) else (
    :: Generic fallback
    start "" "!chosen_cmd!" /k "!APP_CMD!"
)

echo.
echo Jumperless should now be running in !chosen_name!
echo You can close this window.
echo.

:: Wait a moment then exit
timeout /t 3 /nobreak >nul
exit /b 0 