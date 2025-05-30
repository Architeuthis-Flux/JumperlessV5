# SerialWrapper with Dynamic Serial Redirection

The SerialWrapper class now supports dynamic redirection of the default `Serial` object based on a global volatile bitmask called `serialTarget`. This allows you to control which serial ports receive output without changing your existing code.

## Key Features

- **Backward Compatible**: When `serialTarget` is 0, behaves exactly like the default `Serial`
- **Dynamic Redirection**: Change output destinations at runtime using a simple bitmask
- **Multiple Port Support**: Supports main Serial, USBSer1, Serial1, USBSer2, and Serial2
- **Thread Safe**: Uses volatile global variable for interrupt-safe operation

## Global Variable

```cpp
volatile uint8_t serialTarget = 0;
```

- **0**: Default behavior (main Serial only)
- **Non-zero**: Bitmask specifying which ports to use

## Port Bitmask Constants

```cpp
#define SERIAL_PORT_MAIN    0x01  // Bit 0: Serial (main USB)
#define SERIAL_PORT_USBSER1 0x02  // Bit 1: USBSer1
#define SERIAL_PORT_SERIAL1 0x04  // Bit 2: Serial1
#define SERIAL_PORT_USBSER2 0x08  // Bit 3: USBSer2
#define SERIAL_PORT_SERIAL2 0x10  // Bit 4: Serial2
```

## Convenience Macros

```cpp
SERIAL_REDIRECT_TO(mask)    // Set specific ports
SERIAL_REDIRECT_OFF()       // Disable redirection (back to default)
SERIAL_REDIRECT_ALL()       // Redirect to all enabled ports
```

## Basic Usage

### 1. Replace Serial with SerialWrap

```cpp
#include "SerialWrapper.h"
#define Serial SerialWrap  // Optional: replace Serial globally
```

### 2. Enable Additional Ports

```cpp
void setup() {
    // Enable the ports you want to use
    SerialWrap.enableUSBSer1(true);
    SerialWrap.enableSerial1(true);
    SerialWrap.enableUSBSer2(true);
    SerialWrap.enableSerial2(true);
    
    SerialWrap.begin(115200);
}
```

### 3. Control Redirection

```cpp
// Default behavior - main Serial only
Serial.println("Goes to main Serial");

// Redirect to all enabled ports
SERIAL_REDIRECT_ALL();
Serial.println("Goes to ALL enabled ports");

// Redirect to specific ports
SERIAL_REDIRECT_TO(SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1);
Serial.println("Goes to main Serial and USBSer1");

// Back to default
SERIAL_REDIRECT_OFF();
Serial.println("Back to main Serial only");
```

## Advanced Usage

### Dynamic Redirection Based on Conditions

```cpp
void loop() {
    if (errorCondition) {
        // Send errors to all ports for visibility
        SERIAL_REDIRECT_ALL();
        Serial.println("ERROR: Something went wrong!");
        SERIAL_REDIRECT_OFF();
    }
    
    if (debugMode) {
        // Send debug info to specific port
        SERIAL_REDIRECT_TO(SERIAL_PORT_USBSER2);
        Serial.println("Debug info");
        SERIAL_REDIRECT_OFF();
    }
}
```

### Runtime Control via Commands

```cpp
if (Serial.available()) {
    String cmd = Serial.readString();
    cmd.trim();
    
    if (cmd == "debug_on") {
        SERIAL_REDIRECT_TO(SERIAL_PORT_MAIN | SERIAL_PORT_USBSER2);
    } else if (cmd == "debug_off") {
        SERIAL_REDIRECT_OFF();
    }
}
```

### Check Current Status

```cpp
// Using static methods
uint8_t currentTarget = SerialWrapper::getSerialTarget();
bool isRedirecting = SerialWrapper::isSerialRedirectionEnabled();

// Using instance methods
uint8_t currentTarget = SerialWrap.getSerialTarget();
bool isRedirecting = SerialWrap.isSerialRedirectionEnabled();
```

## Method Reference

### Static Control Methods

```cpp
SerialWrapper::setSerialTarget(uint8_t mask);     // Set target ports
SerialWrapper::getSerialTarget();                 // Get current target
SerialWrapper::enableSerialRedirection(uint8_t mask);  // Enable with mask
SerialWrapper::disableSerialRedirection();        // Disable redirection
SerialWrapper::isSerialRedirectionEnabled();      // Check if enabled
```

### Port Enable/Disable Methods

```cpp
SerialWrap.enableUSBSer1(bool enable);
SerialWrap.enableSerial1(bool enable);
SerialWrap.enableUSBSer2(bool enable);
SerialWrap.enableSerial2(bool enable);
```

## Examples

### Example 1: Simple Redirection

```cpp
#include "SerialWrapper.h"
#define Serial SerialWrap

void setup() {
    SerialWrap.enableUSBSer1(true);
    SerialWrap.begin(115200);
    
    Serial.println("Default output");
    
    SERIAL_REDIRECT_TO(SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1);
    Serial.println("Dual output");
    
    SERIAL_REDIRECT_OFF();
    Serial.println("Back to default");
}
```

### Example 2: Conditional Redirection

```cpp
void logMessage(const char* level, const char* message) {
    if (strcmp(level, "ERROR") == 0) {
        SERIAL_REDIRECT_ALL();  // Errors go everywhere
    } else if (strcmp(level, "DEBUG") == 0) {
        SERIAL_REDIRECT_TO(SERIAL_PORT_USBSER2);  // Debug to specific port
    } else {
        SERIAL_REDIRECT_OFF();  // Normal messages to main Serial
    }
    
    Serial.print("[");
    Serial.print(level);
    Serial.print("] ");
    Serial.println(message);
    
    SERIAL_REDIRECT_OFF();  // Reset to default
}
```

## Notes

- The `serialTarget` variable is volatile and can be safely modified from interrupts
- When `serialTarget` is 0, the wrapper behaves exactly like the default Serial
- Only enabled ports will actually receive data, even if included in the bitmask
- Reading operations (available, read, peek) respect the serialTarget setting
- The unified buffer is used only when targeting the main Serial port exclusively
- Methods with bitmask parameters require explicit bitmask values (no default parameters to avoid ambiguity)

## Compilation Notes

To avoid function call ambiguity with Arduino's Print class, some design decisions were made:

### ✅ **Available Methods:**
```cpp
// These work without ambiguity
Serial.available()                    // Uses serialTarget (virtual override)
SerialWrap.available(SERIAL_PORT_MAIN) // Explicit bitmask
SerialWrap.availableAny()             // Check all enabled ports

Serial.flush()                        // Uses serialTarget (virtual override)  
SerialWrap.flush(SERIAL_PORT_MAIN)    // Explicit bitmask
SerialWrap.flushMain()                // Flush main Serial only

Serial.write()                        // Uses serialTarget (virtual override)
SerialWrap.write(data, SERIAL_PORT_MAIN) // Explicit bitmask
```

### ✅ **Print Methods:**
```cpp
// Standard print methods work with serialTarget
Serial.print("Hello");                // Uses serialTarget automatically
Serial.println(123);                  // Uses serialTarget automatically

// Individual port methods (no ambiguity)
SerialWrap.printUSBSer1("Debug");     // Print to USBSer1 only
SerialWrap.printlnSerial1(value);     // Print to Serial1 only
SerialWrap.printAll("Broadcast");     // Print to all enabled ports
```

### ⚠️ **Removed to Avoid Ambiguity:**
- Template `print(T val, uint8_t portMask)` methods were removed
- Use individual port methods or `printAll()` instead
- The standard `Serial.print()` respects `serialTarget` automatically 