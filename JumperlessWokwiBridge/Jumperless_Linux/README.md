# Jumperless Bridge for Linux

## Quick Start
1. Extract this archive: `tar -xzf Jumperless_Linux.tar.gz`
2. Run the launcher: `./jumperless_launcher.sh`

## Launcher Options
Multiple launcher options are provided for different use cases:

### Option 1: Shell Script Launchers (Recommended for most users)
- `jumperless_launcher.sh` - Full-featured shell launcher
- `jumperless_launcher` - Executable launcher (no extension) for double-clicking

### Option 2: Python Wrapper (For advanced users)
- `jumperless_launcher.py` - Cross-platform Python launcher

### Option 3: Manual Execution
- Run `python3 JumperlessWokwiBridge.py` directly after installing requirements

## Simple Launcher Features
All launchers provide:
- ✅ Automatic Python dependency installation
- ✅ Clear feedback about terminal environment
- ✅ Graceful error handling and user-friendly messages

## Requirements
- Python 3.6 or higher
- pip (Python package installer)

## Optional Tools for Enhanced Terminal Management
- `xdotool` - For X11 window management (recommended)
- `wmctrl` - Alternative window management tool
- Most Linux distributions include these or they can be installed via package manager

## Manual Installation
If the launcher doesn't work, you can run manually:

1. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

2. Run the application:
   ```bash
   python3 JumperlessWokwiBridge.py
   ```

## Files Included
- `JumperlessWokwiBridge.py` - Main application
- `requirements.txt` - Python dependencies
- `jumperless_launcher.sh` - Linux shell launcher script
- `jumperless_launcher` - Executable launcher (no extension)
- `jumperless_launcher.py` - Python wrapper launcher
- `README.md` - This file

## Compatibility
This package works on all Linux architectures (x86_64, ARM64, etc.)
since it contains pure Python code.

## Notes
- All launchers automatically install Python dependencies
- You may want to use a Python virtual environment
- For system-wide installation, you may need sudo for pip install
- Process killing ensures clean startup without conflicts
- Terminal window management works best with xdotool installed

## Troubleshooting
- If Python is not found: Install `python3` via your package manager
- For permission issues: Try `pip install --user -r requirements.txt`
- If terminal windows don't close: Install `xdotool` or `wmctrl`
- If launchers don't work: Try the Python wrapper or manual execution
- For double-click issues: Use the `jumperless_launcher` file (no extension)
- On some distros: You may need to install `python3-pip` separately

## Support
Visit: https://github.com/Architeuthis-Flux/JumperlessV5
