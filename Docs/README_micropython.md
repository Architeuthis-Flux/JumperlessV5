# MicroPython with Jumperless Hardware Control

This guide covers how to write, load, and run Python scripts that control Jumperless hardware using the embedded MicroPython interpreter.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Hardware Control Functions](#hardware-control-functions)
3. [Writing Python Scripts](#writing-python-scripts)
4. [Loading and Running Scripts](#loading-and-running-scripts)
5. [REPL (Interactive Mode)](#repl-interactive-mode)
6. [File Management](#file-management)
7. [Examples and Demos](#examples-and-demos)
8. [Troubleshooting](#troubleshooting)

## Quick Start

### Starting MicroPython REPL
From the main Jumperless menu, press `p` to enter the MicroPython REPL:

```
>>> 
```

### Basic Hardware Control
```python
# Connect nodes 1 and 5
connect(1, 5)

# Set GPIO pin 1 to HIGH
gpio_set(1, True)

# Read ADC channel 0
voltage = adc_get(0)
print("Voltage: " + str(voltage) + "V")

# Set DAC channel 0 to 3.3V
dac_set(0, 3.3)
```


## Hardware Control Functions

All Jumperless hardware functions are automatically imported into the global namespace - no prefix needed

### Node Connections
```python
# Connect two nodes
connect(1, 5)                    # Connect node 1 to node 5
connect("D13", "TOP_RAIL")       # Connect using node names
connect(TOP_RAIL, BOTTOM_RAIL)   # Connect power rails

# Disconnect nodes
disconnect(1, 5)

# Check if nodes are connected
if is_connected(1, 5):
    print("Nodes 1 and 5 are connected")

# Clear all connections
nodes_clear()
```

### DAC (Digital-to-Analog Converter)
```python
# Set DAC voltage (-8.0V to 8.0V)
dac_set(0, 2.5)    # Set DAC channel 0 to 2.5V
dac_set(1, 1.65)   # Set DAC channel 1 to 1.65V

# Read current DAC voltage
voltage = dac_get(0)
print("DAC 0: " + str(voltage) + "V")

# Available channels:
# 0 = DAC0, 1 = DAC1, 2 = TOP_RAIL, 3 = BOTTOM_RAIL
# Can also use node names: DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL
```

### ADC (Analog-to-Digital Converter)
```python
# Read analog voltage (0-8V range for channels 0-3, 0-5V for channel 4)
voltage = adc_get(0)    # Read ADC channel 0
voltage = adc_get(1)    # Read ADC channel 1

# Available channels: 0, 1, 2, 3, 4
```

### GPIO (General Purpose I/O)
```python
# Set GPIO direction
gpio_set_dir(1, True)   # Set GPIO 1 as OUTPUT
gpio_set_dir(2, False)  # Set GPIO 2 as INPUT

# Set GPIO state
gpio_set(1, True)       # Set GPIO 1 HIGH
gpio_set(1, False)      # Set GPIO 1 LOW

# Read GPIO state (returns "HIGH" or "LOW")
state = gpio_get(2)     # Returns formatted string

# Configure pull resistors
gpio_set_pull(3, 1)     # Enable pull-up
gpio_set_pull(3, -1)    # Enable pull-down
gpio_set_pull(3, 0)     # No pull resistor

# Read GPIO configuration (returns formatted strings)
direction = gpio_get_dir(1)    # Returns "INPUT" or "OUTPUT"
pull = gpio_get_pull(2)        # Returns "PULLUP", "PULLDOWN", or "NONE"

# Available GPIO pins: 1-8 (GPIO 1-8), 9 (UART Tx), 10 (UART Rx)
```

### Current Sensing (INA219)
```python
# Read current sensor data
current = ina_get_current(0)      # Current in mA
voltage = ina_get_voltage(0)      # Shunt voltage in V
bus_voltage = ina_get_bus_voltage(0)  # Bus voltage in V
power = ina_get_power(0)          # Power in mW

# Available sensors: 0, 1
```

### OLED Display
```python
# Initialize OLED
oled_connect()                 # Connect to OLED
oled_print("Hello World!")     # Display text

# Clear display
oled_clear()

# Disconnect
oled_disconnect()
```

### Probe Functions
```python
# Read probe pad (blocking)
pad = probe_read_blocking()       # Returns ProbePad object only when a pad is touched

# Read probe pad (non-blocking)
pad = probe_read_nonblocking()    # Returns ProbePad object (which can be NO_PAD)

# Button functions (probe button)
button = probe_button()           # Read probe button state (blocking)
button = get_button()             # Alias
button = button_read()            # Another alias
button = read_button()            # Another alias
button = check_button()           # Non-blocking check
button = button_check()           # Alias

# Button with parameters
button = probe_button(True)       # Blocking
button = probe_button(False)      # Non-blocking
```

### Clickwheel Control
```python
# Simulate clickwheel actions
clickwheel_up(1)      # Up click
clickwheel_down(1)    # Down click
clickwheel_press()    # Press
```

### System Functions
```python
# Reset Arduino
arduino_reset()

# Run built-in apps
run_app("i2c")        # Run I2C scanner
run_app("spi")        # Run SPI test

# Show help
help()                # Display all available functions
nodes_help()          # Show all available node names and aliases
```

### Formatted Output and Custom Types
The Jumperless module provides formatted output for better readability:

```python
# GPIO state returns formatted strings
state = gpio_get(1)           # Returns "HIGH" or "LOW"
direction = gpio_get_dir(1)   # Returns "INPUT" or "OUTPUT"
pull = gpio_get_pull(1)       # Returns "PULLUP", "PULLDOWN", or "NONE"

# Connection status returns formatted strings
connected = is_connected(1, 5) # Returns "CONNECTED" or "DISCONNECTED"

# Voltage and current readings are automatically formatted
voltage = adc_get(0)          # Returns float (e.g., 3.300)
current = ina_get_current(0)  # Returns float in mA (e.g., 123.4)
power = ina_get_power(0)      # Returns float in mW (e.g., 456.7)

# All functions work with both numbers and string aliases
gpio_set_dir("GPIO_1", True)  # Same as gpio_set_dir(1, True)
connect("TOP_RAIL", "GPIO_1") # Same as connect(101, 131)
```

## Writing Python Scripts

### Basic Script Structure
```python
#!/usr/bin/env python3
"""
My Jumperless Script
Description of what this script does
"""

import time

def main():
    print("Starting my script...")
    
    # Connect some nodes
    connect(1, 5)
    connect(2, 6)
    
    # Set up GPIO
    gpio_set_dir(1, True)  # Output
    gpio_set_dir(2, False) # Input
    
    # Main loop
    for i in range(10):
        gpio_set(1, True)
        time.sleep(0.5)
        gpio_set(1, False)
        time.sleep(0.5)
        
        # Read input
        if gpio_get(2) == "HIGH":
            print("Button pressed!")
    
    # Cleanup
    nodes_clear()
    print("Script complete!")

if __name__ == "__main__":
    main()
```

### Node Names and Constants
The Jumperless module provides extensive node name support with multiple aliases for each node:

```python
# Power rails (multiple aliases supported)
TOP_RAIL = 101        # Also: TOPRAIL, T_R, TOP_R
BOTTOM_RAIL = 102     # Also: BOT_RAIL, BOTTOMRAIL, BOTRAIL, B_R, BOT_R
SUPPLY_3V3 = 103      # Also: 3V3, 3.3V
SUPPLY_5V = 105       # Also: 5V, +5V
SUPPLY_8V_P = 120     # Also: 8V_P, 8V_POS
SUPPLY_8V_N = 121     # Also: 8V_N, 8V_NEG

# Ground connections
GND = 100             # Also: GROUND
TOP_RAIL_GND = 104    # Also: TOP_GND
BOTTOM_RAIL_GND = 126 # Also: BOT_GND, BOTTOM_GND

# DAC outputs
DAC0 = 106            # Also: DAC_0, DAC0_5V
DAC1 = 107            # Also: DAC_1, DAC1_8V

# ADC inputs
ADC0 = 110            # Also: ADC_0, ADC0_8V
ADC1 = 111            # Also: ADC_1, ADC1_8V
ADC2 = 112            # Also: ADC_2, ADC2_8V
ADC3 = 113            # Also: ADC_3, ADC3_8V
ADC4 = 114            # Also: ADC_4, ADC4_5V
ADC7 = 115            # Also: ADC_7, ADC7_PROBE, PROBE

# Current sensing
ISENSE_PLUS = 108     # Also: ISENSE_POS, ISENSE_P, INA_P, I_P, CURRENT_SENSE_PLUS, ISENSE_POSITIVE, I_POS
ISENSE_MINUS = 109    # Also: ISENSE_NEG, ISENSE_N, INA_N, I_N, CURRENT_SENSE_MINUS, ISENSE_NEGATIVE, I_NEG

# GPIO pins (multiple naming conventions)
GPIO_1 = 131          # Also: RP_GPIO_1, GPIO1, GP_1, GP1
GPIO_2 = 132          # Also: RP_GPIO_2, GPIO2, GP_2, GP2
GPIO_3 = 133          # Also: RP_GPIO_3, GPIO3, GP_3, GP3
GPIO_4 = 134          # Also: RP_GPIO_4, GPIO4, GP_4, GP4
GPIO_5 = 135          # Also: RP_GPIO_5, GPIO5, GP_5, GP5
GPIO_6 = 136          # Also: RP_GPIO_6, GPIO6, GP_6, GP6
GPIO_7 = 137          # Also: RP_GPIO_7, GPIO7, GP_7, GP7
GPIO_8 = 138          # Also: RP_GPIO_8, GPIO8, GP_8, GP8

# UART pins
UART_TX = 116         # Also: RP_UART_TX, TX, RP_GPIO_16
UART_RX = 117         # Also: RP_UART_RX, RX, RP_GPIO_17

# Additional RP GPIOs
RP_GPIO_18 = 118      # Also: GP_18
RP_GPIO_19 = 119      # Also: GP_19

# Buffer connections
BUFFER_IN = 139       # Also: ROUTABLE_BUFFER_IN, BUF_IN, BUFF_IN, BUFFIN
BUFFER_OUT = 140      # Also: ROUTABLE_BUFFER_OUT, BUF_OUT, BUFF_OUT, BUFFOUT

# Arduino Nano pins (extensive support)
D13 = 83              # Also: NANO_D13
D12 = 82              # Also: NANO_D12
D11 = 81              # Also: NANO_D11
D10 = 80              # Also: NANO_D10
D9 = 79               # Also: NANO_D9
D8 = 78               # Also: NANO_D8
D7 = 77               # Also: NANO_D7
D6 = 76               # Also: NANO_D6
D5 = 75               # Also: NANO_D5
D4 = 74               # Also: NANO_D4
D3 = 73               # Also: NANO_D3
D2 = 72               # Also: NANO_D2
D1 = 71               # Also: NANO_D1
D0 = 70               # Also: NANO_D0

# Arduino Nano analog pins
A0 = 86               # Also: NANO_A0
A1 = 87               # Also: NANO_A1
A2 = 88               # Also: NANO_A2
A3 = 89               # Also: NANO_A3
A4 = 90               # Also: NANO_A4
A5 = 91               # Also: NANO_A5
A6 = 92               # Also: NANO_A6
A7 = 93               # Also: NANO_A7

# Arduino Nano power/control
VIN = 69              # Also: NANO_VIN
RESET = 84            # Also: NANO_RESET
AREF = 85             # Also: NANO_AREF
RST0 = 94             # Also: NANO_RESET_0
RST1 = 95             # Also: NANO_RESET_1
N_GND1 = 96           # Also: NANO_GND_1
N_GND0 = 97           # Also: NANO_GND_0

# Special functions
EMPTY = 127           # Also: EMPTY_NET
```

### Error Handling
```python
try:
    # Attempt hardware operation
    connect(1, 5)
    voltage = adc_get(0)
    print("Voltage: " + str(voltage) + "V")
    
except Exception as e:
    print("Error: " + str(e))
    # Cleanup on error
    nodes_clear()
```



## Loading and Running Scripts



### Method 1: File Manager
From the REPL, type `files` to open the file manager:

```
>>> files
```

Navigate to your script and press Enter to load it for editing, then press `Ctrl+P` to load it into the REPL for execution.

**Note:** The standard Python `exec(open(...).read())` method is not supported in the Jumperless MicroPython environment. Always use the file manager and `Ctrl+P` to run scripts.

### Method 2: REPL Commands
From the MicroPython REPL, you can use the following commands to manage scripts:

```
# Load script into editor for modification
load my_script.py

# Save current session as script
save my_new_script.py
```

### Method 3: eKilo Editor
From the REPL, type `new` to create a new script:

```
>>> new
```

This opens the eKilo text editor where you can write and save scripts.

### Method 4: Direct Execution
From the main Jumperless menu, you can execute single commands:

```
> gpio_set(1, True)
> adc_get(0)
> connect(1, 5)
```

## REPL (Interactive Mode)

### Starting REPL
From main menu: Press `p`

### REPL Commands
```
CTRL + q           - Exit REPL
history            - Show command history and saved scripts
save [name]        - Save last executed script
load <name>        - Load script by name or number
files              - Open file manager
new                - Create new script with eKilo editor
helpl              - Show REPL help
help()             - Show hardware commands
```

### Navigation
```
↑/↓ arrows         - Browse command history
←/→ arrows         - Move cursor, edit text
TAB                - Add 4-space indentation
Enter              - Execute (empty line in multiline to finish)
Ctrl+Q             - Force quit REPL or interrupt running script
```

### Multiline Mode
The REPL automatically detects when you need multiple lines:

```python
>>> def blink_led():
...     for i in range(5):
...         gpio_set(1, True)
...         time.sleep(0.5)
...         gpio_set(1, False)
...         time.sleep(0.5)
... 
>>> blink_led()
```

### Command History
- Use ↑/↓ arrows to browse previous commands
- Commands are automatically saved
- Type `history` to see all saved scripts

## File Management

### Directory Structure
```
/python_scripts/           # Main scripts directory
├── examples/             # Built-in examples
│   ├── 01_dac_basics.py
│   ├── 02_adc_basics.py
│   ├── 03_gpio_basics.py
│   ├── 04_node_connections.py
│   ├── led_brightness_control.py
│   ├── voltage_monitor.py
│   ├── stylophone.py
│   └── README.md
├── lib/                  # User modules
├── modules/              # Additional modules
└── history.txt          # Command history
```

### File Manager Commands
From the file manager interface:

```
Enter          - Open file or enter directory
Backspace      - Go up one directory
Ctrl+N         - Create new file
Ctrl+E         - Edit selected file
Ctrl+D         - Delete selected file
Ctrl+P         - Save and launch REPL
Ctrl+Q         - Exit file manager
```

### USB Mass Storage
When connected via USB, the Jumperless appears as a mass storage device:

1. Connect Jumperless via USB
2. Device appears as "JUMPERLESS" drive
3. Navigate to `/python_scripts/` folder
4. Copy Python files directly
5. Safely eject drive

## Examples and Demos

### Built-in Examples
The system includes several example scripts. To run an example:

1. Type `files` in the REPL.
2. Navigate to the `examples/` directory.
3. Select the desired script and press Enter to edit/view it.
4. Press `Ctrl+P` to load it into the REPL for execution.

Example scripts include:
- 01_dac_basics.py (DAC basics - voltage control)
- 02_adc_basics.py (ADC basics - voltage reading)
- 03_gpio_basics.py (GPIO basics - digital I/O)
- 04_node_connections.py (Node connections)
<!-- - led_brightness_control.py (LED brightness control)
- voltage_monitor.py (Voltage monitoring)
- stylophone.py (Stylophone demo) -->

### LED Blinking Example
```python
import time

def blink_led():
    # Connect LED to GPIO 1
    gpio_set_dir(1, True)
    
    print("Blinking LED on GPIO 1...")
    for i in range(10):
        gpio_set(1, True)
        time.sleep(0.5)
        gpio_set(1, False)
        time.sleep(0.5)
    
    print("Blinking complete!")

blink_led()
```

### Voltage Monitor Example
```python
import time

def monitor_voltage():
    print("Monitoring ADC voltage...")
    print("Press Ctrl+Q to stop")
    
    try:
        while True:
            voltage = adc_get(0)
            print("ADC 0: " + str(round(voltage, 3)) + "V")
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nMonitoring stopped")

monitor_voltage()
```

### Node Connection Example
```python
def test_connections():
    print("Testing node connections...")
    
    # Connect some test nodes
    connect(1, 5)
    connect(2, 6)
    connect(3, 7)
    
    # Check connections
    print("1-5 connected: " + str(is_connected(1, 5)))
    print("2-6 connected: " + str(is_connected(2, 6)))
    print("3-7 connected: " + str(is_connected(3, 7)))
    
    # Show all connections
    print_bridges()
    
    # Clean up
    nodes_clear()
    print("Test complete!")

test_connections()
```

### Alias Examples - Multiple Ways to Reference Nodes
```python
def demonstrate_aliases():
    print("Demonstrating node name aliases...")
    
    # All these connect the same nodes (TOP_RAIL to BOTTOM_RAIL)
    connect(TOP_RAIL, BOTTOM_RAIL)      # Full names
    connect("TOPRAIL", "BOT_RAIL")      # Short aliases
    connect("T_R", "B_R")               # Very short aliases
    connect(101, 102)                   # Raw numbers
    
    # Power supply connections
    connect("3V3", "5V")                # Short voltage names
    connect(SUPPLY_3V3, SUPPLY_5V)      # Full names
    connect(103, 105)                   # Numbers
    
    # GPIO connections with different naming styles
    connect("GPIO_1", "GPIO_2")         # Standard GPIO names
    connect("RP_GPIO_1", "GP1")         # Mixed naming
    connect("GP_1", "GPIO1")            # Short forms
    connect(131, 132)                   # Numbers
    
    # Arduino Nano connections
    connect("D13", "A0")                # Standard Arduino names
    connect("NANO_D13", "NANO_A0")      # Full Nano names
    connect(83, 86)                     # Numbers
    
    # Current sensing
    connect("ISENSE_PLUS", "ISENSE_MINUS")  # Full names
    connect("INA_P", "INA_N")               # Short INA names
    connect("I_P", "I_N")                   # Very short
    connect(108, 109)                       # Numbers
    
    print("All alias examples connected!")
    print_bridges()
    nodes_clear()

demonstrate_aliases()
```

### Advanced Hardware Control Examples
```python
def advanced_examples():
    print("Advanced hardware control examples...")
    
    # Power rail control with DAC
    dac_set(0, 3.3)                    # Set DAC_A to 3.3V
    dac_set(1, 2.5)                    # Set DAC_B to 2.5V
    
    # Connect power to rails
    connect("DAC0", "TOP_RAIL")        # Power top rail
    connect("DAC1", "BOTTOM_RAIL")     # Power bottom rail
    
    # Set up GPIO for LED control
    gpio_set_dir("GPIO_1", True)       # Output
    gpio_set_dir("GP_2", False)        # Input (using short alias)
    gpio_set_pull("GPIO_2", 1)         # Pull-up resistor
    
    # Monitor voltage and current
    voltage = adc_get("ADC0")          # Read voltage
    current = ina_get_current(0)       # Read current
    
    print("Voltage: " + str(voltage) + "V")
    print("Current: " + str(current) + "mA")
    
    # OLED display
    oled_connect()
    oled_print("Advanced Demo", 2)
    
    # Cleanup
    nodes_clear()
    oled_disconnect()

advanced_examples()
```

### Comprehensive Example - All New Features
```python
def comprehensive_demo():
    """
    Demonstrates all the new Jumperless MicroPython features:
    - Extensive node name aliases
    - Formatted output
    - Advanced probe functions
    - String-based GPIO control
    - Multiple connection methods
    """
    print("=== Comprehensive Jumperless Demo ===")
    
    # 1. Power rail setup with aliases
    print("\n1. Setting up power rails...")
    dac_set(0, 3.3)                    # DAC_A to 3.3V
    dac_set(1, 2.5)                    # DAC_B to 2.5V
    
    # Connect using different alias styles
    connect("DAC0", "TOP_RAIL")        # Full names
    connect("DAC_1", "BOT_RAIL")       # Mixed aliases
    print("✓ Power rails configured")
    
    # 2. GPIO setup with string aliases
    print("\n2. Configuring GPIO...")
    gpio_set_dir("GPIO_1", True)       # Output
    gpio_set_dir("GP_2", False)        # Input (short alias)
    gpio_set_pull("GPIO_2", 1)         # Pull-up
    
    # Test GPIO with formatted output
    gpio_set("GPIO_1", True)
    state = gpio_get("GPIO_1")
    direction = gpio_get_dir("GP_1")
    pull = gpio_get_pull("GPIO_2")
    
    print("  GPIO_1 state: " + str(state))
    print("  GPIO_1 direction: " + str(direction))
    print("  GPIO_2 pull: " + str(pull))
    
    # 3. Voltage monitoring with aliases
    print("\n3. Monitoring voltages...")
    voltage1 = adc_get("ADC0")         # String alias
    voltage2 = adc_get(1)              # Number
    current = ina_get_current(0)       # Current sensor
    
    print("  ADC0: " + str(round(voltage1, 3)) + "V")
    print("  ADC1: " + str(round(voltage2, 3)) + "V")
    print("  Current: " + str(round(current, 1)) + "mA")
    
    # 4. Node connections with extensive aliases
    print("\n4. Testing node connections...")
    
    # Connect using various alias styles
    connect("TOP_RAIL", "GPIO_1")      # Full names
    connect("T_R", "GP1")              # Short aliases
    connect("3V3", "GP_1")             # Mixed styles
    connect(101, 131)                  # Raw numbers
    
    # Check connections with formatted output
    connected1 = is_connected("TOP_RAIL", "GPIO_1")
    connected2 = is_connected("T_R", "GP1")
    
    print("  TOP_RAIL-GPIO_1: " + str(connected1))
    print("  T_R-GP1: " + str(connected2))
    
    # 5. OLED display
    print("\n5. Testing OLED display...")
    oled_connect()
    oled_print("Jumperless Demo", 2)
    print("  ✓ OLED message displayed")
    
    # 6. Probe functions
    print("\n6. Testing probe functions...")
    print("  Touch probe to any pad (or press Ctrl+Q to skip)...")
    
    try:
        pad = probe_read_blocking()
        print("  ✓ Probe touched pad: " + str(pad))
    except KeyboardInterrupt:
        print("  ⚠ Probe test skipped")
    
    # 7. Show all connections
    print("\n7. Current connections:")
    print_bridges()
    
    # 8. Cleanup
    print("\n8. Cleaning up...")
    nodes_clear()
    oled_disconnect()
    gpio_set("GPIO_1", False)
    
    print("✓ Comprehensive demo complete!")

# Run the comprehensive demo
comprehensive_demo()
```

## Troubleshooting

### Common Issues

**Script not found:**
```python
# Check if file exists
import os
print(os.listdir('/python_scripts'))
```

**Function not available:**
```python
# Check available functions
help()
# or
import jumperless
print(dir(jumperless))
```

**Memory errors:**
```python
# Check memory usage
import gc
print("Free: " + str(gc.mem_free()) + ", Used: " + str(gc.mem_alloc()))
gc.collect()  # Force garbage collection
```

**REPL not responding:**
- Press Ctrl+Q to force quit
- Restart the Jumperless device
- Check for infinite loops in your code

### Debug Tips

1. **Use print statements:**
```python
print("Debug: Starting function")
result = some_function()
print("Debug: Result = " + str(result))
```

2. **Check return values:**
```python
success = connect(1, 5)
if not success:
    print("Failed to connect nodes")
```

3. **Test hardware step by step:**
```python
# Test each component individually
print("Testing GPIO...")
gpio_set_dir(1, True)
gpio_set(1, True)
time.sleep(1)
gpio_set(1, False)

print("Testing ADC...")
voltage = adc_get(0)
print("ADC reading: " + str(voltage) + "V")
```

### Getting Help

- Type `help()` in the REPL for function documentation
- Type `helpl` for REPL-specific help
- Check the built-in examples for working code
- Use the file manager to browse existing scripts

### Performance Tips

1. **Use local variables:**
```python
# Good
voltage = adc_get(0)
if voltage > 2.0:
    print("High voltage")

# Avoid repeated function calls
```

2. **Clean up resources:**
```python
try:
    # Your code here
    pass
finally:
    nodes_clear()  # Always clean up connections
```

3. **Use appropriate delays:**
```python
import time
time.sleep(0.1)  # 100ms delay
time.sleep(1)    # 1 second delay
```

## Advanced Features

### Custom Modules
Place your modules in `/python_scripts/lib/`:

```python
# my_module.py
def my_function():
    return "Hello from my module!"

# main_script.py
import my_module
result = my_module.my_function()
print(result)
```

### Persistent Storage
Scripts and data persist across reboots in the filesystem.

### Real-time Control
All hardware functions provide real-time control with minimal latency.

### Interrupt Handling
Use Ctrl+Q to interrupt long-running scripts safely.

---

For more information, see the built-in examples and use `help()` in the REPL for detailed function documentation. 