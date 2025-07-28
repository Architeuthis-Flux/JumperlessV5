# Build System Cleanup Improvements

## Overview
Enhanced the `build_pulseview_BPandJ.sh` script with comprehensive cleanup capabilities to handle duplicate files and build artifacts that were accumulating during the build process.

## Problem
The build process was creating numerous duplicate and temporary files, particularly in:
- `bindings/cxx/` directory with files like "enums 2.cpp", "libsigrokcxx 2.pc"
- Multiple `.deps/` and `.libs/` directories throughout the source tree
- `.lo` (libtool object) files
- `.timestamp` files
- `.dirstamp` files
- Autotools generated cache and config files

## Solution
Added two new cleanup functions with comprehensive file removal:

### Enhanced `clean` Command
```bash
./build_pulseview_BPandJ.sh clean
```

**Removes:**
- ✅ Build work directory
- ✅ Autotools cache and config files  
- ✅ Libtool object files (.lo, .la)
- ✅ Dependency tracking (.deps, .libs, .dirstamp)
- ✅ Duplicate files with spaces in names
- ✅ Hardware driver object files
- ✅ Python cache files and directories

### New `deepclean` Command  
```bash
./build_pulseview_BPandJ.sh deepclean
```

**Removes everything from `clean` plus:**
- ✅ ALL autotools generated files (Makefile.in, configure, etc.)
- ✅ All backup and temporary files (*~, *.bak)
- ✅ Forces complete regeneration with ./autogen.sh

## Critical Bug Fix: Naming Inconsistency

### Problem Discovered
During testing, found a critical naming inconsistency in the build script:
- **JulsView executable built as**: `pulseview`  
- **Script tried to copy**: `julseview` (incorrect name)
- **Result**: Build reported success but failed during app bundle creation

### Solution Applied
Fixed both instances in the script:
```bash
# Before (incorrect)
cp julseview "$app_dir/MacOS/JulseView-BPandJ"

# After (correct) 
cp pulseview "$app_dir/MacOS/JulseView-BPandJ"
```

**Locations Fixed:**
1. `fast_rebuild()` function (line ~193)
2. `create_app_bundle()` function (line ~506)  
3. Documentation string (line 711)

## Autotools Configuration Fixes

### Missing Macro Files
The cleanup revealed missing autotools files that were restored:
- **m4/sigrok.m4** - Custom libsigrok macros
- **m4/ax_cxx_compile_stdcxx.m4** - C++17 standard macros
- **include/libsigrok/version.h.in** - Version header template  
- **include/libsigrok/libsigrok.h** - Main header
- **include/libsigrok/proto.h** - Protocol definitions

### Result
- ✅ Autoreconf now works without errors
- ✅ All hardware drivers compile successfully  
- ✅ Build process is stable and reliable

## Jumperless Driver Investigation

### Comprehensive Analysis Performed
Despite successful compilation, the `jumperless-mixed-signal` driver doesn't appear in PulseView scan lists. Extensive investigation confirmed:

#### ✅ **Verified Working:**
1. **Driver compilation** - No errors, compiles successfully
2. **Symbol presence** - Driver symbols exist in both static/dynamic libraries
3. **Registration mechanism** - Uses correct `SR_REGISTER_DEV_DRIVER` macro
4. **Linker sections** - Driver appears in correct `__sr_driver_list` section  
5. **Build inclusion** - Driver included in Makefile object lists
6. **Library linking** - App bundle uses updated library with driver symbols

#### ❌ **Issue Identified:**
- Driver is completely absent from runtime scan lists
- No initialization or scanning messages appear in verbose logs
- Driver appears to be filtered out during libsigrok startup

#### **Investigation Methods Used:**
```bash
# Check driver compilation
make -j4 | grep jumperless

# Verify symbol presence  
nm /usr/local/lib/libsigrok.dylib | grep jumperless

# Test driver scanning
/Applications/JulseView-BPandJ.app/Contents/MacOS/JulseView-BPandJ -l 5

# Check driver registration
nm /usr/local/lib/libsigrok.dylib | grep driver_list
```

#### **Root Cause Status:**
🔍 **Under Investigation** - The driver builds and links correctly but doesn't register at runtime. This suggests a deeper issue with:
- Driver initialization sequence  
- Runtime filtering mechanisms
- Driver context setup
- Library loading order

## Results Summary

### ✅ **Fully Resolved:**
1. **Build script naming inconsistency** - Fixed completely
2. **Duplicate file cleanup** - Comprehensive solution implemented  
3. **Autotools configuration** - All missing files restored
4. **Build reliability** - Process now stable and repeatable

### 🔍 **Ongoing Investigation:**
1. **Jumperless driver visibility** - Requires deeper runtime analysis
2. **Driver registration debugging** - May need libsigrok internal investigation

## Usage

### For Normal Development:
```bash
# Quick rebuild after code changes
./build_pulseview_BPandJ.sh rebuild

# Clean build artifacts
./build_pulseview_BPandJ.sh clean  
```

### For Deep Cleanup:
```bash
# Nuclear option - removes everything
./build_pulseview_BPandJ.sh deepclean

# Rebuild autotools after deepclean
cd .. && ./autogen.sh && cd pulseview-bpandj
./build_pulseview_BPandJ.sh rebuild
```

## Implementation Notes

- All cleanup functions include proper error handling
- Progress messages inform users of current operations
- Sudo permissions cached to avoid repeated password prompts
- Both library versions (C and C++) updated automatically
- Code signing and permission fixes applied to macOS app bundles 