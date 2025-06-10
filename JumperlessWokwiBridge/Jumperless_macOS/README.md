# Jumperless Bridge for macOS (Python Source)

## Quick Start
1. Extract this archive: `tar -xzf Jumperless_macOS.tar.gz`
2. Run the launcher: `./jumperless_launcher.sh`

## Launcher Options
Multiple launcher options are provided for different use cases:

### Option 1: Shell Script Launchers (Recommended for most users)
- `jumperless_launcher.sh` - Full-featured shell launcher with comprehensive process killing
- `jumperless_launcher` - Executable launcher (no extension) for double-clicking

### Option 2: Python Wrapper (For advanced users)
- `jumperless_launcher.py` - Cross-platform Python launcher with process killing

### Option 3: Manual Execution
- Run `python3 JumperlessWokwiBridge.py` directly after installing requirements

## Smart Process Management
All launchers include comprehensive process killing functionality:
- ✅ Automatically kills existing Jumperless instances before starting
- ✅ Closes Terminal.app and iTerm2 windows running Jumperless
- ✅ Uses AppleScript for intelligent window management
- ✅ Graceful process termination with force-kill fallback
- ✅ User-friendly messages and error handling

## Requirements
- Python 3.6 or higher
- pip (Python package installer)

## Installation Options
### Option 1: Official Python
Download from python.org (recommended for beginners)

### Option 2: Homebrew
```bash
brew install python3
```

### Option 3: System Python (macOS 12.3+)
Recent macOS versions include Python 3

## Manual Installation
If the launcher doesn't work, you can run manually:

1. Install dependencies:
   ```bash
   pip3 install -r requirements.txt
   ```

2. Run the application:
   ```bash
   python3 JumperlessWokwiBridge.py
   ```

## Files Included
- `JumperlessWokwiBridge.py` - Main application
- `requirements.txt` - Python dependencies
- `jumperless_launcher.sh` - macOS shell launcher script
- `jumperless_launcher` - Executable launcher (no extension)
- `jumperless_launcher.py` - Python wrapper launcher
- `README.md` - This file

## Compatibility
This package works on:
- macOS 10.13+ (Intel and Apple Silicon)
- Any macOS version with Python 3.6+

## Notes
- All launchers automatically install Python dependencies
- You may want to use a Python virtual environment
- Compatible with both Intel and Apple Silicon Macs
- Process killing ensures clean startup without conflicts
- Launchers detect and close existing Terminal/iTerm2 windows

## Troubleshooting
- If Python is not found: Install from python.org or use Homebrew
- For permission issues: Try `pip3 install --user -r requirements.txt`
- On older macOS: You may need to install Command Line Tools
- If launchers don't work: Try the Python wrapper or manual execution
- For double-click issues: Use the `jumperless_launcher` file (no extension)

## Support
Visit: https://github.com/Architeuthis-Flux/JumperlessV5
