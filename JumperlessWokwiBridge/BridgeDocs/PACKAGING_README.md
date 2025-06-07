# Jumperless Cross-Platform Packager

This directory contains packaging scripts for creating Jumperless installers on both macOS and Linux.

## Files Overview

- `Packager_clean.py` - New clean cross-platform packager
- `Packager.py` - Original packager (kept for reference)
- `jumperless_cli_launcher.sh` - Cross-platform launcher script

## Quick Start

### Run the Clean Packager
```bash
python Packager_clean.py
```

The packager will:
1. Detect your operating system automatically
2. Create the appropriate package format:
   - **macOS**: Creates a `.app` bundle and DMG installer
   - **Linux**: Creates an AppImage (portable executable)
   - **Windows**: Creates a single `Jumperless.exe` file

### GitHub Actions (Recommended)
For the most reliable builds, especially for Windows, use the GitHub Actions workflow:

1. **Push to GitHub** or create a **Pull Request**
2. **Packages built automatically** on native platforms
3. **Download from Actions tab** or **GitHub Releases**

See `BUILD_INSTRUCTIONS.md` for detailed GitHub Actions usage.

## Platform-Specific Details

### macOS Packaging
When run on macOS, the packager will:
- Use PyInstaller to create a universal2 binary (Intel + Apple Silicon)
- Create a proper `.app` bundle structure
- Include the launcher script that handles terminal window management
- Generate a DMG installer file
- Ask if you want to also create Linux and Windows packages

**Output files:**
- `Jumperless_Installer.dmg` - Main installer
- `dist/Jumperless.app` - App bundle
- Various support files in `JumperlessDMG/`

### Linux Packaging
When run on Linux (or when selected on macOS), the packager will:
- Use PyInstaller to create Linux executables for both x86_64 and ARM64 architectures
- Create AppImage structures for both architectures
- Download the AppImage build tools automatically
- Include the launcher script with Linux terminal support
- Generate either `.AppImage` files or `.tar.gz` fallbacks for each architecture

**Output files:**
- `Jumperless-x86_64.AppImage` - Portable Linux executable (Intel/AMD 64-bit)
- `Jumperless-aarch64.AppImage` - Portable Linux executable (ARM 64-bit)
- `JumperlessLinux_x64.AppDir/` - x86_64 AppImage source directory
- `JumperlessLinux_arm64.AppDir/` - ARM64 AppImage source directory
- `Jumperless-linux-x86_64.tar.gz` - x86_64 fallback if AppImage tools unavailable
- `Jumperless-linux-arm64.tar.gz` - ARM64 fallback if AppImage tools unavailable

**Supported ARM64 devices:**
- Raspberry Pi 4/5 (64-bit OS)
- ARM-based Linux laptops and servers
- Apple Silicon Macs running Linux
- Other aarch64 Linux systems

### Windows Packaging
When run on Windows (or when selected on macOS), the packager will:
- Use PyInstaller to create a single Windows x64 executable (`Jumperless.exe`)
- **No installation required** - users can run the .exe directly
- **GitHub Actions recommended** for reliable native Windows builds

**Output files:**
- **`Jumperless.exe`** - Single portable executable (~10MB)

**Windows features:**
- **Single file executable** - No installation required
- **Portable** - Run from any folder, no dependencies
- **Self-contained** - All libraries bundled inside
- **Process cleanup** - Built-in handling of existing instances  
- **Native Windows builds via GitHub Actions**

## Launcher Script Features

The `jumperless_cli_launcher.sh` script includes:
- **Cross-platform support** (macOS and Linux)
- **Process cleanup** - Kills any existing Jumperless instances
- **Terminal window management** - Closes old terminal windows
- **Large terminal windows** - Opens with generous screen real estate
- **Multiple terminal emulator support** on Linux

### Supported Linux Terminal Emulators
- gnome-terminal (GNOME)
- xterm (universal)
- konsole (KDE)
- xfce4-terminal (XFCE)
- terminator
- Fallback to current terminal if GUI unavailable

## Dependencies

### macOS
- Python 3.13+ with PyInstaller
- Xcode command line tools (for universal2 builds)

### Linux
- Python 3.x with PyInstaller
- wget (for downloading AppImage tools)
- Optional: xdotool, wmctrl (for better window management)

## Installation Methods

