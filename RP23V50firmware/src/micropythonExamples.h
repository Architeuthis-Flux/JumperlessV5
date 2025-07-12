#ifndef MICROPYTHON_EXAMPLES_H
#define MICROPYTHON_EXAMPLES_H

//==============================================================================
// MicroPython Examples - Compile-time Configuration
//==============================================================================

// Define which examples to include at compile time
// Comment out any example you don't want to include
#define INCLUDE_DAC_BASICS
#define INCLUDE_ADC_BASICS  
#define INCLUDE_GPIO_BASICS
#define INCLUDE_NODE_CONNECTIONS
#define INCLUDE_README
//#define INCLUDE_TEST_RUNNER
#define INCLUDE_LED_BRIGHTNESS_CONTROL
#define INCLUDE_VOLTAGE_MONITOR
#define INCLUDE_STYLOPHONE

// Convenience defines to disable groups of examples
// Uncomment any of these to disable entire categories
// #define DISABLE_ALL_EXAMPLES
// #define DISABLE_HARDWARE_EXAMPLES    // Disables DAC, ADC, GPIO, Node examples
// #define DISABLE_DEMO_EXAMPLES        // Disables LED, Voltage, Stylophone demos
// #define DISABLE_UTILITY_EXAMPLES     // Disables README and test runner

#ifdef DISABLE_ALL_EXAMPLES
    #undef INCLUDE_DAC_BASICS
    #undef INCLUDE_ADC_BASICS
    #undef INCLUDE_GPIO_BASICS
    #undef INCLUDE_NODE_CONNECTIONS
    #undef INCLUDE_README
    #undef INCLUDE_TEST_RUNNER
    #undef INCLUDE_LED_BRIGHTNESS_CONTROL
    #undef INCLUDE_VOLTAGE_MONITOR
    #undef INCLUDE_STYLOPHONE
#endif

#ifdef DISABLE_HARDWARE_EXAMPLES
    #undef INCLUDE_DAC_BASICS
    #undef INCLUDE_ADC_BASICS
    #undef INCLUDE_GPIO_BASICS
    #undef INCLUDE_NODE_CONNECTIONS
#endif

#ifdef DISABLE_DEMO_EXAMPLES
    #undef INCLUDE_LED_BRIGHTNESS_CONTROL
    #undef INCLUDE_VOLTAGE_MONITOR
    #undef INCLUDE_STYLOPHONE
#endif

#ifdef DISABLE_UTILITY_EXAMPLES
    #undef INCLUDE_README
    #undef INCLUDE_TEST_RUNNER
#endif

//==============================================================================
// MicroPython Examples - Individual Demo Files
//==============================================================================

#ifdef INCLUDE_DAC_BASICS
const char* DAC_BASICS_PY = R"("""
Basic DAC (Digital-to-Analog Converter) operations.
This example shows how to set DAC voltages.

