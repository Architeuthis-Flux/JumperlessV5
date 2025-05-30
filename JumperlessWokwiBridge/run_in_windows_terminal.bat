@echo off
:: Quick launcher for Jumperless in Windows Terminal

cd /d "%~dp0"

:: Check if Windows Terminal is available
where wt >nul 2>&1
if %errorlevel% neq 0 (
    echo Windows Terminal not found. Please install it from the Microsoft Store.
    echo Falling back to Command Prompt...
    goto :fallback
)

:: Determine what Jumperless file to run
if exist "Jumperless.exe" (
    echo Launching Jumperless.exe in Windows Terminal...
    start "" wt -p "Command Prompt" cmd /k "Jumperless.exe"
    exit /b 0
)

if exist "JumperlessWokwiBridge.exe" (
    echo Launching JumperlessWokwiBridge.exe in Windows Terminal...
    start "" wt -p "Command Prompt" cmd /k "JumperlessWokwiBridge.exe"
    exit /b 0
)

if exist "JumperlessWokwiBridge.py" (
    python --version >nul 2>&1
    if %errorlevel% equ 0 (
        echo Launching JumperlessWokwiBridge.py in Windows Terminal...
        start "" wt -p "Command Prompt" cmd /k "python JumperlessWokwiBridge.py"
        exit /b 0
    ) else (
        echo Python not found. Cannot run Python script.
        goto :error
    )
)

:error
echo No Jumperless application found!
echo Expected: Jumperless.exe, JumperlessWokwiBridge.exe, or JumperlessWokwiBridge.py
pause
exit /b 1

:fallback
if exist "Jumperless.exe" (
    start "" cmd /k "Jumperless.exe"
) else if exist "JumperlessWokwiBridge.exe" (
    start "" cmd /k "JumperlessWokwiBridge.exe"  
) else if exist "JumperlessWokwiBridge.py" (
    start "" cmd /k "python JumperlessWokwiBridge.py"
) else (
    goto :error
) 