# Jumperless Native MicroPython Module

This document describes the implementation of the native Jumperless MicroPython module, which provides direct C-level integration with MicroPython instead of the previous string-based command parsing approach.

## Overview

The new implementation creates a proper MicroPython C extension module that integrates directly with the MicroPython interpreter, similar to built-in modules like `machine` or `time`. This approach eliminates the string parsing overhead and provides a more robust, type-safe interface.

## Key Benefits

### 1. **Performance**
- **Direct function calls**: No string parsing or command interpretation overhead
- **Native C integration**: Functions execute at native speed
- **Efficient memory usage**: No intermediate string buffers or parsing structures

### 2. **Type Safety**
- **Proper Python types**: Direct support for `int`, `float`, `bool`, `str`
- **Automatic type conversion**: MicroPython handles Python ↔ C type conversion
- **Parameter validation**: Built-in argument checking and error handling

### 3. **Error Handling**
- **Native exceptions**: Proper Python `ValueError`, `TypeError` exceptions
- **Stack traces**: Full Python debugging support
- **Immediate feedback**: Errors caught at call time, not after string parsing

### 4. **Developer Experience**
- **IDE support**: Proper Python syntax highlighting and completion
- **Pythonic API**: Follows Python conventions and patterns
- **Documentation**: Standard Python docstrings and help system
- **No string escaping**: No need to worry about quote handling in commands
- **Syntax highlighting**: Color-coded functions in Jumperless text editor

## Syntax Highlighting Features

The Jumperless text editor provides intelligent syntax highlighting for Python code with special recognition of Jumperless-specific functions:

### Color Coding Scheme

- **Orange (214)**: Python keywords (`def`, `class`, `if`, `for`, etc.)
- **Forest Green (79)**: Python built-ins (`print`, `len`, `range`, etc.)
- **Bright Yellow (39)**: Strings
- **Pink (199)**: Numbers
- **Magenta (207)**: Jumperless hardware functions (`gpio_set`, `dac_set`, `connect`, etc.)
- **Purple (105)**: Jumperless constants/types (`TOP_RAIL`, `GPIO_1`, `CONNECT_BUTTON`, etc.)
- **Cyan-Blue (45)**: **NEW** - JFS filesystem functions (`jfs.open`, `jfs.read`, `fs_exists`, etc.)
- **Green (34)**: Comments

### JFS Module Support

The editor now recognizes and highlights all JFS (Jumperless FileSystem) functions:

```python
import jfs

# All JFS functions are highlighted in cyan-blue
jfs.open("config.txt", "r")    # File operations
jfs.read("data.py")           # Direct read
jfs.write("log.txt", data)    # Direct write
jfs.exists("script.py")       # File checking
jfs.listdir("/")              # Directory listing
jfs.mkdir("python_scripts")   # Directory creation

# Basic filesystem functions also highlighted
fs_exists("test.txt")         # Simple existence check
fs_read("config.json")        # Simple file read
```

This makes it easy to distinguish between:
- Hardware operations (magenta)
- Filesystem operations (cyan-blue)  
- Standard Python code (orange/green)

## Implementation Architecture

### Core Files

1. **`modules/jumperless/modjumperless.c`**
   - Main MicroPython C extension module
   - Defines all Python-accessible functions
   - Handles parameter validation and type conversion
   - Registers the module with MicroPython
   - Includes comprehensive JFS (Jumperless FileSystem) module

2. **`src/JumperlessMicroPythonAPI.cpp`**
   - C++ wrapper functions that bridge MicroPython calls to existing Jumperless functionality
   - Provides C-compatible interface for the MicroPython module
   - Implements actual hardware control logic
   - **NEW**: Comprehensive filesystem functions for JFS module

3. **`src/EkiloEditor.cpp`**
   - **Updated**: Enhanced syntax highlighting for JFS functions
   - Recognizes filesystem operations with distinct cyan-blue coloring
   - Supports all jfs.* functions and basic fs_* functions

4. **`modules/jumperless/micropython.mk`**
   - Module build configuration
   - Configures compiler flags and source files
   - Enables both jumperless and jfs modules

5. **`lib/micropython/port/mpconfigport.h`**
   - MicroPython configuration enabling custom modules
   - Enables filesystem support for module importing

## API Reference

### JFS (Jumperless FileSystem) Module

The JFS module provides comprehensive filesystem access with both simple and advanced file operations:

```python
import jfs

# Simple file operations
jfs.write("config.txt", "debug=true")
content = jfs.read("config.txt")

# Advanced file operations with handles
with jfs.open("data.log", "a") as file:
    file.write("System started\n")
    size = file.size()
    pos = file.tell()

# Directory operations
jfs.mkdir("python_scripts")
files = jfs.listdir("/")
if jfs.exists("backup.py"):
    jfs.rename("backup.py", "scripts/backup.py")

# Basic filesystem functions (also available in main jumperless module)
fs_exists("test.txt")
fs_read("config.json")
fs_write("output.txt", "result")
```

**Key Features:**
- Context manager support (`with` statements)
- Memory-efficient chunk reading for large files
- Standard Python file API compatibility
- Embedded system optimizations
- **Syntax highlighting**: All JFS functions appear in cyan-blue color

