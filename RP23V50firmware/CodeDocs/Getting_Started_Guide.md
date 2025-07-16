# Getting Started with Jumperless

A comprehensive beginner's guide to using the Jumperless breadboard automation system.

## What is Jumperless?

Jumperless is a revolutionary breadboard that eliminates physical jumper wires by using internal electronic switches to create connections. It combines the familiar breadboard experience with programmable connectivity, allowing you to:

- Create circuits without physical wires
- Program connections via terminal commands or Python
- Save and load circuit configurations
- Monitor power consumption and signals
- Control circuits programmatically

## What You'll Need

### Hardware
- Jumperless V5 board
- USB-C cable for power and communication
- Computer with terminal emulator or serial monitor
- Optional: Arduino Nano (for Arduino integration)
- Optional: Components for your circuits (LEDs, resistors, sensors, etc.)

### Software
- Terminal emulator (PuTTY on Windows, Terminal on Mac/Linux, or Arduino IDE Serial Monitor)
- USB drivers (usually installed automatically)
- Optional: Jumperless Bridge App for enhanced features

## First Power-Up

### 1. Connect Your Jumperless
1. Connect the USB-C cable to your Jumperless board
2. Connect the other end to your computer
3. The board should power up and show the startup animation on the OLED display
4. LEDs will illuminate showing the system is ready

### 2. Open Terminal Connection
**Using Arduino IDE Serial Monitor:**
1. Open Arduino IDE
2. Go to Tools → Port and select the Jumperless port
3. Open Tools → Serial Monitor
4. Set baud rate to 115200
5. You should see the colorful Jumperless menu

**Using Terminal (Mac/Linux):**
```bash
screen /dev/tty.usbmodem* 115200
```

**Using PuTTY (Windows):**
1. Open PuTTY
2. Select "Serial" connection type
3. Enter the COM port (check Device Manager)
4. Set speed to 115200
5. Click "Open"

### 3. First Menu
You should see a colorful menu with options like:
```
Menu

'help' for docs or [command]?

m = show this menu
n = show net list
b = show bridge array
c = show crossbar status
s = show all slot files
...
```

If you see this menu, congratulations! Your Jumperless is working correctly.

## Your First Connection

Let's create your first connection to understand how Jumperless works.

### Step 1: Connect Two Points
Type the following command and press Enter:
```
f 1-5
```

This connects breadboard hole 1 to hole 5. You should see:
- Colored output confirming the connection
- The OLED display updating to show the connection
- LEDs lighting up to indicate the active path

### Step 2: Verify the Connection
Check your connection with:
```
n
```

This shows the "netlist" - all current connections. You should see hole 1 and 5 listed as connected.

### Step 3: Add More Connections
Try adding more connections:
```
+ 10-15
+ 20-25
```

The `+` command adds connections without clearing existing ones.

### Step 4: Remove a Connection
Remove the first connection:
```
- 1-5
```

### Step 5: Clear All Connections
Clear everything to start fresh:
```
x
```

Type `y` to confirm when prompted.

## Understanding Node Names

Jumperless supports various types of connection points called "nodes":

### Breadboard Holes
- **Numbers 1-60:** Physical breadboard holes
- **Example:** `f 1-30` connects hole 1 to hole 30

### Arduino Pins
- **Digital pins:** `D0`, `D1`, `D2`, ..., `D13`
- **Analog pins:** `A0`, `A1`, `A2`, ..., `A7`
- **Example:** `f D2-A0` connects Arduino pin D2 to A0

### Power Rails
- **GND:** Ground rail
- **3V3:** 3.3V power rail
- **5V:** 5V power rail
- **TOP_RAIL:** Programmable top rail voltage
- **BOTTOM_RAIL:** Programmable bottom rail voltage

### GPIO Pins
- **GPIO_1** through **GPIO_8:** General purpose I/O pins
- **Example:** `f GPIO_1-10` connects GPIO 1 to hole 10

### Special Nodes
- **DAC0, DAC1:** Digital-to-analog converter outputs
- **ADC0-ADC4:** Analog-to-digital converter inputs

