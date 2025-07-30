# Slow PWM Help Documentation Updates

## Overview
Updated the onboard help documentation to include the new slow PWM functionality and additional functions.

## Changes Made

### 1. Voltage Category Help (`help voltage`)
- **Added PWM Signal Generation section** with Python examples:
  - `jumperless.pwm(1, 1000, 0.5)` - 1kHz PWM on GPIO_1, 50% duty
  - `jumperless.pwm(2, 0.1, 0.25)` - 0.1Hz slow PWM on GPIO_2, 25% duty
  - `jumperless.pwm_set_frequency(1, 500)` - Change frequency to 500Hz
  - `jumperless.pwm_set_duty_cycle(1, 0.75)` - Change duty cycle to 75%
  - `jumperless.pwm_stop(1)` - Stop PWM on GPIO_1

- **Added PWM Features section**:
  - Hardware PWM: 10Hz to 62.5MHz (high precision)
  - Slow PWM: 0.001Hz to 10Hz (hardware timer based)
  - Automatic mode selection based on frequency
  - Ultra-slow PWM: 0.001Hz = 1000 second period!

### 2. Python Category Help (`help python`)
- **Added to Hardware Control section**:
  - `jumperless.pwm(1, 1000, 0.5)` - PWM output (1kHz, 50% duty)
  - `jumperless.pause_core2()` - Pause core2 processing
  - `jumperless.send_raw(data)` - Send raw data to core2

### 3. Advanced Category Help (`help advanced`)
- **Added Advanced Python Functions section**:
  - `jumperless.pause_core2()` - Pause core2 processing
  - `jumperless.send_raw(data)` - Send raw data to core2
  - `jumperless.pwm(1, 0.001, 0.5)` - Ultra-slow PWM (0.001Hz)

## Implementation Accuracy Verification

### Frequency Ranges
- **MicroPython Interface**: 0.001Hz to 62.5MHz ✅
- **C++ Implementation**: 
  - Hardware PWM: 10Hz to 62.5MHz ✅
  - Slow PWM: 0.001Hz to 10Hz ✅ (updated from 0.01Hz)
- **Automatic Mode Selection**: 
  - Below 10Hz: Uses slow PWM (hardware timer)
  - 10Hz and above: Uses hardware PWM

### Function Availability
- **PWM Functions**: Available through MicroPython interface only
- **pause_core2()**: Available through MicroPython interface
- **send_raw()**: Available through MicroPython interface

### Documentation Accuracy
- All frequency ranges match implementation ✅
- All function signatures match actual implementation ✅
- Examples use correct syntax and parameters ✅
- Cross-referenced with actual code implementation ✅

## Usage Examples

### Basic PWM Usage
```python
# Hardware PWM (10Hz+)
jumperless.pwm(1, 1000, 0.5)  # 1kHz, 50% duty cycle

# Slow PWM (0.001Hz to 10Hz)
jumperless.pwm(2, 0.1, 0.25)  # 0.1Hz, 25% duty cycle (10 second period)
jumperless.pwm(3, 0.001, 0.5) # 0.001Hz, 50% duty cycle (1000 second period!)
```

### Advanced Functions
```python
# Pause core2 processing
jumperless.pause_core2(True)   # Pause
jumperless.pause_core2(False)  # Resume

# Send raw data to core2
jumperless.send_raw("A", 1, 2, 1)  # Set chip A, position (1,2)
```

## Notes
- PWM functions are only available through the MicroPython interface
- No direct terminal commands for PWM (unlike DAC/ADC which have `^` and `v` commands)
- Slow PWM uses hardware timers for precise timing below 10Hz
- Hardware PWM provides high precision for frequencies 10Hz and above 