Hardware Setup:
1. Connect voltmeter or LED to DAC output pins
2. DAC channels: 0=DAC_A, 1=DAC_B, 2=TOP_RAIL, 3=BOTTOM_RAIL
"""

print("DAC Basics Demo")

# Test all DAC channels
channels = [0, 1, 2, 3]
channel_names = ["DAC_A", "DAC_B", "TOP_RAIL", "BOTTOM_RAIL"]

for i, channel in enumerate(channels):
    print("\nTesting " + channel_names[i] + " (channel " + str(channel) + "):")
    
    # Set different voltages
    voltages = [0.0, 1.65, 3.3]
    for voltage in voltages:
        dac_set(channel, voltage)
        actual = dac_get(channel)
        print("  Set: " + str(voltage) + "V, Read: " + str(round(actual, 3)) + "V")
        time.sleep(1)
    
    # Reset to 0V
    dac_set(channel, 0.0)

print("\nDAC Basics complete!")

)";
#endif

#ifdef INCLUDE_ADC_BASICS
const char* ADC_BASICS_PY = R"("""
Basic ADC (Analog-to-Digital Converter) operations.
This example shows how to read analog voltages.
"""

print("ADC Basics Demo")
    
# Read all ADC channels
channels = [0, 1, 2, 3]

print("Reading ADC channels (Ctrl+Q to stop):")
print("Connect voltage sources to ADC inputs")
    
    
while True:
    print("\nADC Readings:")
    for channel in channels:
        voltage = adc_get(channel)
        print("  ADC" + str(channel) + ": " + str(round(voltage, 3)) + "V")
    time.sleep(0.5)
            
)";
#endif

#ifdef INCLUDE_GPIO_BASICS
const char* GPIO_BASICS_PY = R"("""
Basic GPIO (General Purpose Input/Output) operations.
This example shows digital I/O, direction control, and pull resistors.
"""

print("GPIO Basics Demo")

# Test GPIO pin 1
pin = 1
print("Testing GPIO pin " + str(pin))

# Set as output
gpio_set_dir(pin, True)  # True = OUTPUT
print("Set as output")

# Blink test
print("Blinking 5 times...")
for i in range(5):
    gpio_set(pin, True)   # HIGH
    print("  GPIO" + str(pin) + " = HIGH")
    time.sleep(0.5)
    
    gpio_set(pin, False)  # LOW
    print("  GPIO" + str(pin) + " = LOW")
    time.sleep(0.5)

# Set as input
gpio_set_dir(pin, False)  # False = INPUT
print("Set as input")

# Test pull resistors
pulls = [0, 1, -1]  # None, Up, Down
pull_names = ["NONE", "PULLUP", "PULLDOWN"]

for i, pull in enumerate(pulls):
    gpio_set_pull(pin, pull)
    state = gpio_get(pin)
    print("Pull " + pull_names[i] + ": " + str(state))
    time.sleep(1)

print("GPIO Basics complete!")

)";
#endif

#ifdef INCLUDE_NODE_CONNECTIONS
const char* NODE_CONNECTIONS_PY = R"("""
Node connection and routing operations.
This example shows how to connect/disconnect nodes and check connections.
"""

print("Node Connections Demo")
    
# Clear all existing connections
nodes_clear()
print("Cleared all connections")

# Test connections
test_connections = [
    (1, 30),
    (15, 45),
    (DAC0, 20),
    (GPIO_1, 25)
]

for node1, node2 in test_connections:
    print("\nConnecting " + str(node1) + " to " + str(node2))
    
    # Connect nodes
    connect(node1, node2)
    
    # Check connection
    connected = is_connected(node1, node2)
    print("Is connected: " + str(connected))
    
    time.sleep(0.5)
    
    # Disconnect
    disconnect(node1, node2)
    
    # Verify disconnection
    connected = is_connected(node1, node2)
    print("Is connected: " + str(connected))
    
    time.sleep(0.5)

# Show final status
print("\nFinal status:")
print_bridges()

print("Node Connections complete!")
nodes_clear()

)";
#endif

#ifdef INCLUDE_README
const char* README_MD = R"(# Jumperless MicroPython Examples

This directory contains example scripts demonstrating various Jumperless features.

## Examples

### Hardware Basics
- `01_dac_basics.py` - DAC operations and voltage setting
- `02_adc_basics.py` - ADC operations and voltage reading  
- `03_gpio_basics.py` - Digital I/O and GPIO control
- `04_node_connections.py` - Node routing and connections

### Interactive Demos
- `led_brightness_control.py` - Touch-controlled LED brightness
- `voltage_monitor.py` - Real-time voltage monitoring
- `stylophone.py` - Musical instrument using probe

### Utilities
- `test_examples.py` - Run all examples in sequence

## Usage

Load and run any example:
```python
exec(open('examples/01_dac_basics.py').read())
```

Or run individual functions:
```python
from examples.dac_basics import dac_basics
dac_basics()
```

## Hardware Setup

Each example includes specific hardware setup instructions.
General requirements:
- Jumperless board
- Breadboard for connections
- Basic components (LEDs, resistors, etc.)

## Getting Help

- Type `help()` in the REPL for hardware commands
- Check example docstrings for detailed usage
- Refer to Jumperless documentation
)";
#endif

#ifdef INCLUDE_TEST_RUNNER
const char* TEST_RUNNER_PY = R"("""
Test runner for all Jumperless MicroPython examples.
Runs each example in sequence for verification.
"""

import time

def run_all_examples():
    print("Starting Jumperless MicroPython Examples Test")
    print("=" * 50)
    
    oled_clear()
    oled_print("Running All Examples")
    
    examples = [
        ("DAC Basics", "01_dac_basics.py"),
        ("ADC Basics", "02_adc_basics.py"),
        ("GPIO Basics", "03_gpio_basics.py"),
        ("Node Connections", "04_node_connections.py")
    ]
    
    for i, (name, filename) in enumerate(examples):
        print("\nExample " + str(i + 1) + "/4: " + name)
        print("-" * 40)
        
        try:
            exec(open(filename).read())
            print("Example completed successfully")
        except Exception as e:
            print("Example failed: " + str(e))
        
        if i < len(examples) - 1:
            print("Pausing 3 seconds before next example...")
            time.sleep(3)
    
    oled_clear()
    oled_print("All Examples Complete!")
    print("\nAll examples completed!")
    print("=" * 50)

def quick_test():
    print("Quick Jumperless Function Test")
    print("-" * 30)
    
    oled_clear()
    oled_print("Quick Test")
    
    # Quick DAC test
    print("DAC Test: Setting TOP_RAIL to 3.3V")
    dac_set(TOP_RAIL, 3.3)
    voltage = dac_get(TOP_RAIL)
    print("  TOP_RAIL: " + str(voltage) + "V")
    
    # Quick ADC test
    print("ADC Test: Reading ADC0")
    adc_voltage = adc_get(0)
    print("  ADC0: " + str(adc_voltage) + "V")
    
    # Quick GPIO test
    print("GPIO Test: Testing GPIO1")
    gpio_set_dir(1, True)  # Set as output
    gpio_set(1, True)
    state = gpio_get(1)
    print("  GPIO1: " + str(state))  # Convert GPIOState to string
    gpio_set(1, False)
    
    # Quick connection test
    print("Connection Test: Connecting rows 1-30")
    result = connect(1, 30)
    print("  Connect 1-30: " + str(result))  # Convert result to string
    
    connected = is_connected(1, 30)
    print("  Verification: " + str(connected))  # Convert result to string
    
    # Cleanup
    disconnect(1, 30)
    dac_set(TOP_RAIL, 0.0)
    
    oled_clear()
    oled_print("Quick Test Complete!")
    print("Quick test complete!")

if __name__ == "__main__":
    quick_test()
)";
#endif

#ifdef INCLUDE_LED_BRIGHTNESS_CONTROL
const char* LED_BRIGHTNESS_CONTROL_PY = R"("""
LED Brightness Control Demo
Touch breadboard pads 1-10 to control LED brightness levels.

