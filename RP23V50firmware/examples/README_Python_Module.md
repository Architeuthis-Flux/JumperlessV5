# Jumperless MicroPython Module System

This document explains how to use Python to control your Jumperless hardware in three different ways:

1. **Built-in MicroPython REPL** - Interactive Python directly on the Jumperless
2. **Python Scripts on Filesystem** - Store .py files on Jumperless and execute them
3. **External Python over Serial** - Control from your computer via USB/serial

## Overview

The Jumperless Python system consists of:

- **Command Parser** (`src/Python.cpp`) - Hierarchical command parsing with dual syntax support
- **MicroPython Module** (`src/JythonModule.py`) - Clean Python API for hardware control
- **C Extension** (`src/Python.cpp`) - Bridge between Python and hardware functions
- **Example Scripts** (`examples/`) - Demo scripts and external control tools

## Architecture

```
┌─────────────────────┐    ┌─────────────────────┐    ┌─────────────────────┐
│   Python Script    │    │  MicroPython REPL  │    │  External Python   │
│   (.py file)       │    │   (Interactive)     │    │   (via Serial)     │
└─────────┬───────────┘    └─────────┬───────────┘    └─────────┬───────────┘
          │                          │                          │
          └──────────────────────────┼──────────────────────────┘
                                     │
                    ┌─────────────────▼─────────────────┐
                    │       JythonModule.py             │
                    │   (Python API Layer)              │
                    └─────────────────┬─────────────────┘
                                      │
                    ┌─────────────────▼─────────────────┐
                    │    _jumperless C Extension        │
                    │   (Python ↔ C++ Bridge)          │
                    └─────────────────┬─────────────────┘
                                      │
                    ┌─────────────────▼─────────────────┐
                    │    Command Parser Engine          │
                    │  (parseAndExecutePythonCommand)   │
                    └─────────────────┬─────────────────┘
                                      │
          ┌───────────────────────────┼───────────────────────────┐
          │                           │                           │
┌─────────▼─────────┐    ┌────────────▼────────────┐    ┌─────────▼─────────┐
│   DAC/ADC         │    │     GPIO Control        │    │  Node Connections │
│   Functions       │    │     Functions           │    │   Functions       │
└───────────────────┘    └─────────────────────────┘    └───────────────────┘
```

## Usage Methods

### 1. Built-in MicroPython REPL

Access the interactive Python prompt directly on your Jumperless:

```python
# Initialize MicroPython (if not auto-started)
setupJumperlessModule()

# Import the Jumperless module
import JythonModule as jl

# Use the hardware APIs
jl.dac.set(0, 2.5)                    # Set DAC 0 to 2.5V
voltage = jl.adc.get(0)               # Read ADC channel 0
jl.gpio.set(5, jl.gpio.HIGH)          # Set GPIO pin 5 HIGH
jl.nodes.connect(1, 5, save=False)    # Connect nodes 1 and 5
jl.oled.print("Hello World!", 2)      # Display text on OLED

# Get help
jl.help()                             # Show module help
help(jl.dac)                          # Help for specific component
```

### 2. Python Scripts on Filesystem

Create `.py` files and store them on your Jumperless filesystem:

**Example: `demo.py`**
```python
import JythonModule as jl
import time

def blink_sequence():
    for i in range(5):
        jl.gpio.set(5, True)
        jl.oled.print(f"ON {i+1}", 2)
        time.sleep(0.5)
        
        jl.gpio.set(5, False)
        jl.oled.print(f"OFF {i+1}", 2)
        time.sleep(0.5)

if __name__ == "__main__":
    jl.oled.print("Script Start", 1)
    blink_sequence()
    jl.oled.print("Complete!", 2)
```

**Execute from C++:**
```cpp
// In your C++ code
executePythonScript("demo.py");
```

**Execute from MicroPython REPL:**
```python
exec(open('demo.py').read())
```

### 3. External Python over Serial

Control your Jumperless from your computer using the provided external control script:

```bash
# Install requirements
pip install pyserial

# Auto-detect and connect
python3 external_python_control.py

# Specify port manually
python3 external_python_control.py /dev/ttyUSB0 115200
python3 external_python_control.py COM3 115200
```

**Interactive external control:**
```python
from external_python_control import ExternalJumperless

# Connect to Jumperless
jl = ExternalJumperless()
jl.connect("/dev/ttyUSB0", 115200)

# Use the same API as the internal module
jl.dac.set(0, 2.5)
jl.nodes.connect(1, 5)
jl.oled.print("From Computer!")

# Execute raw commands
jl.execute("dac(set, 0, 3.3)")

jl.disconnect()
```