## Basic LED Circuit

Let's build a simple LED circuit to see Jumperless in action.

### What You'll Need
- 1 LED
- 1 resistor (220Ω to 1kΩ)
- Or just use the onboard components

### Method 1: Using GPIO (No External Components)
```
# Set GPIO 1 as output and turn it on
f GPIO_1-10
```

Then in Python mode (type `p`):
```python
import jumperless
jumperless.gpio_set_dir(1, True)  # Set as output
jumperless.gpio_set(1, True)      # Turn on (3.3V)
```

### Method 2: Using DAC (More Control)
```
# Connect DAC0 to hole 10
f DAC0-10
```

In Python:
```python
import jumperless
jumperless.dac_set(0, 3.3)  # Set DAC to 3.3V
jumperless.dac_set(0, 1.5)  # Dim the LED
jumperless.dac_set(0, 0)    # Turn off
```

### Method 3: Traditional Circuit with Components
If you have an LED and resistor:
```
# Connect power, LED, and ground
f 5V-1,1-10,15-GND

# Place LED between holes 10 and 15 (anode to 10)
# Place resistor between holes 1 and 10
```

## Working with Python

Jumperless includes a full MicroPython interpreter for programmatic control.

### Enter Python Mode
```
p
```

You'll see a Python prompt:
```
>>> 
```

### Basic Python Commands
```python
# Import the Jumperless module
import jumperless

# Create connections
jumperless.connect("D2", "A0")
jumperless.connect("GND", "30")

# Control voltage
jumperless.dac_set(0, 3.3)  # Set DAC 0 to 3.3V
voltage = jumperless.adc_get(0)  # Read voltage from ADC 0
print(f"Measured: {voltage}V")

# Control GPIO
jumperless.gpio_set(1, True)   # Set GPIO 1 HIGH
state = jumperless.gpio_get(1) # Read GPIO 1 state

# Display on OLED
jumperless.oled_print("Hello World!")

# Get help
jumperless.help()
```

### Exit Python Mode
```python
quit
```

Or press `Ctrl+D`.

## File Management

Jumperless has an onboard file system for storing scripts and configurations.

### Open File Manager
```
/
```

This opens an interactive file browser with:
- **Arrow keys:** Navigate
- **Enter:** Open files/directories
- **v:** View file contents
- **e:** Edit with eKilo editor
- **n:** Create new file
- **q:** Quit file manager

### Create a Python Script
1. Open file manager with `/`
2. Press `n` to create new file
3. Name it `my_script.py`
4. Edit with `e`
5. Write your Python code
6. Save and exit

### Run Your Script
In Python mode:
```python
exec(open('my_script.py').read())
```

## Monitor and Measure

Jumperless includes built-in measurement capabilities.

### Voltage Measurement
```
v 0    # Read ADC channel 0
v 1    # Read ADC channel 1
```

In Python:
```python
voltage = jumperless.adc_get(0)
print(f"ADC 0: {voltage}V")
```

### Current Monitoring
```python
# Read current consumption
current = jumperless.ina_get_current(0)
power = jumperless.ina_get_power(0)
print(f"Current: {current}A, Power: {power}W")
```

### Set Reference Voltages
```
^ 3.3    # Set DAC voltage to 3.3V
^ 5.0    # Set DAC voltage to 5.0V
^ 0      # Set to 0V (ground)
```

## Save and Load Configurations

### Save Current Setup
Jumperless automatically saves configurations as "slots".

### View Saved Slots
```
s
```

### Load a Slot
```
o 1    # Load slot 1
o 2    # Load slot 2
```

### Cycle Through Slots
```
<
```

## Using the Help System

Jumperless includes comprehensive built-in help.

### Main Help
```
help
```

Shows available help categories.

### Category Help
```
help basics    # Basic commands
help probe     # Probe usage
help voltage   # Power and measurement
help python    # MicroPython features
help debug     # Troubleshooting
```

### Command-Specific Help
```
f?     # Help for connection command
p?     # Help for Python REPL
v?     # Help for voltage measurement
```

