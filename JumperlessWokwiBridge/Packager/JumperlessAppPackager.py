#!/usr/bin/env python3
"""
JumperlessAppPackager.py - Universal Multi-Platform Packager for Jumperless Bridge
Creates AppImages, executables, and portable packages for Linux, Windows, and macOS
"""

import os
import sys
import shutil
import pathlib
import subprocess
import time
import zipfile
import tarfile
import json
from pathlib import Path

def cleanup_intermediate_files():
    """Clean up intermediate files created during packaging"""
    print("🧹 Cleaning up intermediate files...")
    
    # List of files and directories to clean up
    cleanup_patterns = [
        "dist/",
        "build/", 
        "*.spec",
        "*.AppDir/",
        "__pycache__/",
        "*.pyc",
        "*.pyo",
        ".pytest_cache/",
        "*.egg-info/",
        ".DS_Store",
        "Thumbs.db",
        "desktop.ini",
        "Jumperless_Linux/",
        "Jumperless_Windows/",
        "Jumperless_macOS/",
        "tools/appimagetool-*",
        "JumperlessFiles/",
        "venv/",
        ".venv/",
        "env/",
        ".env/",
        ".coverage",
        "htmlcov/",
        ".mypy_cache/",
        ".tox/",
        "*.log",
        "*.tmp",
        "*.bak",
        "*.swp",
        "*.swo",
        "*~",
        ".*.swp",
        ".*.swo"
    ]
    
    # Clean up files and directories
    for pattern in cleanup_patterns:
        if "/" in pattern:
            # Directory cleanup
            dir_path = pattern.rstrip("/")
            if os.path.exists(dir_path):
                try:
                    shutil.rmtree(dir_path)
                    print(f"  🗑️  Removed directory: {dir_path}")
                except Exception as e:
                    print(f"  ⚠️  Could not remove {dir_path}: {e}")
        else:
            # File pattern cleanup
            import glob
            for file_path in glob.glob(pattern):
                try:
                    if os.path.isfile(file_path):
                        os.remove(file_path)
                        print(f"  🗑️  Removed file: {file_path}")
                    elif os.path.isdir(file_path):
                        shutil.rmtree(file_path)
                        print(f"  🗑️  Removed directory: {file_path}")
                except Exception as e:
                    print(f"  ⚠️  Could not remove {file_path}: {e}")
    
    print("✅ Cleanup complete!")

def package_linux_appimage():
    """Package for Linux as AppImage"""
    print("=== Packaging Linux AppImage ===")
    
    # Ensure output directory exists
    appimage_dir = pathlib.Path("builds/linux/appimage")
    appimage_dir.mkdir(parents=True, exist_ok=True)
    
    # Clean up old AppImages to avoid confusion
    print("🧹 Cleaning up old AppImages...")
    old_appimages = list(appimage_dir.glob("Jumperless-*.AppImage"))
    for old_appimage in old_appimages:
        try:
            old_appimage.unlink()
            print(f"  🗑️  Removed old AppImage: {old_appimage.name}")
        except Exception as e:
            print(f"  ⚠️  Could not remove {old_appimage.name}: {e}")
    
    if old_appimages:
        print(f"✅ Cleaned up {len(old_appimages)} old AppImage(s)")
    else:
        print("✅ No old AppImages to clean up")
    
    # Architecture configurations
    architectures = [
        ("x86_64", "dist/JumperlessWokwiBridge_x86_64", "JumperlessWokwiBridge.AppDir", "Jumperless-x86_64.AppImage", "JumperlessWokwiBridge"),
        ("aarch64", "dist/JumperlessWokwiBridge_aarch64", "JumperlessWokwiBridge_aarch64.AppDir", "Jumperless-aarch64.AppImage", "JumperlessWokwiBridge")
    ]
    
    success_count = 0
    arm64_success = False
    
    for arch_name, dist_path, appdir_path, appimage_path, executable_name in architectures:
        print(f"\n📦 Creating {arch_name} AppImage...")
        
        # Create PyInstaller executable for this architecture
        pyinstaller_cmd = [
            sys.executable, "-m", "PyInstaller", 
            "--clean",  # Clear cache for each build
            "--onefile", 
            "--console", 
            "--name", executable_name,
            "--distpath", dist_path,
            "--specpath", f"build/spec_{arch_name}",  # Unique spec path per architecture
            "JumperlessWokwiBridge.py"
        ]
        
        print(f"Running PyInstaller for {arch_name}...")
        result = subprocess.run(pyinstaller_cmd)
        
        if result.returncode != 0:
            print(f"❌ PyInstaller failed for {arch_name}")
            continue
        
        # Create AppImage
        if create_appimage_for_arch(arch_name, dist_path, appdir_path, appimage_path, executable_name):
            success_count += 1
            if arch_name == "aarch64":
                arm64_success = True
    
    # Create launcher scripts for AppImage directory
    create_appimage_launchers(appimage_dir)
    
    # Copy installation script to AppImage directory
    try:
        install_script_source = pathlib.Path("builds/linux/appimage/install_jumperless.sh")
        if install_script_source.exists():
            print("✅ Installation script ready")
        else:
            print("⚠️  Installation script not found")
    except Exception as e:
        print(f"⚠️  Could not check installation script: {e}")
    
    print(f"\n🎉 Linux AppImage packaging complete! Created {success_count} architecture packages.")
    
    # Create tar.gz archive
    create_distribution_archive(appimage_dir)
    
    # Clean up intermediate files
    cleanup_intermediate_files()
    
    return success_count > 0

