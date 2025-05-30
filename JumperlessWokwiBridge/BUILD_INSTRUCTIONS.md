# Jumperless Build Instructions

This project supports building packages for all major desktop platforms: **macOS**, **Linux**, and **Windows**.

## 🚀 Quick Start

### Option 1: GitHub Actions (Recommended for Windows)
The easiest way to build all packages is using GitHub Actions, which builds on native platforms for maximum compatibility.

1. **Push to GitHub** or create a **Pull Request**
2. **GitHub Actions automatically builds** packages for all platforms
3. **Download artifacts** from the Actions tab
4. **Create a GitHub Release** to automatically attach all packages

### Option 2: Local Building
For development and testing, you can build packages locally.

## 📦 Local Building

### Prerequisites
```bash
# Install Python dependencies
pip install -r requirements.txt
pip install pyinstaller

# Make scripts executable (macOS/Linux)
chmod +x jumperless_cli_launcher.sh
chmod +x createDMG.sh
chmod +x Packager_clean.py
```

### Build All Platforms (macOS only)
```bash
python Packager_clean.py
```
This will:
- ✅ Build macOS DMG installer
- ✅ Build Linux x86_64 and ARM64 packages  
- ⚠️ Attempt Windows build (cross-compilation, may have limitations)

### Build Single Platform

#### macOS
```python
from Packager_clean import package_macos
package_macos()
```

#### Linux
```python
from Packager_clean import package_linux
package_linux()
```

#### Windows (Native Only)
```python
from Packager_clean import package_windows
package_windows()
```

## 🌐 GitHub Actions Workflow

### Automatic Triggers
- **Push** to `main` or `develop` branches
- **Pull Requests** to `main`
- **GitHub Releases** (automatically attaches all packages)
- **Manual trigger** from Actions tab

### Build Matrix
| Platform | Runner | Output |
|----------|--------|--------|
| **Windows** | `windows-latest` | `Jumperless.exe` |
| **macOS** | `macos-latest` | `Jumperless_Installer.dmg` |
| **Linux** | `ubuntu-latest` | AppImage + tar.gz packages |

### Artifacts
Each build creates downloadable artifacts:
- **windows-package**: Contains `Jumperless.exe`
- **macos-package**: Contains `Jumperless_Installer.dmg`
- **linux-packages**: Contains AppImage and tar.gz files

### Creating Releases
1. Create a new GitHub release (or tag)
2. GitHub Actions automatically:
   - Builds packages for all platforms
   - Attaches packages to the release
   - Runs tests on all platforms

## 📁 Output Files

### Windows
- **`Jumperless.exe`** - Single executable file (portable)

### macOS
- **`Jumperless_Installer.dmg`** - macOS installer
  - Drag-and-drop installation
  - Universal2 binary (Intel + Apple Silicon)

### Linux
- **`Jumperless-x86_64.AppImage`** - Portable executable (Intel/AMD)
- **`Jumperless-aarch64.AppImage`** - Portable executable (ARM64/Raspberry Pi)
- **`Jumperless-linux-x86_64.tar.gz`** - Fallback package
- **`Jumperless-linux-arm64.tar.gz`** - Fallback package

## 🛠 Platform-Specific Notes

### Windows
- **Native building required** for reliable single-file executables
- **Cross-compilation limitations** when building from macOS/Linux
- **GitHub Actions recommended** for Windows builds

### macOS
- **Universal2 binaries** support both Intel and Apple Silicon
- **Code signing warnings** are normal during development
- **DMG creation** requires macOS (cannot cross-compile)

### Linux
- **AppImage creation** may fail on non-Linux platforms
- **ARM64 cross-compilation** has limitations
- **Native building recommended** for best compatibility

## 🧪 Testing

### Automated Testing
GitHub Actions runs tests on all platforms:
- Verifies package creation
- Checks file sizes
- Validates package structure

### Manual Testing
```bash
# Test Windows executable
./Jumperless.exe

# Test macOS app
open Jumperless_Installer.dmg

# Test Linux packages
chmod +x Jumperless-x86_64.AppImage
./Jumperless-x86_64.AppImage

# Or extract tar.gz
tar -xzf Jumperless-linux-x86_64.tar.gz
cd JumperlessLinux_x64.AppDir
./AppRun
```

## 🔧 Troubleshooting

### GitHub Actions Fails
1. Check the Actions tab for error logs
2. Ensure all required files are committed
3. Verify Python dependencies in requirements.txt

### Local Build Issues
1. **Windows**: Use GitHub Actions for reliable builds
2. **macOS**: Ensure Xcode command line tools installed
3. **Linux**: Install AppImage tools: `sudo apt install wget fuse`

### Package Size Too Large
- Check included dependencies
- Consider excluding unnecessary modules
- Use `--exclude-module` in PyInstaller commands

## 📝 Development Workflow

1. **Local development**: Use local packager for quick testing
2. **Pull requests**: GitHub Actions builds and tests automatically  
3. **Releases**: Create GitHub release to publish all packages
4. **Distribution**: Users download platform-specific packages

This ensures maximum compatibility and professional-grade distribution across all platforms! 🎉 