## Command Syntax

The system supports both hierarchical and legacy command syntax:

### Hierarchical Syntax (Recommended)
```python
# Hardware category, then sub-action, then parameters
dac(set, 0, 2.5, save=False)
gpio(set, 5, HIGH)
nodes(connect, 1, 5, save=True)
oled(print, "Hello", 2)
adc(get, 0)
```

### Legacy Syntax (Backwards Compatible)
```python
# Traditional function names
setDac(0, 2.5, save=False)
setGpio(5, HIGH)
connectNodes(1, 5, save=True)
oledPrint("Hello", 2)
readAdc(0)
```

## API Reference

### DAC (Digital-to-Analog Converter)
```python
jl.dac.set(channel, voltage, save=False)    # Set DAC output
jl.dac.get(channel)                         # Get current setting
```

### ADC (Analog-to-Digital Converter)  
```python
jl.adc.get(channel)                         # Read voltage
jl.adc.read(channel)                        # Alias for get()
```

### GPIO (General Purpose I/O)
```python
jl.gpio.set(pin, value)                     # Set pin HIGH/LOW
jl.gpio.get(pin)                            # Read pin state
jl.gpio.direction(pin, direction)           # Set INPUT/OUTPUT

# Constants
jl.gpio.HIGH, jl.gpio.LOW
jl.gpio.INPUT, jl.gpio.OUTPUT
```

### Nodes (Connection Management)
```python
jl.nodes.connect(node1, node2, save=False)  # Connect nodes
jl.nodes.disconnect(node1, node2)           # Disconnect nodes
jl.nodes.remove(node1, node2)               # Alias for disconnect
jl.nodes.clear()                            # Clear all connections
```

### OLED Display
```python
jl.oled.connect()                           # Connect to display
jl.oled.disconnect()                        # Disconnect display
jl.oled.print(text, size=2)                 # Print text (clear+print+show)
jl.oled.clear()                             # Clear display
jl.oled.show()                              # Update display
jl.oled.set_cursor(x, y)                    # Set cursor position
jl.oled.set_text_size(size)                 # Set text size
jl.oled.cycle_font()                        # Cycle to next font
jl.oled.set_font(font)                      # Set specific font
jl.oled.is_connected()                      # Check connection
```

### Arduino Control
```python
jl.arduino.reset()                          # Reset connected Arduino
jl.arduino.flash()                          # Flash Arduino (if supported)
```

### UART Control
```python
jl.uart.connect()                           # Connect UART to Arduino D0/D1
jl.uart.disconnect()                        # Disconnect UART
```

### Probe Simulation
```python
jl.probe.tap(node)                          # Simulate probe tap
jl.probe.click(action="click")              # Simulate button press
```

## Convenience Functions

```python
# Quick access functions
jl.connect_nodes(1, 5, save=False)          # Quick node connection
jl.set_voltage(0, 3.3, save=False)          # Quick DAC setting
jl.read_voltage(0)                          # Quick ADC reading
jl.display("Hello!", size=2)                # Quick OLED display
jl.reset_arduino()                          # Quick Arduino reset
```

## Error Handling

```python
try:
    result = jl.dac.set(0, 2.5)
    if result.startswith("ERROR:"):
        print(f"Command failed: {result}")
    else:
        print(f"Success: {result}")
except JumperlessError as e:
    print(f"Jumperless error: {e}")
except Exception as e:
    print(f"Python error: {e}")
```

## Example Applications

### 1. Voltage Sweep Test
```python
import JythonModule as jl
import time

def voltage_sweep():
    """Sweep DAC voltage from 0V to 3.3V"""
    for voltage in [0.0, 1.0, 2.0, 3.0, 3.3]:
        jl.dac.set(0, voltage)
        reading = jl.adc.get(0)
        jl.oled.print(f"{voltage}V", 2)
        print(f"Set: {voltage}V, Read: {reading}")
        time.sleep(1)
```

### 2. GPIO Blink Pattern
```python
import JythonModule as jl
import time

def blink_pattern():
    """Create a blinking pattern on multiple GPIO pins"""
    pins = [1, 2, 3, 4, 5]
    
    for i in range(10):
        # Turn on pins in sequence
        for pin in pins:
            jl.gpio.set(pin, True)
            time.sleep(0.1)
        
        # Turn off pins in reverse
        for pin in reversed(pins):
            jl.gpio.set(pin, False)
            time.sleep(0.1)
```