def create_appimage_for_arch(arch_name, dist_path, appdir_path, appimage_path, executable_name):
    """Create AppImage for specific architecture"""
    try:
        # Create AppDir structure
        appdir = pathlib.Path(appdir_path)
        if appdir.exists():
            shutil.rmtree(appdir)
        appdir.mkdir(parents=True, exist_ok=True)
        
        # Copy executable
        executable_source = pathlib.Path(dist_path) / executable_name
        executable_dest = appdir / "AppRun"
        
        if not executable_source.exists():
            print(f"❌ Executable not found: {executable_source}")
            return False
        
        shutil.copy2(executable_source, executable_dest)
        os.chmod(executable_dest, 0o755)
        
        # Create desktop file
        desktop_content = '''[Desktop Entry]
Type=Application
Name=Jumperless Wokwi Bridge
Exec=AppRun
Icon=icon
Comment=Jumperless Wokwi Bridge - Electronics Prototyping Tool
Categories=Development;Electronics;Education;
Terminal=true
StartupNotify=false
'''
        
        desktop_file = appdir / "Jumperless.desktop"
        with open(desktop_file, 'w') as f:
            f.write(desktop_content)
        
        # Copy icon if available
        icon_source = pathlib.Path("assets/icons/icon.png")
        if icon_source.exists():
            shutil.copy2(icon_source, appdir / "icon.png")
        
        # Download appimagetool if needed
        appimagetool_path = download_appimagetool()
        if not appimagetool_path:
            print("❌ Could not download appimagetool")
            return False
        
        # Create AppImage
        appimage_output = pathlib.Path(f"builds/linux/appimage/{appimage_path}")
        appimage_cmd = [str(appimagetool_path), str(appdir), str(appimage_output)]
        
        print(f"Creating AppImage: {appimage_output}")
        result = subprocess.run(appimage_cmd, capture_output=True, text=True)
        
        if result.returncode == 0 and appimage_output.exists():
            # Make executable
            os.chmod(appimage_output, 0o755)
            
            # Get file size
            appimage_size = appimage_output.stat().st_size / (1024*1024)
            print(f"✅ Created {arch_name} AppImage: {appimage_output} ({appimage_size:.1f} MB)")
            
            # Clean up AppDir
            if appdir.exists():
                shutil.rmtree(appdir)
            
            return True
        else:
            print(f"❌ AppImage creation failed for {arch_name}")
            print(f"Error: {result.stderr}")
            return False
            
    except Exception as e:
        print(f"❌ Error creating AppImage for {arch_name}: {e}")
        return False

