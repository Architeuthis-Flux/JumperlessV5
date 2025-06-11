# MicroPython Integration Setup Guide

This guide explains how to properly set up MicroPython embedding in the Jumperless V5 firmware with **full floating-point support** for the RP2350 FPU.

## 🚨 Important: Previous Integration Issues Fixed

## 🛠️ Setup Instructions

### Step 1: Install MicroPython Repository

The build system needs access to the official MicroPython repository:

```bash
# Create directory structure
mkdir -p ~/src/micropython
cd ~/src/micropython

# Clone MicroPython repository
git clone https://github.com/micropython/micropython.git
cd micropython

# Build the mpy-cross compiler (required for embed port)
cd mpy-cross
make
```

### Step 2: Build MicroPython Embed Port

You can either run the automated build or manual build:

#### Option A: Automated Build (Recommended)
The build will automatically happen when you compile with PlatformIO, but you can test it manually:

```bash
# From the project root
./scripts/build_micropython.sh
```

#### Option B: Manual Build
```bash
# From src/micropython directory
make -f micropython_embed.mk MICROPYTHON_TOP=$HOME/src/micropython/micropython
```

## 🎯 Features Enabled

With the corrected integration, you now have:

### ✅ **Floating Point Support**
- Full `float` type support using RP2350 FPU
- Mathematical operations: `+`, `-`, `*`, `/`, `**`
- Built-in functions: `abs()`, `round()`, `min()`, `max()`
- Math module with trigonometric functions

### ✅ **Core MicroPython Features**
- Variables and basic data types
- Control flow (if/else, loops)  
- Functions and classes
- Error handling (try/except)
- Import system
- Garbage collection

### ✅ **Built-in Modules**
- `gc` - Garbage collection control
- `sys` - System-specific parameters  
- `io` - Core I/O functionality
- `builtins` - Built-in functions

## 🧪 Testing MicroPython Integration

You can test the integration with floating-point support:

```python
# Test basic floating point
x = 3.14159
y = 2.71828
result = x * y + 1.0
print(f"Result: {result}")

# Test math operations
import math
angle = 45.0
sin_val = math.sin(math.radians(angle))
print(f"sin(45°) = {sin_val}")
```

## 🔧 Configuration Details

### ROM Level: BASIC_FEATURES
- Provides floating-point support with proper QSTR generation
- Includes essential modules without bloat
- Optimized for embedded systems with FPU

### Memory Configuration
- **Heap Size**: Configured for RP2350 RAM constraints
- **Stack Size**: Optimized for embedded use
- **Flash Usage**: ~200KB for MicroPython core

## 🐛 Troubleshooting

### Build Fails: "MicroPython repository not found"
```bash
# Ensure MicroPython is cloned to the correct location
ls ~/src/micropython/micropython
# Should show: LICENSE, README.md, py/, ports/, etc.
```

### Build Fails: "mpy-cross not found" 
```bash
# Build the cross-compiler
cd ~/src/micropython/micropython/mpy-cross
make
```

### QSTR Errors During Compilation
```bash
# Clean and rebuild MicroPython embed port
rm -rf src/micropython/micropython_embed
./scripts/build_micropython.sh
```

### "Multiple Definition" Linker Errors
These should be resolved with the correct build flags in `platformio.ini`. If you still see them, ensure you're not manually including MicroPython source files in your main code.

## 📁 Directory Structure

After successful build:
```
src/micropython/
├── mpconfigport.h              # Configuration for RP2350
├── micropython_embed.mk        # Build script  
├── micropython_embed/          # Generated embed port
│   ├── genhdr/
│   │   ├── qstrdefs.generated.h    # Auto-generated QSTR symbols
│   │   └── moduledefs.h           # Auto-generated module definitions
│   ├── py/                    # Core MicroPython files
│   ├── shared/runtime/        # Shared runtime files
│   └── port/                  # Port-specific files
└── README.md                  # Basic usage info
```

## 🚀 Next Steps

With proper MicroPython integration now working:

1. **Add Hardware Bindings**: Create Python modules for Jumperless hardware
2. **Implement REPL**: Add interactive Python console over USB/UART
3. **File System**: Add support for storing Python scripts on flash
4. **Custom Modules**: Develop Jumperless-specific Python libraries

The foundation is now solid and follows MicroPython best practices! 🎉 