Hardware Setup:
1. Connect LED anode to breadboard row 15
2. Connect LED cathode to GND
"""

print("LED Brightness Control Demo")
    
oled_print("LED Brightness")

print("Hardware Setup:")
print("  Connect LED anode to row 15")
print("  Connect LED cathode to GND")

connect(DAC0, 15)

while True:
    pad = probe_read_blocking()

    if pad:

        voltage = (float(pad) / 60.0) * 5.0
        dac_set(DAC0, voltage)
        print("\r                      ", end="\r")
        print(str(pad) + ": " + str(round(voltage, 1)) + "V", end="")
        oled_clear()
        oled_print("Bright: " + str(pad) + "/60")
            
    time.sleep(0.1)
    
)";
#endif

#ifdef INCLUDE_VOLTAGE_MONITOR
const char* VOLTAGE_MONITOR_PY = R"("""
Voltage Monitor Demo
Monitor voltage on ADC with real-time OLED display.

Hardware Setup:
1. Connect voltage source to breadboard row 20
2. Voltage range: 0V to 3.3V
"""

import time

print("Voltage Monitor Demo")

connect(ADC0, 20)
print("ADC0 connected to row 20")
print("Connect voltage source to row 20")

oled_print("Voltage Monitor")
time.sleep(1)

while True:
    voltage = adc_get(0)
    oled_print(str(round(voltage, 3)) + "V")
    print("\r                      ", end="\r")
    print("Voltage: " + str(round(voltage, 3)) + "V", end="")
    time.sleep(0.15)

)";
#endif

#ifdef INCLUDE_STYLOPHONE
const char* STYLOPHONE_PY = R"("""
Jumperless Stylophone
Musical instrument using probe and GPIO to generate audio tones.

Hardware Setup:
1. Connect speaker between rows 25 (positive) and 55 (negative)    
"""

import time

speaker_pos_row = 25
speaker_neg_row = 55
freq_multiplier = 40.0

def setup_audio():
    connect(GPIO_1, speaker_pos_row)
    connect(GND, speaker_neg_row)
    pwm(GPIO_1, 10, 0.5)
    print("Connect speaker: positive to row " + str(speaker_pos_row) + ", negative to row " + str(speaker_neg_row))
    time.sleep(0.1)

print("Jumperless Stylophone")
oled_print("Touch pads!")

while True:

    pad = probe_read_blocking()

    frequency = float(pad) * freq_multiplier
    pwm_set_frequency(GPIO_1, frequency)

    print("\r                                 ", end="\r")
    print("Pad: " + str(pad) + " " + str(frequency) + " Hz", end="")
    oled_print(str(frequency) + " Hz")

)";
#endif

#endif // MICROPYTHON_EXAMPLES_H 