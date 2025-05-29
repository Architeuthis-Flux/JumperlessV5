#!/bin/bash
# Cross-platform launcher for Jumperless CLI
# Supports both macOS and Linux

# Function to kill existing instances
kill_existing_instances() {
    echo "Checking for existing Jumperless instances..."
    
    # Detect OS for platform-specific terminal window closing
    OS=$(detect_os)
    
    # Close terminal windows first (before killing processes)
    if [ "$OS" = "macOS" ]; then
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
        
    elif [ "$OS" = "Linux" ]; then
        # Close terminal windows on Linux by finding window IDs
        if command -v xdotool >/dev/null 2>&1; then
            # Use xdotool to find and close windows with Jumperless in title or command
            xdotool search --name "Jumperless" windowclose 2>/dev/null || true
            xdotool search --name "jumperless" windowclose 2>/dev/null || true
        fi
        
        # Try to close specific terminal emulator windows
        if command -v wmctrl >/dev/null 2>&1; then
            wmctrl -c "Jumperless" 2>/dev/null || true
            wmctrl -c "jumperless" 2>/dev/null || true
        fi
        
        # For GNOME, try to close terminals by searching processes
        if command -v gdbus >/dev/null 2>&1; then
            # Get list of gnome-terminal windows and close ones running Jumperless
            for pid in $(pgrep -f "gnome-terminal"); do
                if ps -p $pid -o args= | grep -q -i jumperless 2>/dev/null; then
                    kill $pid 2>/dev/null || true
                fi
            done
        fi
    fi
    
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

# Detect operating system
detect_os() {
    case "$(uname -s)" in
        Darwin*)
            echo "macOS"
            ;;
        Linux*)
            echo "Linux"
            ;;
        *)
            echo "Unknown"
            ;;
    esac
}

# Main execution
main() {
    OS=$(detect_os)
    echo "Detected OS: $OS"
    
    # Kill existing instances first
    kill_existing_instances
    
    case $OS in
        "macOS")
            echo "Launching Jumperless on macOS..."
            # Original macOS launch command with larger window size
            if [ -f "/Applications/Jumperless.app/Contents/MacOS/Jumperless_cli" ]; then
                # Use osascript to open Terminal with specific size and run the command
                osascript -e 'tell application "Terminal"
                    activate
                    set newWindow to do script ""
                    delay 0.5
                    do script "cd /Applications/Jumperless.app/Contents/MacOS && ./Jumperless_cli" in newWindow
                    set bounds of front window to {100, 100, 1000, 700}
                end tell'
            else
                echo "Error: Jumperless.app not found in Applications folder"
                exit 1
            fi
            ;;
        "Linux")
            echo "Launching Jumperless on Linux..."
            # Try different possible locations for Linux installation with larger terminal
            JUMPERLESS_EXEC=""
            if [ -f "./Jumperless_cli" ]; then
                JUMPERLESS_EXEC="./Jumperless_cli"
            elif [ -f "./Jumperless" ]; then
                JUMPERLESS_EXEC="./Jumperless"
            elif [ -f "/usr/local/bin/Jumperless_cli" ]; then
                JUMPERLESS_EXEC="/usr/local/bin/Jumperless_cli"
            elif [ -f "/opt/Jumperless/Jumperless_cli" ]; then
                JUMPERLESS_EXEC="/opt/Jumperless/Jumperless_cli"
            elif [ -f "$(dirname "$0")/Jumperless_cli" ]; then
                JUMPERLESS_EXEC="$(dirname "$0")/Jumperless_cli"
            fi
            
            if [ -n "$JUMPERLESS_EXEC" ]; then
                # Try different terminal emulators with larger window size
                if command -v gnome-terminal >/dev/null 2>&1; then
                    gnome-terminal --geometry=120x40 -- "$JUMPERLESS_EXEC"
                elif command -v xterm >/dev/null 2>&1; then
                    xterm -geometry 120x40 -e "$JUMPERLESS_EXEC"
                elif command -v konsole >/dev/null 2>&1; then
                    konsole --geometry 120x40 -e "$JUMPERLESS_EXEC"
                elif command -v xfce4-terminal >/dev/null 2>&1; then
                    xfce4-terminal --geometry=120x40 -e "$JUMPERLESS_EXEC"
                elif command -v terminator >/dev/null 2>&1; then
                    terminator --geometry=960x600 -e "$JUMPERLESS_EXEC"
                else
                    # Fallback: run directly in current terminal
                    echo "No suitable terminal emulator found, running in current terminal..."
                    exec "$JUMPERLESS_EXEC"
                fi
            else
                echo "Error: Jumperless executable not found"
                echo "Please ensure Jumperless is installed or run this script from the Jumperless directory"
                exit 1
            fi
            ;;
        *)
            echo "Error: Unsupported operating system: $OS"
            echo "This script supports macOS and Linux only"
            exit 1
            ;;
    esac
}

# Run main function
main "$@"

# Legacy comments for reference:
# This was originally the launcher for OSX, this way the app will be opened
# when you double click it from the apps folder

# This should be called jumperlesswokwibridge in the contents folder of the app
# and the real app should be renamed to jumperlesswokwibridge_cli
# It's a hack to allow you to both have a .app file and actually run it in a persistent Terminal