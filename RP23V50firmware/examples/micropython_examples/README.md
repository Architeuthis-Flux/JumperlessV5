# Jumperless MicroPython Examples

This directory contains comprehensive MicroPython examples demonstrating the various functions available in the jumperless module. Each example is designed to be simple, educational, and practical, with both OLED display output and serial console logging.

## 📁 Available Examples

### 01_dac_basics.py
**Digital-to-Analog Converter (DAC) Operations**

Demonstrates how to control the DAC outputs on your Jumperless device:
- Setting voltages on different DAC channels (DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL)
- Reading back DAC values for verification
- Voltage sweeping demonstrations
- Power rail setup and control
- Resetting all DACs

**Key Functions:**
- `dac_set(channel, voltage)` - Set DAC output voltage
- `dac_get(channel)` - Read current DAC voltage
- Support for channels 0-3 or constants (DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL)
- Voltage range: -8.0V to +8.0V

### 02_adc_basics.py
**Analog-to-Digital Converter (ADC) Operations**

Shows how to read analog voltages from the ADC inputs:
- Reading individual ADC channels (ADC0-ADC3)
- Reading all channels in sequence
- Continuous monitoring with statistics
- Voltage range detection and classification
- Real-time monitoring with interrupt handling

**Key Functions:**
- `adc_get(channel)` - Read ADC input voltage
- Support for channels 0-3
- Voltage range: typically 0V to 3.3V (hardware dependent)

### 03_gpio_basics.py
**General Purpose Input/Output (GPIO) Operations**

Comprehensive GPIO control demonstrations:
- Setting pin directions (input/output)
- Reading and writing pin states
- Configuring pull-up/pull-down resistors
- Blinking patterns and multi-pin control
- Binary counting demonstrations
- Advanced GPIO patterns

**Key Functions:**
- `gpio_set(pin, value)` - Set GPIO pin state (HIGH/LOW)
- `gpio_get(pin)` - Read GPIO pin state
- `gpio_set_dir(pin, direction)` - Set pin direction (INPUT/OUTPUT)
- `gpio_get_dir(pin)` - Get pin direction
- `gpio_set_pull(pin, pull)` - Set pull resistor configuration
- `gpio_get_pull(pin)` - Get pull resistor configuration

### 04_node_connections.py
**Node Connection Operations**

The heart of Jumperless functionality - connecting breadboard nodes:
- Basic breadboard hole connections
- Connecting to special nodes (power rails, DAC, ADC, GPIO)
- Using different node specification methods (numbers, strings, constants)
- Power rail setup and device connections
- DAC-to-ADC testing through breadboard connections
- Connection verification and cleanup

**Key Functions:**
- `connect(node1, node2)` - Connect two nodes
- `disconnect(node1, node2)` - Disconnect two nodes
- `is_connected(node1, node2)` - Check if nodes are connected
- `nodes_clear()` - Clear all connections

## 🚀 Getting Started

### Prerequisites
- Jumperless V5 device with MicroPython firmware
- USB connection to your computer
- Terminal emulator (like PuTTY, screen, or Thonny)

### Running Examples

1. **Copy examples to your Jumperless:**
   ```bash
   # Copy the entire examples directory to your Jumperless device
   cp -r micropython_examples/ /path/to/jumperless/storage/
   ```

2. **Connect to MicroPython REPL:**
   ```bash
   # Using screen (Linux/Mac)
   screen /dev/ttyACM0 115200
   
   # Using PuTTY (Windows)
   # Connect to COM port at 115200 baud
   ```

3. **Run individual examples:**
   ```python
   # In the MicroPython REPL
   exec(open('micropython_examples/01_dac_basics.py').read())
   ```

4. **Or run specific demo functions:**
   ```python
   # Load the module
   exec(open('micropython_examples/01_dac_basics.py').read())
   
   # Run individual demos
   dac_basic_demo()
   dac_sweep_demo()
   dac_rail_demo()
   ```

## 📋 Example Structure

Each example follows a consistent structure:

```python
"""
Example Title
============

Description of what the example demonstrates.

Functions demonstrated:
- function1() - Description
- function2() - Description

Key concepts:
- Concept 1
- Concept 2
"""

import time

def demo_function_1():
    """Individual demonstration function"""
    # Setup
    oled_clear()
    oled_print("Demo Title")
    
    # Main demo logic with both OLED and console output
    print("Console output with details")
    
    # Cleanup if needed

def run_all_demos():
    """Run all demonstration functions"""
    demo_function_1()
    demo_function_2()
    # ... etc

# Run when executed directly
if __name__ == "__main__":
    run_all_demos()
```

