# Routable Buffer GPIO Functionality

This document describes the new GPIO functionality added to the `routableBufferPower()` function, which allows connecting the routable buffer to an unused GPIO instead of a DAC.

## Overview

The `routableBufferPower()` function now includes a new optional parameter `useGPIO` that allows you to connect the routable buffer to an available GPIO pin instead of using a DAC. This is useful when you need to conserve DAC resources or when you want to use a GPIO for power control.

## Function Signature

```cpp
void routableBufferPower(int offOn, int flash = 0, int force = 0, int useGPIO = 0);
```

### Parameters

- `offOn`: Power state (1 = on, 0 = off)
- `flash`: Flash mode (1 = flash, 0 = local)
- `force`: Force refresh (1 = force, 0 = normal)
- `useGPIO`: Use GPIO instead of DAC (1 = use GPIO, 0 = use DAC) **[NEW]**

## How It Works

1. **GPIO Detection**: When `useGPIO = 1`, the function calls `findUnusedGPIO()` to find an available GPIO from RP_GPIO_1 through RP_GPIO_8.

2. **Connection Logic**: 
   - If an unused GPIO is found, it connects `ROUTABLE_BUFFER_IN` to that GPIO
   - If no unused GPIO is available, it falls back to the original DAC behavior

3. **Fallback Behavior**: The function automatically falls back to DAC behavior if no GPIOs are available, ensuring backward compatibility.

## GPIO Availability

The function checks for GPIO availability by scanning all nets to see if any GPIO nodes (RP_GPIO_1 through RP_GPIO_8) are currently connected. A GPIO is considered available if it's not connected to any net.

## Usage Examples

### C++ Usage

```cpp
#include "Probing.h"

// Power on using DAC (default behavior)
routableBufferPower(1, 0, 0, 0);

// Power on using GPIO (new functionality)
routableBufferPower(1, 0, 0, 1);

// Power off using GPIO
routableBufferPower(0, 0, 0, 1);
```

### Python Usage (via jumperless module)

```python
import jumperless

# Power on using DAC (default behavior)
jumperless.routable_buffer_power(1, 0, 0, 0)

# Power on using GPIO (new functionality)
jumperless.routable_buffer_power(1, 0, 0, 1)

# Power off using GPIO
jumperless.routable_buffer_power(0, 0, 0, 1)
```

## Implementation Details

### findUnusedGPIO() Function

```cpp
int findUnusedGPIO() {
    // Check each GPIO to see if it's connected to anything
    for (int i = 0; i < 8; i++) {
        int gpioNode = RP_GPIO_1 + i; // RP_GPIO_1 through RP_GPIO_8
        
        // Check if this GPIO is connected to anything in any net
        bool isConnected = false;
        for (int netIndex = 0; netIndex < numberOfNets; netIndex++) {
            for (int nodeIndex = 0; nodeIndex < MAX_NODES; nodeIndex++) {
                if (net[netIndex].nodes[nodeIndex] <= 0) {
                    break; // End of nodes array
                }
                if (net[netIndex].nodes[nodeIndex] == gpioNode) {
                    isConnected = true;
                    break;
                }
            }
            if (isConnected) break;
        }
        
        // If not connected, this GPIO is available
        if (!isConnected) {
            return gpioNode;
        }
    }
    
    // No unused GPIOs found
    return -1;
}
```

### Modified routableBufferPower() Logic

The function now includes this logic at the beginning:

```cpp
// If useGPIO is specified, try to use an unused GPIO instead of DAC
if (useGPIO == 1) {
    int unusedGPIO = findUnusedGPIO();
    if (unusedGPIO != -1) {
        // Use the found GPIO instead of DAC
        if (offOn == 1) {
            // Power on - connect ROUTABLE_BUFFER_IN to the unused GPIO
            if (checkIfBridgeExistsLocal(ROUTABLE_BUFFER_IN, unusedGPIO) == 0) {
                addBridgeToNodeFile(ROUTABLE_BUFFER_IN, unusedGPIO, netSlot, flashOrLocal, 0);
                needToRefresh = true;
            }
            bufferPowerConnected = true;
        } else {
            // Power off - disconnect ROUTABLE_BUFFER_IN from the GPIO
            if (checkIfBridgeExistsLocal(ROUTABLE_BUFFER_IN, unusedGPIO) == 1) {
                removeBridgeFromNodeFile(ROUTABLE_BUFFER_IN, unusedGPIO, netSlot, flashOrLocal);
                needToRefresh = true;
            }
            bufferPowerConnected = false;
        }
        
        if (needToRefresh == true || force == 1) {
            if (flash == 1) {
                refreshConnections(0, 0, 0);
            } else {
                refreshLocalConnections(0, 0, 0);
            }
        }
        
        return;
    } else {
        // No unused GPIO found, fall back to DAC behavior
    }
}
```

## Benefits

1. **Resource Conservation**: Allows you to save DAC resources for other applications
2. **Flexibility**: Provides an alternative power source for the routable buffer
3. **Backward Compatibility**: Existing code continues to work without modification
4. **Automatic Fallback**: Gracefully falls back to DAC behavior when no GPIOs are available

## Testing

See the example files:
- `routable_buffer_gpio_test.py` - Python test script
- `routable_buffer_gpio_example.cpp` - C++ example

These examples demonstrate how to use the new functionality and check GPIO availability.

## Notes

- The function prioritizes GPIOs in order (RP_GPIO_1, RP_GPIO_2, etc.)
- If all GPIOs are in use, the function automatically falls back to DAC behavior
- The GPIO connection is managed through the same bridge system used for DAC connections
- This feature is particularly useful in scenarios where you need to conserve DAC resources or when you want to use GPIO-based power control 