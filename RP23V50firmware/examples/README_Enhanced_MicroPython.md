# Enhanced MicroPython for Jumperless V5

This document describes the enhanced MicroPython implementation for Jumperless V5 that adds:

1. **Runtime Module Import** - Import .py and .mpy modules from the filesystem
2. **Machine Module Support** - Standard MicroPython machine classes integrated with Jumperless hardware  
3. **Filesystem Integration** - Full filesystem access for scripts and modules
4. **Backward Compatibility** - All existing Jumperless functions continue to work

## New Features

### 🔧 Machine Module Support

The standard MicroPython `machine` module is now available with Jumperless-specific integration:

#### Pin Class
Control GPIO pins using node numbers or names:

```python
from machine import Pin

# Using node numbers (breadboard nodes)
pin1 = Pin(1, Pin.OUT)       # Breadboard node 1
pin13 = Pin(13, Pin.OUT)     # Breadboard node 13

# Using GPIO numbers directly  
pin_gpio = Pin(26, Pin.IN)   # RP2350 GPIO 26

# Pin operations
pin1.on()                    # Set HIGH
pin1.off()                   # Set LOW
state = pin1.value()         # Read state (0 or 1)
pin1.value(1)                # Set state

# Pin modes
pin_in = Pin(2, Pin.IN)                    # Input
pin_out = Pin(3, Pin.OUT, value=1)         # Output, initially HIGH
pin_pullup = Pin(4, Pin.IN, Pin.PULL_UP)   # Input with pullup
```

#### ADC Class
Read analog values from ADC channels:

```python
from machine import ADC

# Create ADC on GPIO 26 (NANO_A0 equivalent)
adc0 = ADC(26)              # GPIO 26 = ADC channel 0
adc1 = ADC(27)              # GPIO 27 = ADC channel 1  
adc2 = ADC(28)              # GPIO 28 = ADC channel 2
adc3 = ADC(29)              # GPIO 29 = ADC channel 3

# Read values
reading = adc0.read_u16()   # Returns 0-65535
voltage = (reading / 65535) * 3.3  # Convert to voltage
```

#### PWM Class (for DAC)
Generate PWM signals and control DAC outputs:

```python
from machine import Pin, PWM

# Regular PWM on any pin
pwm = PWM(Pin(2))
pwm.duty_u16(32768)  # 50% duty cycle

# DAC control via PWM (TOP_RAIL and BOTTOM_RAIL)
dac_top = PWM(Pin(16))      # TOP_RAIL (DAC channel 2)
dac_bottom = PWM(Pin(17))   # BOTTOM_RAIL (DAC channel 3)

# Set voltage via duty cycle (-8V to +8V range)
# 0V = duty 32768, 3.3V = duty 46244, -3.3V = duty 19289
duty_3v3 = int(((3.3 + 8.0) / 16.0) * 65535)
dac_top.duty_u16(duty_3v3)  # Set TOP_RAIL to 3.3V
```

### 📁 Filesystem and Module Import

#### Module Search Paths
MicroPython now searches these directories for modules:

```python
import sys
print(sys.path)
# Output:
# ['', '/python_scripts', '/python_scripts/lib', '/python_scripts/modules', '/lib', '/modules']
```

#### Creating Custom Modules
Create `.py` files in `python_scripts/lib/` for automatic import:

**File: `python_scripts/lib/sensors.py`**
```python
# Custom sensor library
from machine import ADC

def read_all_adcs():
    """Read all 4 ADC channels and return voltages"""
    voltages = []
    for channel in range(4):
        adc = ADC(26 + channel)
        reading = adc.read_u16()
        voltage = (reading / 65535) * 3.3
        voltages.append(voltage)
    return voltages

def temperature_from_adc(adc_channel, r_ref=10000):
    """Convert ADC reading to temperature (assuming thermistor)"""
    adc = ADC(26 + adc_channel)
    reading = adc.read_u16()
    voltage = (reading / 65535) * 3.3
    
    # Simple thermistor calculation (customize for your sensor)
    if voltage > 0:
        resistance = r_ref * (3.3 - voltage) / voltage
        # Steinhart-Hart equation (simplified)
        temp_k = 1.0 / (0.001129148 + 0.000234125 * log(resistance) + 0.0000000876741 * log(resistance)**3)
        temp_c = temp_k - 273.15
        return temp_c
    return 0.0
```

