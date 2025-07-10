#!/bin/bash
# Cross-platform launcher for Jumperless CLI
# Simple version focused on stability - no process killing

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
    echo "Launching Jumperless CLI on $OS..."
    
    case $OS in
        "macOS")
            # macOS launch with terminal resizing
            if [ -f "/Applications/Jumperless.app/Contents/MacOS/Jumperless_cli" ]; then
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
            # Simple Linux launch - find executable and run it
            JUMPERLESS_EXEC=""
            
            # Check common locations
            for path in "./Jumperless_cli" "./Jumperless" "/usr/local/bin/Jumperless_cli" "/opt/Jumperless/Jumperless_cli" "$(dirname "$0")/Jumperless_cli"; do
                if [ -f "$path" ]; then
                    JUMPERLESS_EXEC="$path"
                    break
                fi
            done
            
            if [ -n "$JUMPERLESS_EXEC" ]; then
                # Try to launch in a new terminal with larger window size
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
                    # Run in current terminal if no terminal emulator found
                    echo "No suitable terminal emulator found, running in current terminal..."
                    exec "$JUMPERLESS_EXEC"
                fi
            else
                echo "Error: Jumperless executable not found"
                echo "Searched locations:"
                echo "  ./Jumperless_cli"
                echo "  ./Jumperless"
                echo "  /usr/local/bin/Jumperless_cli"
                echo "  /opt/Jumperless/Jumperless_cli"
                echo "  $(dirname "$0")/Jumperless_cli"
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