## 🔧 Key Features

### Dual Output
- **OLED Display**: Visual feedback on the device's built-in display
- **Serial Console**: Detailed logging and status information

### MicroPython Compatibility
- No f-strings (uses string concatenation for compatibility)
- Pure Python with time delays for visual demonstration
- Error handling where appropriate

### Educational Focus
- Clear, commented code
- Step-by-step progression
- Real-world usage examples
- Verification and testing included

## 🎯 Node Reference

### Node Types
- **Numbers**: 1-60 (breadboard holes)
- **Arduino Pins**: D0-D13, A0-A7 (nano header)
- **GPIO**: GPIO_1-GPIO_8 (routable GPIO)
- **Power**: TOP_RAIL, BOTTOM_RAIL, GND
- **DAC**: DAC0, DAC1 (analog outputs)
- **ADC**: ADC0-ADC3 (analog inputs)

### Three Ways to Specify Nodes

1. **Numbers** (direct breadboard holes):
   ```python
   connect(1, 30)    # Connect holes 1 and 30
   ```

2. **Strings** (case-insensitive names):
   ```python
   connect("D13", "TOP_RAIL")    # Arduino pin to power rail
   connect("gpio_1", "adc0")     # GPIO to ADC
   ```

3. **Constants** (pre-defined objects):
   ```python
   connect(TOP_RAIL, D13)        # Using imported constants
   connect(GPIO_1, A0)           # No quotes needed
   ```

## 🛠️ Common Usage Patterns

### Power Rail Setup
```python
# Set up power rails
dac_set(TOP_RAIL, 5.0)      # 5V power
dac_set(BOTTOM_RAIL, 0.0)   # Ground

# Connect devices to power
connect(TOP_RAIL, 1)        # 5V to breadboard hole 1
connect(BOTTOM_RAIL, 2)     # Ground to breadboard hole 2
```

### Signal Testing
```python
# Connect DAC to ADC through breadboard
connect(DAC0, 10)           # DAC0 to hole 10
connect(10, ADC0)           # Hole 10 to ADC0

# Test signal path
dac_set(DAC0, 3.3)          # Set DAC to 3.3V
voltage = adc_get(0)        # Read back from ADC
print("Signal: " + str(voltage) + "V")
```

### GPIO Control
```python
# Set up GPIO as output
gpio_set_dir(1, True)       # GPIO1 as output
connect(GPIO_1, 15)         # Connect to breadboard hole 15

# Control the GPIO
gpio_set(1, True)           # Set high
gpio_set(1, False)          # Set low
```

## 🔍 Troubleshooting

### Common Issues

1. **"NameError: name 'connect' is not defined"**
   - Make sure all jumperless functions are imported globally
   - Try: `from jumperless import *`

2. **"OLED not updating"**
   - Check OLED connections with `oled_connect()`
   - Verify OLED is not disconnected with `oled_disconnect()`

3. **Unexpected voltage readings**
   - Check node connections with `is_connected(node1, node2)`
   - Verify power rail voltages with `dac_get(TOP_RAIL)`

4. **GPIO not responding**
   - Check pin direction with `gpio_get_dir(pin)`
   - Verify connections with breadboard nodes

### Debugging Tips

```python
# Check connection status
connected = is_connected(1, 30)
print("Connection 1-30: " + connected)

# Read current DAC settings
for ch in [DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL]:
    voltage = dac_get(ch)
    print("DAC " + str(ch) + ": " + str(voltage) + "V")

# Check GPIO status
for pin in range(1, 9):
    direction = gpio_get_dir(pin)
    state = gpio_get(pin)
    print("GPIO" + str(pin) + ": " + direction + ", " + state)
```

## 📚 Next Steps

After working through these examples, you'll be ready to:
- Create your own custom circuit connections
- Build complex signal processing chains
- Develop automated test sequences
- Design educational demonstrations
- Prototype electronic circuits rapidly

## 🤝 Contributing

Feel free to contribute additional examples or improvements:
- Add more complex demonstrations
- Include error handling patterns
- Create application-specific examples
- Document advanced techniques

## 📖 Additional Resources

- [Jumperless Documentation](../README.md)
- [MicroPython Official Docs](https://docs.micropython.org/)
- [Jumperless Hardware Guide](../../docs/)

---

Happy prototyping with your Jumperless device! 🎉 