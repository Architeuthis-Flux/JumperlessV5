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

![Screenshot 2025-07-04 at 7 03 24 PM](https://github.com/user-attachments/assets/e7ce0688-5ddf-48da-8560-4a8f6b747c4f)


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
connect(1, 5)                    # Connect using numbers
connect("d13", "tOp_rAiL")       # Connect using node names (case insensitive when in quotes)
connect(TOP_RAIL, BOTTOM_RAIL)   # Connect using DEFINEs (all caps) Note: This will actually just be ignored by the Jumperless due to Do Not Intersect rules

# Disconnect bridges
disconnect(1, 5)

# Disconnect everything connected to a node
disconnect(5, -1)

# Check if nodes are connected
if is_connected(1, 5):
    print("Nodes 1 and 5 are connected")

# Clear all connections
nodes_clear()
```
![Screenshot 2025-07-04 at 7 22 35 PM](https://github.com/user-attachments/assets/e08d9b83-aa4d-4e1a-873c-7f6c46ddb5bc)


### DAC (Output Voltage)
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
![Screenshot 2025-07-04 at 7 17 36 PM](https://github.com/user-attachments/assets/f68b3bf2-3420-4d51-800a-1e8e9e804261)

### ADC (Measure Voltage)
```python
# Read analog voltage (0-8V range for channels 0-3, 0-5V for channel 4)
voltage = adc_get(0)    # Read ADC channel 0
voltage = adc_get(1)    # Read ADC channel 1

# Available channels: 0, 1, 2, 3, 4
```
![Screenshot 2025-07-04 at 7 22 01 PM](https://github.com/user-attachments/assets/79cf16e8-8a79-4f11-9cf4-52456735b0dc)

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
![Screenshot 2025-07-04 at 7 22 35 PM](https://github.com/user-attachments/assets/5b5f884f-f459-4a31-9f21-89d084594f97)
![Screenshot 2025-07-04 at 7 31 19 PM](https://github.com/user-attachments/assets/c7bdb245-59a4-46db-9c52-fcc43c1f359e)

### Current Sensing (INA219)
```python
# Read current sensor data
current = ina_get_current(0)          # Current in A
current = ina_get_current(0) * 1000   # Current in mA
voltage = ina_get_voltage(0)          # Shunt voltage in V
bus_voltage = ina_get_bus_voltage(0)  # Bus voltage in V
power = ina_get_power(0)              # Power in W

# Available sensors: 0, 1    # INA 1 is hardwired to the output of DAC 0 because it's meant for measuring resistance
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
![Screenshot 2025-07-04 at 7 37 54 PM](https://github.com/user-attachments/assets/4d0b2e29-e33d-4e1c-b339-336d1d686319)


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

The help() functions will list all the available commands (except for the new ones I forget to update)

<details> 
<summary> Expand for the entire output of help() </summary>

```python
>>> help()
Jumperless Native MicroPython Module
Hardware Control Functions with Formatted Output:
(GPIO functions return formatted strings like HIGH/LOW, INPUT/OUTPUT, PULLUP/NONE, CONNECTED/DISCONNECTED)

DAC (Digital-to-Analog Converter):
  jumperless.dac_set(channel, voltage)         - Set DAC output voltage
  jumperless.dac_get(channel)                  - Get DAC output voltage
  jumperless.set_dac(channel, voltage)         - Alias for dac_set
  jumperless.get_dac(channel)                  - Alias for dac_get

          channel: 0-3, DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL
          channel 0/DAC0: DAC 0
          channel 1/DAC1: DAC 1
          channel 2/TOP_RAIL: top rail
          channel 3/BOTTOM_RAIL: bottom rail
            voltage: -8.0 to 8.0V

ADC (Analog-to-Digital Converter):
  jumperless.adc_get(channel)                  - Read ADC input voltage
  jumperless.get_adc(channel)                  - Alias for adc_get

                                              channel: 0-4

INA (Current/Power Monitor):
  jumperless.ina_get_current(sensor)          - Read current in amps
  jumperless.ina_get_voltage(sensor)          - Read shunt voltage
  jumperless.ina_get_bus_voltage(sensor)      - Read bus voltage
  jumperless.ina_get_power(sensor)            - Read power in watts
  Aliases: get_current, get_voltage, get_bus_voltage, get_power

             sensor: 0 or 1

GPIO:
  jumperless.gpio_set(pin, value)             - Set GPIO pin state
  jumperless.gpio_get(pin)                    - Read GPIO pin state
  jumperless.gpio_set_dir(pin, direction)     - Set GPIO pin direction
  jumperless.gpio_get_dir(pin)                - Get GPIO pin direction
  jumperless.gpio_set_pull(pin, pull)         - Set GPIO pull-up/down
  jumperless.gpio_get_pull(pin)               - Get GPIO pull-up/down
  Aliases: set_gpio, get_gpio, set_gpio_dir, get_gpio_dir, etc.

            pin 1-8: GPIO 1-8
            pin   9: UART Tx
            pin  10: UART Rx
              value: True/False   for HIGH/LOW
          direction: True/False   for OUTPUT/INPUT
               pull: -1/0/1       for PULL_DOWN/NONE/PULL_UP

Node Connections:
  jumperless.connect(node1, node2)            - Connect two nodes
  jumperless.disconnect(node1, node2)         - Disconnect nodes
  jumperless.is_connected(node1, node2)       - Check if nodes are connected

  jumperless.nodes_clear()                    - Clear all connections
         set node2 to -1 to disconnect everything connected to node1

OLED Display:
  jumperless.oled_print("text")               - Display text
  jumperless.oled_clear()                     - Clear display
  jumperless.oled_connect()                   - Connect OLED
  jumperless.oled_disconnect()                - Disconnect OLED

Clickwheel:
  jumperless.clickwheel_up([clicks])          - Scroll up
  jumperless.clickwheel_down([clicks])        - Scroll down
  jumperless.clickwheel_press()               - Press button
           clicks: number of steps

Status:
  jumperless.print_bridges()                  - Print all bridges
  jumperless.print_paths()                    - Print path between nodes
  jumperless.print_crossbars()                - Print crossbar array
  jumperless.print_nets()                     - Print nets
  jumperless.print_chip_status()              - Print chip status

Probe Functions:
  jumperless.probe_read([blocking=True])      - Read probe (default: blocking)
  jumperless.read_probe([blocking=True])      - Read probe (default: blocking)
  jumperless.probe_read_blocking()            - Wait for probe touch (explicit)
  jumperless.probe_read_nonblocking()         - Check probe immediately (explicit)
  jumperless.get_button([blocking=True])      - Get button state (default: blocking)
  jumperless.probe_button([blocking=True])    - Get button state (default: blocking)
  jumperless.probe_button_blocking()          - Wait for button press (explicit)
  jumperless.probe_button_nonblocking()       - Check buttons immediately (explicit)
  Touch aliases: probe_wait, wait_probe, probe_touch, wait_touch (always blocking)
  Button aliases: button_read, read_button (parameterized)
  Non-blocking only: check_button, button_check
  Touch returns: ProbePad object (1-60, D13_PAD, TOP_RAIL_PAD, LOGO_PAD_TOP, etc.)
  Button returns: CONNECT, REMOVE, or NONE (front=connect, rear=remove)

Misc:
  jumperless.arduino_reset()                  - Reset Arduino
  jumperless.probe_tap(node)                  - Tap probe on node (unimplemented)
  jumperless.run_app(appName)                 - Run app
  jumperless.format_output(True/False)        - Enable/disable formatted output

Help:
  jumperless.help()                           - Display this help

Node Names:
  jumperless.node("TOP_RAIL")                  - Create node from string name
  jumperless.TOP_RAIL                        - Pre-defined node constant
  jumperless.D2, jumperless.A0, etc.         - Arduino pin constants
  For global access: from jumperless_nodes import *
  Node names: All standard names like "D13", "A0", "GPIO_1", etc.

Examples (all functions available globally):
  dac_set(TOP_RAIL, 3.3)                     # Set Top Rail to 3.3V using node
  set_dac(3, 3.3)                            # Same as above using alias
  dac_set(DAC0, 5.0)                         # Set DAC0 using node constant
  voltage = get_adc(1)                       # Read ADC1 using alias
  connect(TOP_RAIL, D13)                     # Connect using constants
  connect("TOP_RAIL", 5)                      # Connect using strings
  connect(4, 20)                             # Connect using numbers
  top_rail = node("TOP_RAIL")                 # Create node object
  connect(top_rail, D13)                     # Mix objects and constants
  oled_print("Fuck you!")                    # Display text
  current = get_current(0)                   # Read current using alias
  set_gpio(1, True)                          # Set GPIO pin high using alias
  pad = probe_read()                         # Wait for probe touch
  if pad == 25: print('Touched pad 25!')    # Check specific pad
  if pad == D13_PAD: connect(D13, TOP_RAIL)  # Auto-connect Arduino pin
  if pad == TOP_RAIL_PAD: show_voltage()     # Show rail voltage
  if pad == LOGO_PAD_TOP: print('Logo!')    # Check logo pad
  button = get_button()                      # Wait for button press (blocking)
  if button == CONNECT_BUTTON: ...          # Front button pressed
  if button == REMOVE_BUTTON: ...           # Rear button pressed
  button = check_button()                   # Check buttons immediately
  if button == BUTTON_NONE: print('None')   # No button pressed
  pad = wait_touch()                        # Wait for touch
  btn = check_button()                      # Check button immediately
  if pad == D13_PAD and btn == CONNECT_BUTTON: connect(D13, TOP_RAIL)

Note: All functions and constants are available globally!
No need for 'jumperless.' prefix in REPL or single commands.

>>> 
```
</details>


<details> 
<summary> Expand for the entire output of nodes_help() </summary>

```python
>>> nodes_help()
Jumperless Node Reference
========================

NODE TYPES:
  Numbered:     1-60 (breadboard)
  Arduino:      D0-D13, A0-A7 (nano header)
  GPIO:         GPIO_1-GPIO_8 (routable GPIO)
  Power:        TOP_RAIL, BOTTOM_RAIL, GND
  DAC:          DAC0, DAC1 (analog outputs)
  ADC:          ADC0-ADC4, PROBE (analog inputs)
  Current:      ISENSE_PLUS, ISENSE_MINUS
  UART:         UART_TX, UART_RX
  Buffer:       BUFFER_IN, BUFFER_OUT

THREE WAYS TO USE NODES:

1. NUMBERS (direct breadboard holes):
   connect(1, 30)                     # Connect holes 1 and 30
   connect(15, 42)                    # Any number 1-60

2. STRINGS (case-insensitive names):
   connect("D13", "TOP_RAIL")         # Arduino pin to power rail
   connect("gpio_1", "adc0")          # GPIO to ADC (case-insensitive)
   connect("15", "dac1")              # Mix numbers and names

3. CONSTANTS (pre-defined objects):
   connect(TOP_RAIL, D13)            # Using imported constants
   connect(GPIO_1, A0)               # No quotes needed
   connect(DAC0, 25)                 # Mix constants and numbers

MIXED USAGE:
   my_pin = "D13"                    # Create node object from string
   connect(my_pin, TOP_RAIL)         # Use node object with constant
   oled_print(my_pin)                # Display shows 'D13'

COMMON ALIASES (many names work for same node):
   "TOP_RAIL" = "T_R"
   "GPIO_1" = "GPIO1" = "GP1"
   "DAC0" = "DAC_0"
   "UART_TX" = "TX"

NOTES:
  - String names are case-insensitive: "d13" = "D13" = "nAnO_d13"
  - Constants are case-sensitive: use D13, not d13
  - All three methods work in any function
```

![Screenshot 2025-07-04 at 8 27 39 PM](https://github.com/user-attachments/assets/8d8dfc16-0dca-4ab8-9bcf-c511415bffc7)

</details>

   
 

### Formatted Output and Custom Types
The Jumperless module provides formatted output for better readability:

```python
# GPIO state returns formatted strings
state = gpio_get(1)           # Returns "HIGH" or "LOW"
direction = gpio_get_dir(1)   # Returns "INPUT" or "OUTPUT"
pull = gpio_get_pull(1)       # Returns "PULLUP", "PULLDOWN", or "NONE"

# Connection status returns formatted strings
connected = is_connected(1, 5) # Returns "CONNECTED" (truthy) or "DISCONNECTED" (falsey)

# Voltage and current readings are automatically formatted
voltage = adc_get(0)          # Returns float (e.g., 3.300)
current = ina_get_current(0)  # Returns float in A (e.g., 0.0123)
power = ina_get_power(0)      # Returns float in W (e.g., 0.4567)

# All functions work with both numbers and string aliases
gpio_set_dir("GPIO_1", True)  # Same as gpio_set_dir(1, True)
connect("TOP_RAIL", "GPIO_1") # Same as connect(101, 131)
```
![Screenshot 2025-07-04 at 8 16 04 PM](https://github.com/user-attachments/assets/4ae5e7e2-845a-4e6e-bbd9-5c328624cfe9)



## Writing Python Scripts

### Basic Script Structure
```python
"""
My Jumperless Script
Description of what this script does
"""

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
TOP_RAIL_GND = 104    # Also: TOP_GND (not actually routable but included for PADs)
BOTTOM_RAIL_GND = 126 # Also: BOT_GND, BOTTOM_GND (not actually routable but included for PADs)

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

# Arduino Nano non-routable hardwired connections
VIN = 69              # Unconnected to anything
RST0 = 94             # Hardwired to GPIO 18 on the RP2350
RST1 = 95             # Hardwired to GPIO 19 on the RP2350
N_GND0 = 97           # GND
N_GND1 = 96           # GND
NANO_5V = 99          # Hardwired to USB 5V bus (can also be used to power the Jumperless)
NANO_3V3 = 98         # Unconnected (without bridging the solder jumper on the back)

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
│   └── README.md
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