For complete JFS documentation, see [`CodeDocs/JFS_Module_Documentation.md`](JFS_Module_Documentation.md).

### DAC Functions
```python
import jumperless

# Set DAC channel 0 to 2.5V (saved to memory)
jumperless.dac_set(0, 2.5)

# Set DAC channel 1 to 1.8V (temporary, not saved)
jumperless.dac_set(1, 1.8, False)

# Read current DAC setting
voltage = jumperless.dac_get(0)
```

### ADC Functions
```python
# Read ADC channel 0
voltage = jumperless.adc_get(0)
print(f"ADC reading: {voltage}V")
```

### INA Current/Power Sensor Functions
```python
# Read from INA sensor 0
current = jumperless.ina_get_current(0)      # mA
voltage = jumperless.ina_get_voltage(0)      # mV (shunt voltage)
bus_voltage = jumperless.ina_get_bus_voltage(0)  # V
power = jumperless.ina_get_power(0)          # mW
```

### GPIO Functions
```python
# Set GPIO pin 3 HIGH
jumperless.gpio_set(3, True)

# Set GPIO pin 4 LOW  
jumperless.gpio_set(4, False)

# Read GPIO pin state
state = jumperless.gpio_get(3)  # Returns True/False
```

### Node Connection Functions
```python
# Connect node 1 to node 5 (saved to memory)
jumperless.nodes_connect(1, 5)

# Connect nodes temporarily (not saved)
jumperless.nodes_connect(10, 15, False)

# Disconnect nodes
jumperless.nodes_disconnect(1, 5)

# Clear all connections
jumperless.nodes_clear()
```

### OLED Display Functions
```python
# Connect to OLED
connected = jumperless.oled_connect()

if connected:
    # Print text with size 2 font
    jumperless.oled_print("Hello World!", 2)
    
    # Print with size 1 font
    jumperless.oled_print("Small text", 1)
    
    # Update display
    jumperless.oled_show()
    
    # Clear display
    jumperless.oled_clear()
    
    # Disconnect
    jumperless.oled_disconnect()
```

### Arduino Functions
```python
# Reset Arduino
jumperless.arduino_reset()
```

### Probe Functions
```python
# Simulate probe tap on node 5
jumperless.probe_tap(5)
```

### Clickwheel Functions
```python
# Scroll up 3 clicks
jumperless.clickwheel_up(3)

# Scroll down 1 click
jumperless.clickwheel_down(1)

# Press clickwheel button
jumperless.clickwheel_press()
```

## Error Handling

The module provides proper Python exception handling:

```python
try:
    jumperless.dac_set(5, 2.5)  # Invalid channel
except ValueError as e:
    print(f"Error: {e}")  # "DAC channel must be 0-3"

try:
    jumperless.gpio_set(15, True)  # Invalid pin
except ValueError as e:
    print(f"Error: {e}")  # "GPIO pin must be 1-10"
```

## Comparison: Old vs New Approach

### Old String-Based Approach
```python
# String parsing with potential parsing errors
execute_sync('dac(set, 0, 2.5)')
result = execute_sync('adc(get, 0)')
execute_sync('nodes(connect, 1, 5)')

# Issues:
# - String parsing overhead
# - Error-prone quote handling
# - No type safety
# - Complex result parsing
# - Debugging difficulties
```

### New Native Approach
```python
# Direct function calls
jumperless.dac_set(0, 2.5)
result = jumperless.adc_get(0)
jumperless.nodes_connect(1, 5)

# Benefits:
# - No parsing overhead
# - Type safety
# - Native error handling
# - Clean Python syntax
# - Full IDE support
```

## Build Integration

The module integrates with the existing MicroPython build system:

1. **Source files** are automatically compiled with the MicroPython port
2. **Module registration** happens at compile time via `MP_REGISTER_MODULE`
3. **Configuration** is controlled via `MODULE_JUMPERLESS_ENABLED`
4. **Dependencies** are handled through the existing Jumperless build system

## Future Enhancements

1. **Object-oriented API**: Create Python classes for different hardware components
2. **Async support**: Add MicroPython async/await support for long-running operations
3. **Event callbacks**: Support for hardware event notifications
4. **Buffer operations**: Efficient bulk data operations for high-speed applications
5. **Documentation**: Auto-generated documentation from C module definitions

## Migration Guide

To migrate from the old string-based system:

1. Replace `execute_sync('command')` calls with direct function calls
2. Update error handling to use try/except blocks
3. Remove string formatting and quote escaping
4. Update return value handling (no more string parsing)
5. Test with proper Python types instead of string representations

## Example Migration

**Before:**
```python
execute_sync('dac(set, 0, 2.5)')
result = execute_sync('adc(get, 0)')
if 'SUCCESS' in result:
    voltage = float(result.split('=')[1].split()[0])
```

**After:**
```python
jumperless.dac_set(0, 2.5)
voltage = jumperless.adc_get(0)  # Direct float return
```

This implementation provides a more robust, efficient, and maintainable foundation for MicroPython integration with Jumperless hardware. 