def download_appimagetool():
    """Download appimagetool if not already available"""
    tools_dir = pathlib.Path("tools")
    tools_dir.mkdir(exist_ok=True)
    
    # Check current architecture
    arch = os.uname().machine
    if arch == "x86_64":
        appimagetool_name = "appimagetool-x86_64.AppImage"
    elif arch in ["aarch64", "arm64"]:
        appimagetool_name = "appimagetool-aarch64.AppImage"
    else:
        print(f"❌ Unsupported architecture for appimagetool: {arch}")
        return None
    
    appimagetool_path = tools_dir / appimagetool_name
    
    if appimagetool_path.exists():
        os.chmod(appimagetool_path, 0o755)
        return appimagetool_path
    
    print(f"📥 Downloading {appimagetool_name}...")
    download_url = f"https://github.com/AppImage/AppImageKit/releases/download/continuous/{appimagetool_name}"
    
    try:
        import urllib.request
        urllib.request.urlretrieve(download_url, appimagetool_path)
        os.chmod(appimagetool_path, 0o755)
        print(f"✅ Downloaded {appimagetool_name}")
        return appimagetool_path
    except Exception as e:
        print(f"❌ Failed to download appimagetool: {e}")
        return None

def create_appimage_launchers(appimage_dir):
    """Create launcher scripts for AppImage directory"""
    
    # Smart launcher script
    smart_launcher = '''#!/bin/bash

# Simple Jumperless AppImage Launcher
# Detects architecture and runs the correct AppImage

echo "Starting Jumperless..."

# Detect architecture
ARCH=$(uname -m)
echo "Architecture: $ARCH"

# Set AppImage name based on architecture
case "$ARCH" in
    x86_64)
        APPIMAGE="Jumperless-x86_64.AppImage"
        ;;
    aarch64|arm64)
        APPIMAGE="Jumperless-aarch64.AppImage"
        ;;
    *)
        echo "Unsupported architecture: $ARCH"
        echo "Supported: x86_64, aarch64/arm64"
        echo "Trying x86_64 AppImage..."
        APPIMAGE="Jumperless-x86_64.AppImage"
        ;;
esac

# Look for AppImage in current directory
if [ -f "$APPIMAGE" ]; then
    APPIMAGE_PATH="./$APPIMAGE"
else
    echo "AppImage not found: $APPIMAGE"
    echo "Make sure you're running this script from the AppImage directory"
    echo "To create AppImages, run: python3 JumperlessAppPackager.py"
    exit 1
fi

# Make executable if not already
if [ ! -x "$APPIMAGE_PATH" ]; then
    echo "Making AppImage executable..."
    chmod +x "$APPIMAGE_PATH"
fi

# Run the AppImage
echo "Running: $APPIMAGE_PATH"
exec "$APPIMAGE_PATH" "$@"
'''
    

    
    # Desktop launcher script
    desktop_launcher = '''#!/bin/bash

# Desktop Launcher for Jumperless
# This script finds its own location and runs Jumperless from there

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Change to that directory and run the launcher
cd "$SCRIPT_DIR"

echo "Jumperless Desktop Launcher"
echo "Running from: $SCRIPT_DIR"
echo "=========================="

# Run the main launcher
./run_jumperless.sh "$@"

# Keep terminal open on exit
echo ""
echo "Jumperless has exited."
echo "Press Enter to close this window..."
read
'''
    
    # Write launcher scripts
    smart_launcher_path = appimage_dir / "run_jumperless.sh"
    with open(smart_launcher_path, 'w', encoding='utf-8') as f:
        f.write(smart_launcher)
    os.chmod(smart_launcher_path, 0o755)
    print(f"✅ Created run_jumperless.sh")
    

    
    desktop_launcher_path = appimage_dir / "desktop_launcher.sh"
    with open(desktop_launcher_path, 'w', encoding='utf-8') as f:
        f.write(desktop_launcher)
    os.chmod(desktop_launcher_path, 0o755)
    print(f"✅ Created desktop_launcher.sh")
    
    # Create desktop file
    desktop_content = '''[Desktop Entry]
Version=1.0
Type=Application
Name=Jumperless
Exec=sh -c "\\\\$HOME/.local/bin/jumperless/desktop_launcher.sh"
Icon=jumperless
Comment=Jumperless Wokwi Bridge - Electronics Prototyping Tool
Categories=Development;
Terminal=true
StartupNotify=true
Path=/usr/share/applications/
'''
    
    desktop_file_path = appimage_dir / "Jumperless.desktop"
    with open(desktop_file_path, 'w', encoding='utf-8') as f:
        f.write(desktop_content)
    print(f"✅ Created Jumperless.desktop")
    
    # Create uninstaller script
    uninstaller_content = '''#!/bin/bash

# Jumperless Linux Uninstaller Script
# Completely removes Jumperless from standard Linux directories

set -e  # Exit on any error

echo "🗑️  Jumperless Uninstaller"
echo "========================="
echo ""

# Define installation directories
INSTALL_DIR="$HOME/.local/bin/jumperless"
DESKTOP_DIR="$HOME/.local/share/applications"
ICONS_DIR="$HOME/.local/share/icons/hicolor/256x256/apps"

echo "This will completely remove Jumperless from your system:"
echo "📁 $INSTALL_DIR"
echo "📄 $DESKTOP_DIR/Jumperless.desktop"
echo "🖼️  $ICONS_DIR/jumperless.png"
echo ""

# Check if installation exists
if [ ! -d "$INSTALL_DIR" ] && [ ! -f "$DESKTOP_DIR/Jumperless.desktop" ]; then
    echo "✅ Jumperless is not installed (nothing to remove)"
    exit 0
fi

# Confirmation prompt
read -p "Are you sure you want to uninstall Jumperless? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "❌ Uninstall cancelled"
    exit 0
fi

echo ""
echo "🗑️  Removing Jumperless..."

# Remove main installation directory
if [ -d "$INSTALL_DIR" ]; then
    rm -rf "$INSTALL_DIR"
    echo "  ✅ Removed installation directory"
else
    echo "  ⚠️  Installation directory not found"
fi

# Remove desktop file
if [ -f "$DESKTOP_DIR/Jumperless.desktop" ]; then
    rm -f "$DESKTOP_DIR/Jumperless.desktop"
    echo "  ✅ Removed desktop file"
else
    echo "  ⚠️  Desktop file not found"
fi

# Remove icon
if [ -f "$ICONS_DIR/jumperless.png" ]; then
    rm -f "$ICONS_DIR/jumperless.png"
    echo "  ✅ Removed system icon"
else
    echo "  ⚠️  System icon not found"
fi

# Update desktop database
echo "🔄 Updating desktop database..."
if command -v update-desktop-database &> /dev/null; then
    update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
    echo "  ✅ Desktop database updated"
fi

# Update icon cache
echo "🖼️  Updating icon cache..."
if command -v gtk-update-icon-cache &> /dev/null; then
    gtk-update-icon-cache "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
    echo "  ✅ Icon cache updated"
fi

echo ""
echo "✅ Jumperless has been completely uninstalled!"
echo ""
echo "🙋 If you want to reinstall later, just run:"
echo "   ./install_jumperless.sh"
echo ""
echo "Thanks for using Jumperless! 👋"
'''
    
    uninstaller_path = appimage_dir / "uninstall_jumperless.sh"
    with open(uninstaller_path, 'w', encoding='utf-8') as f:
        f.write(uninstaller_content)
    os.chmod(uninstaller_path, 0o755)
    print(f"✅ Created uninstall_jumperless.sh")
    
    # Copy icon if available
    icon_source = pathlib.Path("assets/icons/icon.png")
    if icon_source.exists():
        shutil.copy2(icon_source, appimage_dir / "icon.png")
        print(f"✅ Copied icon.png")
    
    # Create unified README.md
    readme_content = '''# Jumperless Linux Distribution

Jumperless App distribution for Linux.

## Quick Start

### 1. Install with Desktop Integration (Recommended)

The simplest way to get Jumperless running with desktop integration:

```bash
./install_jumperless.sh
```

Benefits:
- Desktop integration (appears in Applications Menu)
- System icon support
- Follows Linux standards (XDG directories)
- Easy updates and uninstall
- Automatic architecture detection

After installation, launch from:
- Applications Menu: Search for "Jumperless"
- Terminal: `~/.local/bin/jumperless/desktop_launcher.sh`

### 2. Python Script (Auto-Updates Available)

For automatic updates from GitHub:

```bash
# Install Python dependencies first
pip install -r python/requirements.txt

# Run from source
python3 python/JumperlessWokwiBridge.py
```

When running as a Python script, Jumperless can automatically check for and download updates from GitHub, keeping you current with new features and bug fixes.

### 3. Launcher Scripts

Quick portable usage without installation:

```bash
# Smart launcher with architecture detection
./run_jumperless.sh

# Desktop-friendly launcher
./desktop_launcher.sh
```

### 4. Direct AppImage Execution

Manual execution:

```bash
# Make executable (first time only)
chmod +x Jumperless-*.AppImage

# Run directly
./Jumperless-x86_64.AppImage    # Intel/AMD systems
./Jumperless-aarch64.AppImage   # ARM64 systems (Raspberry Pi, etc.)
```

## What's Included

### AppImage Files
- `Jumperless-aarch64.AppImage` - ARM64 systems (Raspberry Pi 4+, Apple Silicon)
- `Jumperless-x86_64.AppImage` - Intel/AMD 64-bit systems (most PCs)

### Installation & Management
- `install_jumperless.sh` - Linux installer
- `uninstall_jumperless.sh` - Complete removal tool
- `Jumperless.desktop` - Desktop integration file

### Launcher Scripts
- `run_jumperless.sh` - Smart launcher with architecture detection
- `desktop_launcher.sh` - Desktop environment launcher

### Source Code & Dependencies
- `python/JumperlessWokwiBridge.py` - Main application source
- `python/requirements.txt` - Python dependencies

### Assets
- `icon.png` - Application icon
- `JumperlessFiles/` - Application data directory

## Method Comparison

| Method | Installation | Auto-Updates | Desktop Integration | Startup Speed |
|--------|-------------|--------------|-------------------|---------------|
| Desktop Install | One-time | Manual | Full | Fast |
| Python Script | Dependencies only | Automatic | Manual | Medium |
| Launcher Scripts | None | Manual | None | Fast |
| Direct AppImage | None | Manual | None | Fast |

## System Requirements

- OS: Linux (any distribution)
- Architecture: x86_64 or ARM64/aarch64
- Python: 3.10+ (for Python script method)
- Desktop: Any modern Linux desktop environment

## Architecture Detection

All launchers automatically detect your system:
- x86_64/amd64: Intel/AMD 64-bit processors
- aarch64/arm64: ARM 64-bit processors (Raspberry Pi, ARM servers)

## Troubleshooting

### Permission Issues
```bash
chmod +x *.sh *.AppImage
```

### Python Dependencies
```bash
cd python/
pip install -r requirements.txt
```

### Desktop Integration Not Working
```bash
./install_jumperless.sh
```

### AppImage Won't Start
```bash
./run_jumperless.sh
```

## Updates

### Automatic Updates (Python Script Only)
When running `python3 python/JumperlessWokwiBridge.py`, the app automatically:
- Checks GitHub for new versions
- Downloads updates in the background
- Keeps you current with latest features

### Manual Updates (Other Methods)
1. Download new AppImage distribution
2. Extract and replace files
3. Run installer again (if using desktop installation)

## Uninstallation

### Desktop Installation
```bash
./uninstall_jumperless.sh
```

### Manual Cleanup
```bash
rm -rf ~/.local/bin/jumperless/
rm ~/.local/share/applications/Jumperless.desktop
rm ~/.local/share/icons/hicolor/256x256/apps/jumperless.png
```

## Usage Tips

- For daily use: Use the desktop installer for best experience
- For development: Use Python script method for auto-updates
- For portability: Use launcher scripts on shared systems
- For minimal setup: Use direct AppImage execution

## Support & Documentation

- GitHub Repository: 	https://github.com/Architeuthis-Flux/JumperlessV5
- Support:		https://discord.gg/TcjM5uEgb4
- Getting Started: 	https://github.com/Architeuthis-Flux/JumperlessV5/blob/main/Docs/GettingStarted.md

---

Choose the method that best fits your workflow.
'''
    
    readme_path = appimage_dir / "README.md"
    with open(readme_path, 'w', encoding='utf-8') as f:
        f.write(readme_content)
    print(f"✅ Created unified README.md")
    
    # Create organized folder structure
    create_organized_structure(appimage_dir)