### 3. Node Connection Matrix
```python
import JythonModule as jl
import time

def test_connections():
    """Test connectivity between multiple nodes"""
    test_pairs = [(1, 5), (2, 6), (3, 7), (4, 8)]
    
    for node1, node2 in test_pairs:
        print(f"Testing connection {node1} <-> {node2}")
        
        jl.nodes.connect(node1, node2)
        jl.oled.print(f"{node1}-{node2}", 2)
        time.sleep(1)
        
        jl.nodes.disconnect(node1, node2)
        time.sleep(0.5)
```

## Development Notes

### Adding New Functions

1. **Add to Command Parser** (`src/Python.cpp`):
   ```cpp
   // Add new action type
   enum actionType {
       action_existing,
       action_new_function  // Add your new function category
   };
   
   // Add sub-actions
   enum new_function_sub {
       new_function_sub_action1,
       new_function_sub_action2
   };
   ```

2. **Add to Python Module** (`src/JythonModule.py`):
   ```python
   class NewFunction:
       @staticmethod
       def action1(param1, param2):
           cmd = f"new_function(action1, {param1}, {param2})"
           return _execute_command(cmd)
   
   # Create instance
   new_function = NewFunction()
   ```

3. **Add Hardware Implementation**:
   ```cpp
   // In the appropriate hardware file
   void handleNewFunction(pythonCommand& cmd) {
       switch(cmd.subAction) {
           case new_function_sub_action1:
               // Implement action1
               break;
       }
   }
   ```

### Testing

Run the comprehensive test suite:

```cpp
// C++ tests
testPythonParser();           // Test command parsing
testPythonExecution();        // Test execution with hardware
testJumperlessModule();       // Test MicroPython integration

// Python tests
import JythonModule as jl
jl.help()                     // Verify module loads
```

```python
# External Python tests
python3 external_python_control.py   // Test serial communication
```

## Troubleshooting

### Common Issues

1. **"Module not found" Error**:
   - Ensure MicroPython is initialized: `setupJumperlessModule()`
   - Check that `JythonModule.py` is accessible

2. **Serial Connection Failed**:
   - Verify correct port and baud rate
   - Check that Jumperless is connected and powered
   - Try different ports: `python3 external_python_control.py`

3. **Command Execution Errors**:
   - Check command syntax: hierarchical vs legacy
   - Verify parameter types and ranges
   - Review error messages in serial output

4. **OLED Display Issues**:
   - Ensure OLED is connected: `jl.oled.connect()`
   - Check I2C connections
   - Try different fonts: `jl.oled.cycle_font()`

### Debug Mode

Enable verbose logging:
```python
# In JythonModule.py, set debug flag
_debug = True

# Or from external Python
jl.execute("debug(enable)")
```

## Performance Notes

- **Command Parsing**: ~1ms typical, ~5ms worst case
- **Serial Communication**: ~10-50ms round trip depending on baud rate
- **Hardware Operations**: Varies by operation (DAC: ~1ms, GPIO: ~0.1ms)
- **OLED Updates**: ~20-100ms depending on content

## Integration Examples

### Arduino Sketch Integration
```cpp
#include "Python.h"

void setup() {
    // Initialize Jumperless systems
    setupJumperlessModule();
    
    // Run a Python script on startup
    executePythonScript("startup.py");
}

void loop() {
    // Execute Python commands based on conditions
    if (some_condition) {
        executeMicroPython("jl.oled.print('Alert!', 2)");
    }
}
```

### Web Interface Integration
```python
# Flask web server controlling Jumperless
from flask import Flask, request, jsonify
from external_python_control import ExternalJumperless

app = Flask(__name__)
jl = ExternalJumperless("/dev/ttyUSB0", 115200)

@app.route('/api/dac', methods=['POST'])
def set_dac():
    channel = request.json['channel']
    voltage = request.json['voltage']
    result = jl.dac.set(channel, voltage)
    return jsonify({'result': result})

@app.route('/api/gpio/<int:pin>/<state>')
def set_gpio(pin, state):
    result = jl.gpio.set(pin, state.upper())
    return jsonify({'result': result})
```

This comprehensive Python module system provides a powerful, flexible way to control your Jumperless hardware using modern Python syntax while maintaining backwards compatibility and supporting multiple usage scenarios. 