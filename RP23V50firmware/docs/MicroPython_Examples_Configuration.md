# MicroPython Examples - Compile-Time Configuration

This document explains how to control which MicroPython examples are included in your build at compile time.

## Overview

The Jumperless firmware includes a set of MicroPython examples that are automatically loaded into the filesystem. You can control which examples are included by modifying compile-time defines in `src/micropythonExamples.h`.

## Configuration Options

### Individual Example Control

Edit `src/micropythonExamples.h` and comment out any examples you don't want to include:

```cpp
// Define which examples to include at compile time
// Comment out any example you don't want to include
#define INCLUDE_DAC_BASICS
#define INCLUDE_ADC_BASICS  
#define INCLUDE_GPIO_BASICS
#define INCLUDE_NODE_CONNECTIONS
#define INCLUDE_README
#define INCLUDE_TEST_RUNNER
#define INCLUDE_LED_BRIGHTNESS_CONTROL
#define INCLUDE_VOLTAGE_MONITOR
#define INCLUDE_STYLOPHONE
```

### Bulk Disable Options

For convenience, you can disable entire categories of examples:

```cpp
// Uncomment any of these to disable entire categories
// #define DISABLE_ALL_EXAMPLES
// #define DISABLE_HARDWARE_EXAMPLES    // Disables DAC, ADC, GPIO, Node examples
// #define DISABLE_DEMO_EXAMPLES        // Disables LED, Voltage, Stylophone demos
// #define DISABLE_UTILITY_EXAMPLES     // Disables README and test runner
```

## Available Examples

### Hardware Examples (`DISABLE_HARDWARE_EXAMPLES`)
- **DAC Basics** (`INCLUDE_DAC_BASICS`) - Digital-to-Analog Converter operations
- **ADC Basics** (`INCLUDE_ADC_BASICS`) - Analog-to-Digital Converter operations  
- **GPIO Basics** (`INCLUDE_GPIO_BASICS`) - General Purpose Input/Output operations
- **Node Connections** (`INCLUDE_NODE_CONNECTIONS`) - Breadboard node routing

### Demo Examples (`DISABLE_DEMO_EXAMPLES`)
- **LED Brightness Control** (`INCLUDE_LED_BRIGHTNESS_CONTROL`) - Touch-controlled LED brightness
- **Voltage Monitor** (`INCLUDE_VOLTAGE_MONITOR`) - Real-time voltage monitoring
- **Stylophone** (`INCLUDE_STYLOPHONE`) - Musical instrument using probe

### Utility Examples (`DISABLE_UTILITY_EXAMPLES`)
- **README** (`INCLUDE_README`) - Documentation and usage instructions
- **Test Runner** (`INCLUDE_TEST_RUNNER`) - Automated test suite for all examples

## Configuration Examples

### Minimal Configuration (Hardware Only)
```cpp
#define INCLUDE_DAC_BASICS
#define INCLUDE_ADC_BASICS  
#define INCLUDE_GPIO_BASICS
#define INCLUDE_NODE_CONNECTIONS
// #define INCLUDE_README
// #define INCLUDE_TEST_RUNNER
// #define INCLUDE_LED_BRIGHTNESS_CONTROL
// #define INCLUDE_VOLTAGE_MONITOR
// #define INCLUDE_STYLOPHONE
```

### Demo-Only Configuration
```cpp
// #define INCLUDE_DAC_BASICS
// #define INCLUDE_ADC_BASICS  
// #define INCLUDE_GPIO_BASICS
// #define INCLUDE_NODE_CONNECTIONS
#define INCLUDE_README
#define INCLUDE_TEST_RUNNER
#define INCLUDE_LED_BRIGHTNESS_CONTROL
#define INCLUDE_VOLTAGE_MONITOR
#define INCLUDE_STYLOPHONE
```

### Testing Configuration
```cpp
// #define INCLUDE_DAC_BASICS
// #define INCLUDE_ADC_BASICS  
// #define INCLUDE_GPIO_BASICS
// #define INCLUDE_NODE_CONNECTIONS
#define INCLUDE_README
#define INCLUDE_TEST_RUNNER
// #define INCLUDE_LED_BRIGHTNESS_CONTROL
// #define INCLUDE_VOLTAGE_MONITOR
// #define INCLUDE_STYLOPHONE
```

### Disable All Examples
```cpp
#define DISABLE_ALL_EXAMPLES
```

## Build Process

1. Edit `src/micropythonExamples.h` with your desired configuration
2. Build the firmware as usual
3. The selected examples will be automatically created in `/python_scripts/examples/` when the device boots

## Benefits

- **Reduced Flash Usage**: Only include examples you actually need
- **Faster Boot Time**: Fewer files to create during initialization
- **Cleaner Filesystem**: No unused example files cluttering the filesystem
- **Memory Efficiency**: Less RAM used during example loading

## Notes

- Changes require a firmware rebuild - they cannot be changed at runtime
- If no examples are enabled, the initialization function will skip example creation entirely
- The filesystem will still create the `/python_scripts/examples/` directory structure
- Individual example files are only created if they don't already exist on the filesystem

## Troubleshooting

If you encounter build errors after changing the configuration:

1. **Clean Build**: Delete your build directory and rebuild from scratch
2. **Check Syntax**: Ensure all `#define` statements are properly formatted
3. **Verify Dependencies**: Some examples may reference constants from other examples
4. **Reset Configuration**: Temporarily enable all examples to verify the system works

## File Size Reference

Approximate file sizes for each example:
- DAC Basics: ~2.5KB
- ADC Basics: ~2.0KB
- GPIO Basics: ~3.0KB
- Node Connections: ~3.5KB
- README: ~4.0KB
- Test Runner: ~3.0KB
- LED Brightness Control: ~2.5KB
- Voltage Monitor: ~2.5KB
- Stylophone: ~3.0KB

Total: ~26KB for all examples

## Future Enhancements

Planned improvements to the configuration system:
- Runtime configuration via config files
- Web interface for example management
- Custom example templates
- Example dependency management 