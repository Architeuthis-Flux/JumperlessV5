#!/bin/bash
# Jumperless macOS Launcher
# Auto-generated launcher script with comprehensive process killing

# Function to kill existing instances
kill_existing_instances() {
    echo "Checking for existing Jumperless instances..."
    
    # Close Terminal.app windows running Jumperless on macOS
    osascript -e 'tell application "Terminal"
        repeat with w in windows
            try
                set tabProcesses to processes of tabs of w
                repeat with tabProc in tabProcesses
                    repeat with proc in tabProc
                        if proc contains "Jumperless" or proc contains "jumperless" then
                            close w
                            exit repeat
                        end if
                    end repeat
                end repeat
            on error
                -- Ignore errors (window might already be closed or no processes)
            end try
        end repeat
    end tell' 2>/dev/null
    
    # Also try to close iTerm2 windows if it's being used
    osascript -e 'tell application "iTerm2"
        repeat with w in windows
            try
                repeat with t in tabs of w
                    set sessionName to name of current session of t
                    if sessionName contains "Jumperless" or sessionName contains "jumperless" then
                        close w
                        exit repeat
                    end if
                end repeat
            on error
                -- Ignore errors
            end try
        end repeat
    end tell' 2>/dev/null
    
    # Give windows time to close
    sleep 1
    
    # Kill any existing instances of jumperless_cli, Jumperless_cli, or related processes
    pkill -f "Jumperless_cli" 2>/dev/null
    pkill -f "jumperless_cli" 2>/dev/null
    pkill -f "JumperlessWokwiBridge" 2>/dev/null
    pkill -f "Jumperless.app" 2>/dev/null
    
    # Give processes time to terminate gracefully
    sleep 1
    
    # Force kill if still running
    pkill -9 -f "Jumperless_cli" 2>/dev/null
    pkill -9 -f "jumperless_cli" 2>/dev/null
    pkill -9 -f "JumperlessWokwiBridge" 2>/dev/null
    pkill -9 -f "Jumperless.app" 2>/dev/null
    
    echo "Cleared existing instances and terminal windows."
}

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Kill existing instances first
kill_existing_instances

echo ""
echo "==============================================="
echo "       Jumperless macOS Launcher"
echo "==============================================="
echo ""

# Check if Python 3 is available
if command -v python3 &> /dev/null; then
    PYTHON_CMD="python3"
elif command -v python &> /dev/null; then
    PYTHON_CMD="python"
else
    echo "❌ Error: Python 3 is not installed"
    echo ""
    echo "Please install Python 3:"
    echo "  • Download from python.org, or"
    echo "  • Install via Homebrew: brew install python3, or"
    echo "  • Use the system Python 3 (if available)"
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

echo "✅ Python found: $PYTHON_CMD"

# Check if pip is available
if ! $PYTHON_CMD -m pip --version &> /dev/null; then
    echo "❌ Error: pip is not available"
    echo "Please install pip or reinstall Python with pip included"
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

echo "✅ pip is available"

# Install requirements if requirements.txt exists
if [ -f "$SCRIPT_DIR/requirements.txt" ]; then
    echo "📦 Installing Python dependencies..."
    $PYTHON_CMD -m pip install -r "$SCRIPT_DIR/requirements.txt"
    if [ $? -ne 0 ]; then
        echo ""
        echo "⚠️  Warning: Failed to install some dependencies"
        echo "You may need to run: pip install -r requirements.txt"
        echo "Or use a virtual environment"
        echo ""
        read -p "Press Enter to continue anyway..."
    else
        echo "✅ Dependencies installed successfully"
    fi
fi

# Run the main application
echo ""
echo "==============================================="
echo "🚀 Starting Jumperless Bridge..."
echo "==============================================="
echo ""
cd "$SCRIPT_DIR"
exec $PYTHON_CMD JumperlessWokwiBridge.py "$@"
