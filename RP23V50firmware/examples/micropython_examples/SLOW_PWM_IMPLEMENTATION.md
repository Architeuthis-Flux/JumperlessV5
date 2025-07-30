# Slow PWM Implementation for RP2350

## Overview

The RP2350's hardware PWM peripheral has a minimum frequency limit of approximately 10Hz due to hardware constraints. This implementation provides a hardware timer-based PWM solution for frequencies below 10Hz, extending the usable frequency range down to 0.01Hz.

## Implementation Details

### Hardware Timer Approach

The slow PWM implementation uses the RP2350's hardware timer system with `repeating_timer_t` and `add_repeating_timer_ms()` to create precise timing intervals. This approach:

- Uses 1ms timer resolution for good precision
- Leverages the existing alarm pool infrastructure
- Provides hardware-level timing accuracy
- Maintains compatibility with existing PWM functions

### Key Components

#### 1. Global State Tracking
```c
// Slow PWM state tracking (for frequencies below 10Hz)
bool gpioSlowPWMEnabled[10];
repeating_timer_t gpioSlowPWMTimers[10];
volatile uint32_t gpioSlowPWMCounter[10];
volatile uint32_t gpioSlowPWMPeriod[10];
volatile uint32_t gpioSlowPWMDutyTicks[10];
```

#### 2. Timer Callback Function
```c
bool __not_in_flash_func(slowPWMTimerCallback)(repeating_timer_t* rt) {
    int gpio_index = (int)(uintptr_t)rt->user_data;
    
    gpioSlowPWMCounter[gpio_index]++;
    
    if (gpioSlowPWMCounter[gpio_index] >= gpioSlowPWMPeriod[gpio_index]) {
        gpioSlowPWMCounter[gpio_index] = 0;
    }
    
    int physical_pin = gpioDef[gpio_index][0];
    if (gpioSlowPWMCounter[gpio_index] < gpioSlowPWMDutyTicks[gpio_index]) {
        gpio_put(physical_pin, 1); // HIGH
    } else {
        gpio_put(physical_pin, 0); // LOW
    }
    
    return true; // Continue timer
}
```

#### 3. Automatic Mode Selection
The main `setupPWM()` function automatically selects the appropriate implementation:

```c
// For frequencies below 10Hz, use slow PWM with hardware timer
if (frequency < 10.0) {
    return setupSlowPWM(gpio_pin, frequency, duty_cycle);
}
```

## Frequency Range

| Mode | Frequency Range | Implementation |
|------|----------------|----------------|
| Slow PWM | 0.01Hz - 10Hz | Hardware Timer |
| Hardware PWM | 10Hz - 62.5MHz | PWM Peripheral |

## Usage Examples

### MicroPython
```python
import jumperless as jl

# Very slow PWM (0.1Hz = 10 second period)
jl.pwm(GPIO_1, 0.1, 0.5)  # 50% duty cycle

# Ultra-slow PWM (0.01Hz = 100 second period)
jl.pwm(GPIO_2, 0.01, 0.25)  # 25% duty cycle

# Dynamic frequency changes
jl.pwm_set_frequency(GPIO_1, 0.5)  # Change to 0.5Hz
jl.pwm_set_duty_cycle(GPIO_1, 0.75)  # Change to 75% duty cycle

# Stop PWM
jl.pwm_stop(GPIO_1)
```

### C++ API
```cpp
// Setup slow PWM
setupPWM(1, 0.1, 0.5);  // 0.1Hz, 50% duty cycle

// Change parameters
setPWMFrequency(1, 0.5);  // Change to 0.5Hz
setPWMDutyCycle(1, 0.75);  // Change to 75% duty cycle

// Stop PWM
stopPWM(1);
```

## Technical Specifications

### Timing Precision
- **Resolution**: 1ms timer intervals
- **Accuracy**: Hardware timer precision (±1ms)
- **Jitter**: Minimal due to hardware timer usage

### Resource Usage
- **Memory**: ~40 bytes per PWM channel
- **CPU**: Minimal overhead (timer callback only)
- **Timers**: One repeating timer per active slow PWM channel

### Limitations
- **Maximum Frequency**: 10Hz (hardware PWM takes over above this)
- **Minimum Frequency**: 0.01Hz (100 second period)
- **Concurrent Channels**: Limited by available timer resources

## Implementation Files

### Core Implementation
- `src/Peripherals.cpp` - Main PWM functions and slow PWM implementation
- `src/Peripherals.h` - Function declarations and state variables

### MicroPython Integration
- `modules/jumperless/modjumperless.c` - MicroPython PWM functions

### Examples
- `examples/micropython_examples/slow_pwm_demo.py` - Comprehensive demo
- `examples/micropython_examples/pwm_demo.py` - Updated with slow PWM examples

## State Management

The implementation maintains separate state tracking for slow PWM vs hardware PWM:

```c
// Hardware PWM state
bool gpioPWMEnabled[10];
float gpioPWMFrequency[10];
float gpioPWMDutyCycle[10];

// Slow PWM state
bool gpioSlowPWMEnabled[10];
volatile uint32_t gpioSlowPWMCounter[10];
volatile uint32_t gpioSlowPWMPeriod[10];
volatile uint32_t gpioSlowPWMDutyTicks[10];
```

## Error Handling

The implementation includes comprehensive error checking:

- **Invalid Pin**: Returns -1 for pins outside 1-8 range
- **Invalid Frequency**: Returns -2 for frequencies outside 0.01Hz-62.5MHz range
- **Invalid Duty Cycle**: Returns -3 for duty cycles outside 0.0-1.0 range
- **Timer Setup Failure**: Returns -4 if hardware timer cannot be initialized

## Performance Considerations

### Advantages
- **Low Power**: Hardware timer is more efficient than software loops
- **Precise Timing**: Hardware-level accuracy
- **Non-blocking**: Timer callbacks don't block main execution
- **Scalable**: Multiple channels can run simultaneously

### Trade-offs
- **Memory Overhead**: Additional state tracking per channel
- **Timer Resource Usage**: Each slow PWM channel uses one timer
- **Complexity**: Dual implementation (slow vs hardware PWM)

## Future Enhancements

Potential improvements for future versions:

1. **Variable Timer Resolution**: Use microsecond timers for higher precision
2. **PWM Waveform Generation**: Support for triangle, sawtooth, and sine waves
3. **Frequency Modulation**: Real-time frequency changes
4. **Phase Control**: Synchronized PWM across multiple channels
5. **Advanced Timing**: Support for asymmetric duty cycles and custom waveforms

## Testing

The implementation includes comprehensive testing through:

1. **Unit Tests**: Individual function testing
2. **Integration Tests**: Full PWM cycle testing
3. **Demo Scripts**: Real-world usage examples
4. **Performance Tests**: Timing accuracy verification

## Compatibility

This implementation maintains full backward compatibility:

- **Existing Code**: All existing PWM code continues to work
- **API Consistency**: Same function signatures and behavior
- **Error Handling**: Compatible error codes and messages
- **State Management**: Seamless integration with existing state tracking

## Conclusion

The slow PWM implementation successfully extends the RP2350's PWM capabilities to frequencies below 10Hz using hardware timers. This provides a robust, efficient solution for applications requiring very slow PWM signals while maintaining full compatibility with existing code and APIs. 