#!/bin/bash

# Jumperless CLI Launcher
# Run this script directly in your terminal emulator (any Unix shell, WSL, etc.)

echo
echo "==============================================="
echo "         Jumperless CLI Launcher"
echo "==============================================="
echo

# Change to the script directory
cd "$(dirname "$0")"

# Check what Jumperless files are available
PYTHON_SCRIPT=""
EXECUTABLE=""
APP_CMD=""

if [ -f "JumperlessWokwiBridge.py" ]; then
    PYTHON_SCRIPT="JumperlessWokwiBridge.py"
    echo "[✓] Found Python script: JumperlessWokwiBridge.py"
fi

if [ -f "Jumperless" ]; then
    EXECUTABLE="./Jumperless"
    echo "[✓] Found executable: Jumperless"
fi

if [ -f "JumperlessWokwiBridge" ]; then
    EXECUTABLE="./JumperlessWokwiBridge"
    echo "[✓] Found executable: JumperlessWokwiBridge"
fi

# Determine what to run (prefer executable over Python script)
if [ -n "$EXECUTABLE" ]; then
    # Make sure executable has execute permissions
    chmod +x "$EXECUTABLE" 2>/dev/null
    APP_CMD="$EXECUTABLE"
    echo "[→] Will run: $EXECUTABLE"
elif [ -n "$PYTHON_SCRIPT" ]; then
    # Check if Python is available
    if command -v python3 >/dev/null 2>&1; then
        APP_CMD="python3 $PYTHON_SCRIPT"
        echo "[→] Will run: python3 $PYTHON_SCRIPT"
    elif command -v python >/dev/null 2>&1; then
        APP_CMD="python $PYTHON_SCRIPT"
        echo "[→] Will run: python $PYTHON_SCRIPT"
    else
        echo "[✗] ERROR: Python script found but Python is not installed or not in PATH"
        echo "[i] Please install Python or use the executable version"
        echo
        read -p "Press Enter to exit..."
        exit 1
    fi
else
    echo "[✗] ERROR: No Jumperless application found!"
    echo "[i] Expected files: JumperlessWokwiBridge.py, Jumperless, or JumperlessWokwiBridge"
    echo "[i] Make sure this script is in the same directory as your Jumperless files"
    echo
    read -p "Press Enter to exit..."
    exit 1
fi

echo

# Detect current terminal environment for better user experience
TERMINAL_INFO="Unknown terminal"

if [ -n "$WT_SESSION" ]; then
    TERMINAL_INFO="Windows Terminal (WSL)"
elif [ -n "$TABBY_CONFIG_DIRECTORY" ]; then
    TERMINAL_INFO="Tabby Terminal"
elif [ -n "$HYPER_VERSION" ]; then
    TERMINAL_INFO="Hyper Terminal"
elif [ -n "$TERM_PROGRAM" ]; then
    TERMINAL_INFO="$TERM_PROGRAM"
elif [ -n "$TERM" ]; then
    TERMINAL_INFO="Terminal ($TERM)"
fi

echo "[i] Running in: $TERMINAL_INFO"
echo "[i] Starting Jumperless..."
echo
echo "==============================================="
echo

# Run the application directly in current terminal
eval $APP_CMD

# Handle exit
echo
echo "==============================================="
echo "[i] Jumperless has exited"
echo "===============================================" 