**Usage in REPL:**
```python
import sensors

# Read all ADC channels
voltages = sensors.read_all_adcs()
print(f"ADC voltages: {voltages}")

# Get temperature from ADC 0
temp = sensors.temperature_from_adc(0)
print(f"Temperature: {temp:.1f}°C")
```

#### Importing .mpy Files
Compiled MicroPython bytecode files (.mpy) are also supported for faster loading:

1. Compile Python to bytecode: `mpy-cross your_module.py`
2. Copy `your_module.mpy` to `python_scripts/lib/`
3. Import normally: `import your_module`

### 🔗 Backward Compatibility

All existing Jumperless functions continue to work exactly as before:

```python
# Traditional Jumperless functions (still work)
connect(1, 30)
disconnect(1, 30)
dac_set(TOP_RAIL, 3.3)
voltage = adc_get(0)
gpio_set(13, 1)
is_connected(1, 30)

# Node constants still available
TOP_RAIL, BOTTOM_RAIL, DAC0, DAC1, etc.

# All function aliases work
set_dac(TOP_RAIL, 3.3)      # Same as dac_set()
get_adc(0)                  # Same as adc_get()
```

### 🎯 Combining Both Approaches

Mix traditional Jumperless functions with machine module:

```python
from machine import Pin, ADC, PWM

# Set up connections using traditional functions
connect(TOP_RAIL, 1)       # Connect top rail to breadboard node 1
connect(5, 30)             # Connect breadboard nodes 5 and 30

# Control hardware using machine module
led = Pin(13, Pin.OUT)     # LED on breadboard node 13
sensor = ADC(26)           # Sensor on ADC channel 0
dac = PWM(Pin(16))         # DAC control for top rail

# Main loop
while True:
    # Read sensor
    reading = sensor.read_u16()
    voltage = (reading / 65535) * 3.3
    
    # Control LED based on sensor
    if voltage > 1.5:
        led.on()
    else:
        led.off()
    
    # Set DAC output based on sensor
    dac_voltage = voltage * 2  # Scale up
    duty = int(((dac_voltage + 8.0) / 16.0) * 65535)
    dac.duty_u16(duty)
    
    time.sleep(0.1)
```

## Pin Mapping Reference

### Node to GPIO Mapping

| Node Name | Node # | GPIO | Function |
|-----------|--------|------|----------|
| 1-30 | 1-30 | 0-31 | Breadboard nodes |
| NANO_D0 | 60 | 0 | UART RX |
| NANO_D1 | 61 | 1 | UART TX |
| NANO_D2 | 62 | 2 | Digital I/O |
| NANO_D3 | 63 | 3 | Digital I/O, PWM |
| NANO_D13 | 73 | 13 | Digital I/O, LED |
| NANO_A0 | 100 | 26 | ADC0 |
| NANO_A1 | 101 | 27 | ADC1 |
| NANO_A2 | 102 | 28 | ADC2 |
| NANO_A3 | 103 | 29 | ADC3 |
| NANO_A4 | 104 | 4 | I2C SDA |
| NANO_A5 | 105 | 5 | I2C SCL |
| TOP_RAIL | 101 | 16 | DAC channel 2 |
| BOTTOM_RAIL | 102 | 17 | DAC channel 3 |
| DAC0 | 106 | 16 | DAC channel 2 |
| DAC1 | 107 | 17 | DAC channel 3 |

### Communication Interfaces

| Interface | Pins | GPIO |
|-----------|------|------|
| I2C0 | SDA=4, SCL=5 | Primary I2C |
| I2C1 | SDA=22, SCL=23 | Secondary I2C |
| SPI0 | MOSI=23, MISO=20, SCK=22, CS=21 | Primary SPI |
| SPI1 | MOSI=27, MISO=24, SCK=26, CS=25 | Secondary SPI |
| UART0 | TX=0, RX=1 | USB Serial |
| UART1 | TX=24, RX=25 | Hardware Serial |

## Example Projects

### 1. Simple LED Blink
```python
from machine import Pin
import time

led = Pin(13, Pin.OUT)

while True:
    led.on()
    time.sleep(0.5)
    led.off()
    time.sleep(0.5)
```

### 2. Analog Sensor Monitor
```python
from machine import ADC
import time

sensor = ADC(26)  # ADC0

while True:
    reading = sensor.read_u16()
    voltage = (reading / 65535) * 3.3
    print(f"Sensor: {voltage:.3f}V")
    time.sleep(1)
```