def create_organized_structure(appimage_dir):
    """Create organized folder structure with python/ and docs/ folders"""
    
    # Create python folder and copy source files
    python_dir = appimage_dir / "python"
    python_dir.mkdir(exist_ok=True)
    
    # Copy main application file
    main_app_source = pathlib.Path("JumperlessWokwiBridge.py")
    if main_app_source.exists():
        shutil.copy2(main_app_source, python_dir / "JumperlessWokwiBridge.py")
        print(f"✅ Copied JumperlessWokwiBridge.py to python/")
    
    # Copy requirements file
    requirements_source = pathlib.Path("requirements.txt")
    if requirements_source.exists():
        shutil.copy2(requirements_source, python_dir / "requirements.txt")
        print(f"✅ Copied requirements.txt to python/")
    
    # Remove any old README files since we now use a single README.md
    old_readme_files = [
        "README_LAUNCHERS.md",
        "README_AppImages.md", 
        "README_STANDARD_INSTALLATION.md"
    ]
    
    for readme_file in old_readme_files:
        readme_path = appimage_dir / readme_file
        if readme_path.exists():
            readme_path.unlink()
            print(f"✅ Removed old {readme_file}")
    
    print(f"✅ Created organized structure with python/ folder")

def create_distribution_archive(appimage_dir):
    """Create tar.gz archive of the AppImage directory"""
    
    print("\n📦 Creating distribution archive...")
    
    # Create archive path
    linux_dir = pathlib.Path("builds/linux")
    archive_path = linux_dir / "Jumperless-Linux.tar.gz"
    
    # Remove any existing archive
    if archive_path.exists():
        archive_path.unlink()
    
    try:
        import tarfile
        import tempfile
        
        # Create a temporary clean directory
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = pathlib.Path(temp_dir)
            clean_appimage_dir = temp_path / "jumperless"
            
            # Copy everything except fuse_hidden files
            shutil.copytree(appimage_dir, clean_appimage_dir, 
                          ignore=lambda dir, files: [f for f in files if f.startswith('.fuse_hidden')])
            
            # Create tar.gz archive
            with tarfile.open(archive_path, "w:gz") as tar:
                tar.add(clean_appimage_dir, arcname="jumperless")
            
            # Get archive size
            archive_size = archive_path.stat().st_size / 1024  # KB
            print(f"✅ Created distribution archive: {archive_path} ({archive_size:.0f}KB)")
            
    except Exception as e:
        print(f"❌ Failed to create archive: {e}")

def main():
    """Main function"""
    print("🚀 Jumperless Universal Multi-Platform Packager")
    print("=" * 50)
    
    if not os.path.exists("JumperlessWokwiBridge.py"):
        print("❌ JumperlessWokwiBridge.py not found!")
        print("Make sure you're running this script from the project root directory.")
        return 1
    
    # Default to Linux AppImage packaging
    success = package_linux_appimage()
    
    if success:
        print("\n🎉 Packaging completed successfully!")
        print("\n📦 Your packages are ready in the builds/ directory:")
        print("   builds/linux/appimage/")
        print("\n🚀 To run:")
        print("   cd builds/linux/appimage/")
        print("   ./run_jumperless.sh")
        return 0
    else:
        print("\n❌ Packaging failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main()) 