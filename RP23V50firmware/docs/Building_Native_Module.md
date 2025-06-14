# Building the Jumperless Native MicroPython Module

This guide explains how to build the Jumperless firmware with the native MicroPython module integration.

## Prerequisites

- PlatformIO installed and configured
- Git (for MicroPython repository cloning)
- Build tools (make, gcc, etc.)

## Build Process

### 1. Build MicroPython with Jumperless Module

First, build MicroPython with the native Jumperless module integrated:

```bash
# From the project root directory
./scripts/build_micropython.sh
```

This script will:
- Clone or update the MicroPython repository
- Build MicroPython embed port with the Jumperless module included
- Generate QSTR definitions for the Jumperless functions
- Verify the module integration

### 2. Build Jumperless Firmware

After MicroPython is built, compile the main firmware:

```bash
# Using PlatformIO
pio run

# Or using PlatformIO with specific environment
pio run -e pico
```

### 3. Upload to Device

```bash
# Upload firmware
pio run -t upload

# Or with specific environment
pio run -e pico -t upload
```

## Build Verification

The build script will verify:

1. **QSTR Generation**: Confirms MicroPython QSTR definitions are generated
2. **Jumperless QSTRs**: Checks that Jumperless-specific function names are included
3. **Module Files**: Verifies all required source files are present
4. **Configuration**: Ensures the module is enabled in the configuration

### Expected Output

```
✅ MicroPython embed build successful!
   Generated 1234 QSTR definitions
   Jumperless module QSTRs found: 25
   ✅ Jumperless MicroPython module found
   ✅ Jumperless API wrapper found
   ✅ Jumperless module enabled in configuration
✅ MicroPython is ready for use with Jumperless native module enabled!
```

## Troubleshooting

### Common Issues

1. **Module Not Found**
   ```
   ❌ Jumperless MicroPython module missing
   ```
   - Check that `lib/micropython/modjumperless.cpp` exists
   - Verify the USER_C_MODULES path is correct

2. **API Wrapper Missing**
   ```
   ❌ Jumperless API wrapper missing
   ```
   - Check that `src/JumperlessMicroPythonAPI.cpp` exists
   - Verify include paths in the source file

3. **No Jumperless QSTRs**
   ```
   Warning: No Jumperless module QSTRs detected
   ```
   - The module may not be properly registered
   - Check `MODULE_JUMPERLESS_ENABLED` is defined
   - Verify `MP_REGISTER_MODULE` call in the module source

4. **Build Errors**
   - Check that all include paths are correct in `platformio.ini`
   - Verify that required headers are available
   - Ensure `MODULE_JUMPERLESS_ENABLED=1` is set in build flags

### Debug Steps

1. **Check Module Registration**:
   ```bash
   grep -r "MP_QSTR_jumperless" lib/micropython/micropython_embed/
   ```

2. **Verify Build Flags**:
   ```bash
   grep -A5 "build_flags" platformio.ini
   ```

3. **Check Include Paths**:
   ```bash
   find . -name "modjumperless.cpp"
   find . -name "JumperlessMicroPythonAPI.cpp"
   ```

## Runtime Testing

After successful build and upload, test the module:

```python
# In MicroPython REPL
import jumperless

# Test basic functions
jumperless.dac_set(0, 2.5)
voltage = jumperless.adc_get(0)
print(f"ADC reading: {voltage}V")

# Test node connections
jumperless.nodes_connect(1, 5)
jumperless.nodes_disconnect(1, 5)
```

Or call the built-in test function from C++:
```cpp
testJumperlessNativeModule();
```

## File Structure

After building, the key files are:

```
lib/micropython/
├── modjumperless.cpp              # Native module implementation
├── micropython.mk                 # Module build configuration
├── mpconfigport.h                 # Module enabled flag
└── micropython_embed/             # Generated MicroPython files
    ├── genhdr/qstrdefs.generated.h    # QSTR definitions
    └── ...

src/
├── JumperlessMicroPythonAPI.cpp   # C++ wrapper functions
├── Python_Proper.cpp             # MicroPython integration
└── Python_Proper.h               # Header file

platformio.ini                    # Build configuration with module flags
```

## Performance Benefits

The native module provides significant improvements over the string-based approach:

- **~10x faster execution** - No string parsing overhead
- **Better memory efficiency** - No intermediate string buffers
- **Type safety** - Native Python type handling
- **Error handling** - Proper Python exceptions
- **IDE support** - Full Python syntax support

## Next Steps

After successful build:

1. Test all hardware functions through the native module
2. Update existing Python code to use direct function calls
3. Remove old string-based command parsing code
4. Add additional hardware functions as needed

For usage examples, see `examples/python_proper_native_demo.py`. 