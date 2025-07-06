# Clean Packaging Workflow

The packager has been updated to keep the workspace clean and make it clear which AppImage to run.

## What's New

### 🧹 Automatic Cleanup
- The packager now automatically removes intermediate files after packaging
- No more confusion about which AppImage to run
- Clean workspace after packaging completes

### 📦 Clear Final Locations
- **Final AppImages**: `builds/linux/appimage/`
  - `Jumperless-x86_64.AppImage` (for most Linux PCs)
  - `Jumperless-aarch64.AppImage` (for ARM64 systems like Raspberry Pi)
- **Python packages**: `builds/linux/python/`
- **Windows packages**: `builds/windows/`
- **macOS packages**: `builds/macos/`

### 🚀 Simple Launchers
- **`run_jumperless.sh`** - Smart launcher that detects architecture and finds AppImage
- **`run_direct.sh`** - Direct launcher that runs AppImage with minimal checks

## Usage

### Option 1: Smart Launcher (Recommended)
```bash
./run_jumperless.sh
```

### Option 2: Direct Runner (If smart launcher has issues)
```bash
./run_direct.sh
```

### Option 3: Run AppImage Directly
```bash
# Navigate to the AppImage directory
cd builds/linux/appimage/

# Make executable (one time only)
chmod +x Jumperless-*.AppImage

# Run the AppImage
./Jumperless-x86_64.AppImage    # For x86_64 systems
./Jumperless-aarch64.AppImage   # For ARM64 systems
```

### Option 4: Package and Run
```bash
# Package the application
python3 JumperlessAppPackager.py

# Run with launcher
./run_jumperless.sh
```

## Launcher Features

### `run_jumperless.sh` (Smart Launcher)
- ✅ Detects system architecture automatically
- ✅ Searches multiple locations for AppImage
- ✅ Makes AppImage executable if needed
- ✅ Provides helpful error messages
- ✅ Passes all arguments to the AppImage

### `run_direct.sh` (Direct Runner)
- ✅ Minimal overhead - runs AppImage directly
- ✅ Simple architecture detection
- ✅ No fancy features that might cause issues
- ✅ Best for troubleshooting

## Files Cleaned Up

The packager now removes these intermediate files:
- `dist/` - PyInstaller output directory
- `build/` - PyInstaller build directory  
- `JumperlessLinux_x64.AppDir/` - AppImage structure directory
- `JumperlessLinux_arm64.AppDir/` - AppImage structure directory
- `Jumperless-*.AppImage` - Intermediate AppImage files in root
- `*.spec` - PyInstaller spec files
- `__pycache__/` - Python cache directories
- Various temporary files

## Manual Cleanup

If you need to clean up manually:
```bash
python3 -c "
import sys
sys.path.insert(0, '.')
from JumperlessAppPackager import cleanup_intermediate_files
cleanup_intermediate_files()
"
```

## Benefits

1. **No Confusion**: Clear which AppImage to run
2. **Clean Workspace**: No intermediate files cluttering the directory
3. **Easy Launch**: Simple launcher scripts handle everything
4. **Architecture Detection**: Automatically runs the right AppImage for your system
5. **Better Organization**: Final packages are in organized `builds/` directories
6. **Multiple Options**: Choose the launcher that works best for your setup

## Directory Structure After Packaging

```
JumperlessWokwiBridge/
├── builds/
│   ├── linux/
│   │   ├── appimage/
│   │   │   ├── Jumperless-x86_64.AppImage      ← Run this on x86_64
│   │   │   └── Jumperless-aarch64.AppImage     ← Run this on ARM64
│   │   └── python/
│   │       └── Jumperless_Linux.tar.gz
│   ├── windows/
│   └── macos/
├── run_jumperless.sh                           ← Smart launcher
├── run_direct.sh                               ← Direct launcher
├── JumperlessAppPackager.py                    ← Updated packager
└── ... (other project files)
```

## Troubleshooting

If you have issues with the launchers:

1. **Try the direct runner**: `./run_direct.sh`
2. **Run AppImage directly**: `cd builds/linux/appimage/ && ./Jumperless-*.AppImage`
3. **Check permissions**: Make sure scripts are executable (`chmod +x *.sh`)
4. **Verify AppImage exists**: Check `builds/linux/appimage/` directory

The workspace root stays clean with only the essential files visible! 