### AppImage (Linux x86_64)
```bash
# Make executable and run
chmod +x Jumperless-x86_64.AppImage
./Jumperless-x86_64.AppImage
```

### AppImage (Linux ARM64)
```bash
# Make executable and run
chmod +x Jumperless-aarch64.AppImage
./Jumperless-aarch64.AppImage
```

### DMG (macOS)
1. Double-click `Jumperless_Installer.dmg`
2. Drag Jumperless.app to Applications folder
3. Launch from Applications or double-click

### Windows
1. Download `Jumperless.exe`
2. **That's it!** - No installation needed
3. Run it from any folder

**Usage:**
```cmd
# Download and run directly
Jumperless.exe

# Or from any folder
C:\Downloads\Jumperless.exe
```

## Cross-Compilation Notes

**ARM64 Cross-Compilation:** When building on macOS/x86_64, the ARM64 Linux package is created using PyInstaller's cross-compilation features. However, this may have limitations:

- Some binary dependencies might not be available for ARM64
- Native compilation on an ARM64 system is recommended for best compatibility
- If cross-compilation fails, run the packager directly on an ARM64 Linux system

**For best ARM64 results:**
1. Run `python Packager_clean.py` on a Raspberry Pi or ARM64 Linux system
2. Or use an ARM64 Linux virtual machine/container
3. The x86_64 version should work reliably from any platform

**Windows Cross-Compilation:** When building on macOS/Linux, the Windows package is created using PyInstaller's cross-compilation features. However, this may have limitations:

- Some Windows-specific libraries might not be available during cross-compilation
- Native compilation on Windows is recommended for best compatibility
- If cross-compilation fails, run the packager directly on a Windows system

**For best Windows results:**
1. Run `python Packager_clean.py` on a native Windows system
2. Or use a Windows virtual machine
3. Cross-compilation from macOS generally works but may have library limitations

## Troubleshooting

### Linux AppImage Issues
If AppImage creation fails, the packager will create `.tar.gz` packages instead:

**x86_64:**
```bash
tar -xzf Jumperless-linux-x86_64.tar.gz
cd JumperlessLinux_x64.AppDir
./AppRun
```

**ARM64:**
```bash
tar -xzf Jumperless-linux-arm64.tar.gz
cd JumperlessLinux_arm64.AppDir
./AppRun
```

### Terminal Window Issues
If terminal windows don't close properly:
- The launcher includes multiple fallback methods
- Force quit terminals manually if needed
- Check that you have xdotool or wmctrl installed on Linux

### Permission Issues
Make sure scripts are executable:
```bash
chmod +x jumperless_cli_launcher.sh
chmod +x Packager_clean.py
```

### Windows ZIP Issues
If the Windows package doesn't work properly:

**Manual Installation:**
```cmd
# Extract the ZIP file
# Navigate to the extracted folder
cd JumperlessWindows
# Run the installer
install.bat
```

**Portable Usage:**
```cmd
# Extract the ZIP file  
# Navigate to the extracted folder
cd JumperlessWindows
# Run directly without installing
Jumperless.bat
```

## Development Notes

The packager creates different executable names to avoid conflicts:
- **macOS**: Main app renamed to `Jumperless_cli`, launcher becomes `Jumperless`
- **Linux x86_64**: Uses `JumperlessLinux_x64` as the PyInstaller target name
- **Linux ARM64**: Uses `JumperlessLinux_arm64` as the PyInstaller target name
- **Windows x64**: Uses `JumperlessWindows_x64` as the PyInstaller target name

This allows the launcher script to properly manage the actual application while providing a clean user interface.

**Architecture Support:**
- **macOS**: Universal2 binary (Intel + Apple Silicon)
- **Linux**: Separate packages for x86_64 and ARM64 architectures
- **Windows**: x64 (64-bit) executable
- **Cross-platform launchers**: Shell script (Unix) and batch file (Windows)

The multi-architecture approach ensures broad compatibility across different hardware platforms while maintaining optimal performance for each architecture.

**Package Types:**
- **macOS**: `.dmg` installer with `.app` bundle
- **Linux**: `.AppImage` portable executables or `.tar.gz` fallback
- **Windows**: `.zip` package with `.bat` installer and launcher

This comprehensive packaging system provides professional-grade distribution for all major desktop platforms. 