## Advanced Features

### PWM Control
```python
# Generate PWM signal
jumperless.pwm(1, 1000, 0.5)  # GPIO 1, 1kHz, 50% duty
jumperless.pwm_set_duty_cycle(1, 0.25)  # Change to 25%
jumperless.pwm_stop(1)  # Stop PWM
```

### I2C Device Scanning
```
@ 21,22    # Scan I2C on GPIO 21(SDA) and 22(SCL)
@          # Scan default I2C pins
```

### Probe Interaction
The physical probe allows interactive connection/disconnection:

```python
# Wait for probe touch
pad = jumperless.probe_read()
print(f"You touched {pad}")

# Check probe buttons
button = jumperless.probe_button()
print(f"Button pressed: {button}")
```

## Troubleshooting

### Connection Issues
1. **Check node names:** Ensure spelling is correct
2. **View status:** Use `n` to see current connections
3. **Clear and retry:** Use `x` to clear all connections
4. **Check hardware:** Use `b` and `c` for hardware status

### Python Issues
1. **Syntax errors:** Check Python syntax carefully
2. **Module not found:** Ensure `import jumperless` is first
3. **REPL stuck:** Press `Ctrl+C` to interrupt, `Ctrl+D` to reset

### Hardware Issues
1. **No power:** Check USB connection
2. **No display:** Try `. ` to reconnect OLED
3. **Strange behavior:** Type `?` for system info

### Common Error Messages
- **"Invalid node":** Check node name spelling
- **"Connection failed":** Hardware routing issue, try again
- **"File not found":** Check file path in file manager

## Tips for Success

### 1. Start Simple
- Begin with basic connections
- Test each connection before adding more
- Use the `n` command frequently to verify

### 2. Use the Visual Feedback
- Watch the OLED display for connection status
- LED colors indicate different nets
- Terminal colors provide status information

### 3. Learn the Node Names
- Practice with breadboard holes (1-60)
- Remember Arduino pins (D0-D13, A0-A7)
- Use power rail names (GND, 5V, 3V3)

### 4. Leverage Python
- Use Python for complex logic
- Save frequently used scripts
- Take advantage of MicroPython libraries

### 5. Explore Help
- The help system is comprehensive
- Use `[command]?` for specific help
- Read error messages carefully

## What's Next?

### Advanced Topics to Explore
1. **Custom Applications:** Build apps using the app framework
2. **USB Mass Storage:** Share files with your computer
3. **Arduino Integration:** Connect Arduino Nano for more I/O
4. **Complex Circuits:** Multi-rail power systems
5. **Automation:** Python scripts for automated testing

### Resources
- [Jumperless API Reference](Jumperless_API_Reference.md) - Complete function documentation
- [Terminal Commands Reference](Terminal_Commands_Reference.md) - All command details
- [JFS Module Documentation](JFS_Module_Documentation.md) - File system operations
- [Example Scripts](Example_Scripts_Reference.md) - Ready-to-use examples

### Community
- GitHub repository for issues and contributions
- Share your projects and scripts
- Contribute to documentation and examples

## Quick Reference Card

### Essential Commands
```
f 1-5           # Connect holes 1 and 5
+ 10-15         # Add connection 10 to 15
- 1-5           # Remove connection 1 to 5
x               # Clear all connections
n               # Show current connections
m               # Show menu
help            # Get help
p               # Python mode
/               # File manager
```

### Python Essentials
```python
import jumperless
jumperless.connect("D2", "A0")
jumperless.dac_set(0, 3.3)
voltage = jumperless.adc_get(0)
jumperless.gpio_set(1, True)
jumperless.oled_print("Hello!")
```

### Node Names
```
1-60            # Breadboard holes
D0-D13          # Arduino digital pins
A0-A7           # Arduino analog pins
GPIO_1-GPIO_8   # GPIO pins
GND, 5V, 3V3    # Power rails
DAC0, DAC1      # DAC outputs
ADC0-ADC4       # ADC inputs
```

Welcome to the world of programmable breadboarding with Jumperless! 