# Jumperless Project Structure

## 📁 **Root Level Files**
- **`JumperlessWokwiBridge.py`** - Main application source code
- **`JumperlessAppPackager.py`** - Build/packaging script
- **`requirements.txt`** - Python dependencies
- **`example_sketch.ino`** - Example Arduino sketch

## 📁 **Directory Structure**

### `assets/` - Project Assets
- **`icons/`** - All icon formats (PNG, ICO, ICNS)
- **`firmware.uf2`** - Jumperless firmware
- **`JumperlessWokwiDMGwindow4x.png`** - DMG background image

### `builds/` - Organized Build Outputs
```
builds/
├── linux/
│   ├── appimage/           # Linux AppImages (ready to distribute)
│   │   ├── Jumperless-aarch64.AppImage
│   │   ├── Jumperless-x86_64.AppImage
│   │   ├── run_jumperless_appimage.sh
│   │   ├── Jumperless.desktop
│   │   └── README_AppImages.md
│   ├── python/             # Linux Python packages
│   │   ├── Jumperless_Linux.tar.gz
│   │   ├── Jumperless-linux-arm64.tar.gz
│   │   └── Jumperless-linux-x86_64.tar.gz
│   ├── native/             # Linux native builds (future)
│   ├── jumperless_cli_launcher.sh
│   └── jumperless_launcher.sh
├── macos/
│   ├── native/             # macOS native packages
│   │   └── Jumperless_Installer.dmg
│   ├── python/             # macOS Python packages
│   │   └── Jumperless_macOS.tar.gz
│   └── appimage/           # (unused for macOS)
└── windows/
    ├── native/             # Windows native packages
    │   ├── Jumperless.exe
    │   ├── Jumperless.exe.zip
    │   └── Jumperless-Windows-x64.zip
    ├── python/             # Windows Python packages
    │   └── Jumperless_Windows.zip
    ├── appimage/           # (unused for Windows)
    ├── run_in_windows_terminal.bat
    └── run_jumperless_terminal.bat
```

### `tools/` - Build Tools
- **`appimagetool-aarch64.AppImage`** - AppImage creation tool
- **`arduino-cli`** / **`arduino-cli.exe`** - Arduino CLI tools
- **`createDMG.sh`** - macOS DMG creation script
- **`avrdudeCustom.conf`** - AVR programming configuration

### `temp_organize/` - Temporary/Legacy Files
- Old build artifacts and legacy folder structures
- Can be cleaned up after verification

## 🚀 **Usage**

### Building Packages
```bash
# Run the packager (automatically detects platform)
python3 JumperlessAppPackager.py
```

### Distributing
- **Linux Users**: Share `builds/linux/appimage/` folder
- **macOS Users**: Share `builds/macos/native/Jumperless_Installer.dmg`
- **Windows Users**: Share `builds/windows/native/Jumperless.exe`
- **Developers**: Share appropriate Python packages from `builds/*/python/`

## 📦 **Package Types**

### AppImage (Linux)
- **Target**: End users on any Linux distribution
- **Benefits**: No installation required, self-contained
- **Files**: `builds/linux/appimage/`

### Native Packages
- **macOS**: DMG installer with app bundle
- **Windows**: Standalone EXE or installer
- **Files**: `builds/*/native/`

### Python Packages
- **Target**: Developers and advanced users
- **Benefits**: Cross-platform, smaller size, requires Python
- **Files**: `builds/*/python/`

## 🔧 **Development**

### Adding New Build Targets
1. Create subdirectory in `builds/{platform}/{type}/`
2. Update `JumperlessAppPackager.py` paths
3. Test build process

### Icon Updates
1. Update files in `assets/icons/`
2. Rebuild packages to include new icons

### Tool Updates
1. Update tools in `tools/` directory
2. Update paths in `JumperlessAppPackager.py` if needed

## 📋 **Migration Notes**

This structure was reorganized from a flat directory to improve:
- **Organization**: Clear separation by platform and package type
- **Maintainability**: Easier to find and manage build outputs
- **Distribution**: Ready-to-share folders for each platform
- **Development**: Clear separation of source, tools, and outputs

All paths in `JumperlessAppPackager.py` have been updated to use the new structure. 