### 3. Voltage Controller
```python
from machine import Pin, ADC, PWM
import time

# Set up hardware
sensor = ADC(26)           # Input sensor
output_dac = PWM(Pin(16))  # TOP_RAIL DAC
led = Pin(13, Pin.OUT)     # Status LED

target_voltage = 2.0  # Target 2V output

while True:
    # Read input
    reading = sensor.read_u16()
    input_voltage = (reading / 65535) * 3.3
    
    # Calculate output (simple proportional control)
    error = target_voltage - input_voltage
    output_voltage = target_voltage + (error * 0.5)  # Simple P controller
    
    # Clamp output to valid range (-8V to +8V)
    output_voltage = max(-8, min(8, output_voltage))
    
    # Set DAC output
    duty = int(((output_voltage + 8.0) / 16.0) * 65535)
    output_dac.duty_u16(duty)
    
    # Status indicator
    if abs(error) < 0.1:
        led.on()   # Close to target
    else:
        led.off()  # Adjusting
    
    print(f"In: {input_voltage:.2f}V, Out: {output_voltage:.2f}V, Error: {error:.2f}V")
    time.sleep(0.1)
```

## Development Tips

### 1. Module Development
- Place reusable code in `python_scripts/lib/`
- Use descriptive module names
- Include docstrings and comments
- Test modules in REPL before using in scripts

### 2. Debugging
```python
# Enable verbose output
import sys
print(f"Python version: {sys.version}")
print(f"Module paths: {sys.path}")

# Check available modules
import os
print("Available modules:")
for path in sys.path:
    try:
        files = os.listdir(path)
        print(f"  {path}: {files}")
    except:
        print(f"  {path}: (not accessible)")
```

### 3. Performance
- Use .mpy files for large modules (faster loading)
- Avoid complex imports in tight loops
- Cache frequently-used objects

### 4. Memory Management
```python
import gc

# Check memory usage
print(f"Free memory: {gc.mem_free()} bytes")
print(f"Allocated memory: {gc.mem_alloc()} bytes")

# Force garbage collection
gc.collect()
```

## Migration Guide

### From Legacy Jumperless Code
1. **Keep existing code** - it will continue to work
2. **Gradually adopt machine module** for new features
3. **Create modules** for commonly-used functions
4. **Mix approaches** as needed

### Example Migration
**Before:**
```python
# Legacy approach
gpio_set(13, 1)
voltage = adc_get(0)
dac_set(TOP_RAIL, voltage)
```

**After (mixed approach):**
```python
# Mixed approach - use what's most convenient
from machine import Pin, ADC, PWM

led = Pin(13, Pin.OUT)
sensor = ADC(26)
# Still use legacy DAC function (simpler for this case)
voltage = sensor.read_u16() / 65535 * 3.3
led.on() if voltage > 1.5 else led.off()
dac_set(TOP_RAIL, voltage)  # Legacy function still works
```

## Troubleshooting

### Common Issues

**1. Module not found**
```
ImportError: No module named 'my_module'
```
- Check file is in `python_scripts/lib/`
- Verify filename matches import name
- Check for typos

**2. Machine module not available**
```
ImportError: No module named 'machine'
```
- Ensure you're using enhanced MicroPython build
- Check board configuration includes machine module

**3. Pin name not recognized**
```
ValueError: Unknown pin name
```
- Use GPIO numbers instead of names in current implementation
- Refer to pin mapping table above

**4. Out of memory**
```
MemoryError: memory allocation failed
```
- Use `gc.collect()` to free memory
- Reduce module imports
- Use .mpy files instead of .py

### Getting Help

1. **Check examples** - Run `simple_machine_example.py`
2. **Use help()** - `help(Pin)`, `help(ADC)`, etc.  
3. **Check memory** - `gc.mem_free()`
4. **Verify paths** - `print(sys.path)`

## Technical Implementation

### Files Modified/Added
- `lib/micropython/boards/JUMPERLESS_V5/mpconfigboard.h` - Board configuration
- `lib/micropython/boards/JUMPERLESS_V5/pins.csv` - Pin definitions
- `lib/micropython/port/mpconfigport.h` - Port configuration  
- `lib/micropython/port/machine_jumperless.c` - Machine module implementation
- `src/Python_Proper.cpp` - Filesystem and path setup

### Build System Integration
- Machine module automatically registered via `MP_REGISTER_MODULE`
- Board definitions include peripheral mappings
- Filesystem mounted and paths configured at startup
- QSTR definitions auto-generated during build

This enhanced MicroPython implementation provides a powerful, flexible platform for Jumperless development while maintaining complete backward compatibility with existing code. 