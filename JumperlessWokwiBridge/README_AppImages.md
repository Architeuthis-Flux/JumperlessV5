# Jumperless AppImages

## Quick Start

### Option 1: Use the Wrapper Script (Recommended)
1. Double-click `run_jumperless_appimage.sh` 
2. The script will automatically detect your architecture and open Jumperless in a new terminal

### Option 2: Use the Desktop File
1. Double-click `Jumperless.desktop`
2. This provides a more polished experience with an icon

### Option 3: Run Directly in Terminal
1. Open a terminal in this folder
2. Run: `./Jumperless-aarch64.AppImage` (for ARM64) or `./Jumperless-x86_64.AppImage` (for x86_64)

## Files Included

- **`Jumperless-aarch64.AppImage`** - AppImage for ARM64 systems (Raspberry Pi 4+, ARM servers, Apple Silicon with Linux)
- **`Jumperless-x86_64.AppImage`** - AppImage for x86_64 systems (Most Linux PCs and laptops)
- **`run_jumperless_appimage.sh`** - Smart launcher script that detects your architecture
- **`Jumperless.desktop`** - Desktop entry for GUI file managers
- **`icon.png`** - Application icon
- **`README_AppImages.md`** - This file

## Features

### Smart Launcher Script
- ✅ Automatically detects your system architecture (ARM64 vs x86_64)
- ✅ Chooses the correct AppImage for your system
- ✅ Opens Jumperless in a new terminal window with proper sizing
- ✅ Works with various terminal emulators (gnome-terminal, konsole, xterm, etc.)
- ✅ Provides clear error messages and status updates
- ✅ Shows terminal environment information for debugging

### AppImage Benefits
- ✅ **No installation required** - just download and run
- ✅ **Self-contained** - includes all dependencies
- ✅ **Portable** - works on any Linux distribution
- ✅ **Safe** - runs in a sandboxed environment

## System Requirements

- **Linux** (any distribution)
- **Architecture**: ARM64 (aarch64) or x86_64
- **Terminal emulator** (for best experience)

## Troubleshooting

### AppImage won't run when double-clicked
- Use the wrapper script `run_jumperless_appimage.sh` instead
- Or run directly in a terminal: `./Jumperless-[arch].AppImage`

### Permission denied error
```bash
chmod +x Jumperless-*.AppImage
chmod +x run_jumperless_appimage.sh
```

### Wrong architecture
The launcher script automatically detects your architecture, but if you need to check manually:
```bash
uname -m
# aarch64 or arm64 = use Jumperless-aarch64.AppImage
# x86_64 = use Jumperless-x86_64.AppImage
```

### No terminal emulator found
The script will fall back to running in the current terminal if no GUI terminal emulator is found.

## About Jumperless

Jumperless is an electronics prototyping tool that bridges Wokwi simulations with physical hardware, allowing you to test your Arduino code on real components without manually wiring breadboards.

- **GitHub**: https://github.com/Architeuthis-Flux/JumperlessV5
- **Documentation**: Check the main repository for detailed documentation
- **Support**: Open issues on the GitHub repository

## Technical Details

- **AppImage Format**: Portable Linux application format
- **Base**: Built with PyInstaller from Python source
- **Size**: ~9.2 MB per AppImage
- **Runtime**: Includes Python runtime and all dependencies 