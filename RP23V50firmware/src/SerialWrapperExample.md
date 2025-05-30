# SerialWrapper Usage Guide

The `SerialWrapper` class provides additional serial port functionality while keeping the main `Serial` object unchanged. You can explicitly choose where to send output using specific methods.

## Features

- **Explicit Port Control**: Choose exactly where to send output with specific methods
- **Main Serial Unchanged**: `Serial.print()` still goes only to main USB Serial
- **Unified Input Buffer**: Reads from all enabled serial ports are combined
- **Simple Enable/Disable**: Easy control over which ports are active
- **TinyUSB Support**: Works with USB CDC serial ports when available
- **Flexible Output**: Send to one port, all ports, or specific combinations

## Basic Usage - Explicit Control

Include the wrapper header, enable the ports you want, and use specific methods for different destinations:

```cpp
#include "SerialWrapper.h"

// In your setup() function:
void setup() {
    // Enable individual serial ports as needed
    SerialWrap.enableUSBSer1(true);  // Enable USBSer1
    SerialWrap.enableSerial1(true);  // Enable Serial1
    SerialWrap.enableUSBSer2(true);  // Enable USBSer2
    SerialWrap.enableSerial2(true);  // Enable Serial2
    
    // Initialize all enabled ports with the same baud rate
    SerialWrap.begin(115200);
    
    // Normal Serial.print() goes only to main USB Serial
    Serial.println("Hello from main USB Serial only!");
    
    // SerialWrap methods default to main USB (backward compatible)
    SerialWrap.print("Hello main USB!");        // Same as Serial.print()
    SerialWrap.println("Hello main USB!");      // Same as Serial.println()
    
    // Use specific methods to send to other ports
    SerialWrap.printAll("Hello from ALL ports!");
    
    // Send to individual ports
    SerialWrap.printUSBSer1("Hello from USBSer1!");
    SerialWrap.printSerial1("Hello from Serial1!");
    SerialWrap.printUSBSer2("Hello from USBSer2!");
    SerialWrap.printSerial2("Hello from Serial2!");
    
    // Use bitmask to send to specific port combinations
    SerialWrap.print("Hello USB ports!", SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1 | SERIAL_PORT_USBSER2);
    SerialWrap.println("Hello UART ports!", SERIAL_PORT_SERIAL1 | SERIAL_PORT_SERIAL2);
}

void loop() {
    // Check main USB availability (default behavior)
    if (SerialWrap.available()) {
        Serial.println("Main USB has data (using default)");
        char c = SerialWrap.read();
        
        // These are equivalent:
        Serial.print("Serial: ");
        Serial.println(c);
        
        SerialWrap.print("SerialWrap (default): ");  // Goes to main USB only
        SerialWrap.println(c);                       // Goes to main USB only
    }
    
    // Check which specific ports have data
    uint8_t portsWithData = SerialWrap.availablePort();
    
    if (portsWithData) {
        // Get total characters available from those ports
        int totalChars = SerialWrap.available(portsWithData);
        Serial.print("Total chars available: ");
        Serial.println(totalChars);
        
        // Check individual ports
        if (portsWithData & SERIAL_PORT_MAIN) {
            Serial.println("Main USB has data");
        }
        if (portsWithData & SERIAL_PORT_USBSER1) {
            SerialWrap.printUSBSer1("USBSer1 has data");
        }
        if (portsWithData & SERIAL_PORT_SERIAL1) {
            SerialWrap.printSerial1("Serial1 has data");
        }
        
        // Read from unified buffer
        char c = SerialWrap.read();
        
        // Echo to different destinations
        Serial.print("Main USB got: ");
        Serial.println(c);
        
        SerialWrap.printAll("All ports got: ");
        SerialWrap.printlnAll(c);
        
        // Echo to specific port combinations using bitmask
        SerialWrap.print("USB ports got: ", SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1 | SERIAL_PORT_USBSER2);
        SerialWrap.println(c, SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1 | SERIAL_PORT_USBSER2);
    }
}
```

## How It Works

The wrapper provides explicit methods for different serial port destinations while keeping the main `Serial` object unchanged.

### Output Control
- `Serial.print()` - Goes only to main USB Serial (unchanged behavior)
- `SerialWrap.printAll()` - Goes to main USB Serial + all enabled ports
- `SerialWrap.printUSBSer1()` - Goes to USBSer1 and Serial1 (if enabled)
- `SerialWrap.printUSBSer2()` - Goes to USBSer2 and Serial2 (if enabled)

### Input Collection
When you read from `SerialWrap`, it collects available data from:
- The main USB `Serial` port (always)
- `USBSer1` and `Serial1` if enabled
- `USBSer2` and `Serial2` if enabled

All input is combined into a single buffer for seamless reading.

## Installation

1. Add the SerialWrapper files to your project:
   - `SerialWrapper.h`
   - `SerialWrapper.cpp`

2. Include the header in your main file:
```cpp
#include "SerialWrapper.h"
```

3. Enable the individual serial ports you want and initialize:
```cpp
SerialWrap.enableUSBSer1(true);  // Enable USBSer1
SerialWrap.enableSerial1(true);  // Enable Serial1
SerialWrap.enableUSBSer2(true);  // Enable USBSer2
SerialWrap.enableSerial2(true);  // Enable Serial2
SerialWrap.begin(115200);        // Initialize all enabled ports
```

4. Use the appropriate methods for your output needs:
```cpp
Serial.println("Main USB only");           // Normal Serial behavior
SerialWrap.print("Main USB (default)");    // Same as Serial.print() - defaults to main USB
SerialWrap.printAll("To all ports");       // Send to all enabled ports
SerialWrap.printUSBSer1("To USBSer1");     // Send to USBSer1 only
SerialWrap.printSerial1("To Serial1");     // Send to Serial1 only

// Use bitmask for custom port combinations
SerialWrap.print("USB ports only", SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1 | SERIAL_PORT_USBSER2);
SerialWrap.println("UART ports only", SERIAL_PORT_SERIAL1 | SERIAL_PORT_SERIAL2);
```

## Integration with Config System

You can easily integrate this with your existing config system:

```cpp
#include "SerialWrapper.h"
#include "config.h"

extern struct config jumperlessConfig;

void setup() {
    // Enable serial ports based on config
    bool serial1Active = (jumperlessConfig.serial_1.function != 0);
    bool serial2Active = (jumperlessConfig.serial_2.function != 0);
    
    SerialWrap.enableUSBSer1(serial1Active);
    SerialWrap.enableSerial1(serial1Active);
    SerialWrap.enableUSBSer2(serial2Active);
    SerialWrap.enableSerial2(serial2Active);
    
    // Initialize all enabled ports
    SerialWrap.begin(115200);
    
    // Choose where to send initialization message
    Serial.println("Main USB Serial ready!");
    SerialWrap.printAll("All enabled serial ports ready!");
}
```

## Advanced Usage

### Direct Access to Wrapper
If you need to access wrapper-specific functions, you can use `SerialWrap` directly:

```cpp
// Check if specific ports are enabled
if (SerialWrap.isUSBSer1Enabled()) {
    Serial.println("USBSer1 is active");
}

if (SerialWrap.isSerial1Enabled()) {
    Serial.println("Serial1 is active");
}

if (SerialWrap.isUSBSer2Enabled()) {
    Serial.println("USBSer2 is active");
}

if (SerialWrap.isSerial2Enabled()) {
    Serial.println("Serial2 is active");
}

// Enable/disable individual ports at runtime
SerialWrap.enableUSBSer1(false);  // Disable USBSer1
SerialWrap.enableSerial1(true);   // Enable Serial1
SerialWrap.enableUSBSer2(true);   // Enable USBSer2
SerialWrap.enableSerial2(false);  // Disable Serial2

// Clear the input buffer
SerialWrap.clearReadBuffer();

// Check DTR status for USB serial ports
if (SerialWrap.dtrUSBSer1()) {
    Serial.println("USBSer1 DTR is active");
}

// Programmatic port checking with character counts
uint8_t portsWithData = SerialWrap.availablePort();
if (portsWithData) {
    int totalChars = SerialWrap.available(portsWithData);
    Serial.print("Found ");
    Serial.print(totalChars);
    Serial.println(" characters across active ports");
    
    // Check specific port combinations
    int usbOnlyChars = SerialWrap.available(SERIAL_PORT_MAIN);
    int serial1Chars = SerialWrap.available(SERIAL_PORT_USBSER1 | SERIAL_PORT_SERIAL1);
    
    Serial.print("Main USB: ");
    Serial.print(usbOnlyChars);
    Serial.print(", Serial1 ports: ");
    Serial.println(serial1Chars);
}

// Advanced bitmask usage examples
void demonstrateBitmaskMethods() {
    // Send to only USB serial ports (main + USBSer1 + USBSer2)
    uint8_t usbPorts = SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1 | SERIAL_PORT_USBSER2;
    SerialWrap.println("USB ports only!", usbPorts);
    
    // Send to only hardware UART ports
    uint8_t uartPorts = SERIAL_PORT_SERIAL1 | SERIAL_PORT_SERIAL2;
    SerialWrap.println("UART ports only!", uartPorts);
    
    // Send to Serial1 group (USBSer1 + Serial1)
    uint8_t serial1Group = SERIAL_PORT_USBSER1 | SERIAL_PORT_SERIAL1;
    SerialWrap.println("Serial1 group!", serial1Group);
    
    // Send to Serial2 group (USBSer2 + Serial2)
    uint8_t serial2Group = SERIAL_PORT_USBSER2 | SERIAL_PORT_SERIAL2;
    SerialWrap.println("Serial2 group!", serial2Group);
    
    // Send to main USB + one specific port
    SerialWrap.print("Main + USBSer1: ", SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1);
    SerialWrap.println("Hello!");
    
    // Write raw data using bitmask
    uint8_t data[] = {0x48, 0x65, 0x6C, 0x6C, 0x6F}; // "Hello"
    SerialWrap.write(data, sizeof(data), SERIAL_PORT_MAIN | SERIAL_PORT_SERIAL1);
}
```

### Available Methods

**Port Detection:**
- `SerialWrap.availablePort()` - Returns bitmask of ports with data (see constants below)
- `SerialWrap.available()` - Check main USB availability (default, same as Serial.available())
- `SerialWrap.available(portMask)` - Returns total characters available from specific ports (using bitmask)
- `SerialWrap.availableSerial()` - Check if main USB Serial has data
- `SerialWrap.availableUSBSer1()` - Check if USBSer1 has data
- `SerialWrap.availableSerial1()` - Check if Serial1 has data
- `SerialWrap.availableUSBSer2()` - Check if USBSer2 has data
- `SerialWrap.availableSerial2()` - Check if Serial2 has data

**Port Constants (for use with availablePort()):**
- `SERIAL_PORT_MAIN` - Main USB Serial (bit 0)
- `SERIAL_PORT_USBSER1` - USBSer1 (bit 1)
- `SERIAL_PORT_SERIAL1` - Serial1 (bit 2)
- `SERIAL_PORT_USBSER2` - USBSer2 (bit 3)
- `SERIAL_PORT_SERIAL2` - Serial2 (bit 4)

**Print Methods:**
- `SerialWrap.print(value)` - Print to main USB (default, same as Serial.print())
- `SerialWrap.println(value)` - Print line to main USB (default, same as Serial.println())
- `SerialWrap.print(value, portMask)` - Print to ports specified by bitmask
- `SerialWrap.println(value, portMask)` - Print line to ports specified by bitmask
- `SerialWrap.printAll(value)` - Print to all enabled ports
- `SerialWrap.printlnAll(value)` - Print line to all enabled ports  
- `SerialWrap.printUSBSer1(value)` - Print to USBSer1 only
- `SerialWrap.printlnUSBSer1(value)` - Print line to USBSer1 only
- `SerialWrap.printSerial1(value)` - Print to Serial1 only
- `SerialWrap.printlnSerial1(value)` - Print line to Serial1 only
- `SerialWrap.printUSBSer2(value)` - Print to USBSer2 only
- `SerialWrap.printlnUSBSer2(value)` - Print line to USBSer2 only
- `SerialWrap.printSerial2(value)` - Print to Serial2 only
- `SerialWrap.printlnSerial2(value)` - Print line to Serial2 only

**Write Methods:**
- `SerialWrap.write(data)` - Write raw data to main USB (default, same as Serial.write())
- `SerialWrap.write(data, portMask)` - Write raw data to ports specified by bitmask
- `SerialWrap.writeAll(data)` - Write raw data to all enabled ports
- `SerialWrap.writeUSBSer1(data)` - Write raw data to USBSer1 only
- `SerialWrap.writeSerial1(data)` - Write raw data to Serial1 only
- `SerialWrap.writeUSBSer2(data)` - Write raw data to USBSer2 only
- `SerialWrap.writeSerial2(data)` - Write raw data to Serial2 only

**Other Methods:**
- `SerialWrap.available()` - Check main USB availability (default, same as Serial.available())
- `SerialWrap.available(portMask)` - Check availability for ports specified by bitmask
- `SerialWrap.flush()` - Flush main USB (default, same as Serial.flush())
- `SerialWrap.flush(portMask)` - Flush ports specified by bitmask

**Flush Methods:**
- `SerialWrap.flushAll()` - Flush all enabled ports
- `SerialWrap.flushUSBSer1()` - Flush USBSer1 only
- `SerialWrap.flushSerial1()` - Flush Serial1 only
- `SerialWrap.flushUSBSer2()` - Flush USBSer2 only
- `SerialWrap.flushSerial2()` - Flush Serial2 only

## Example: Complete Integration

Your existing Arduino code continues to work without modification:

```cpp
#include "SerialWrapper.h"
#include "config.h"

extern struct config jumperlessConfig;

void setup() {
    // Enable ports based on config
    SerialWrap.enableSerial1(jumperlessConfig.serial_1.function != 0);
    SerialWrap.enableSerial2(jumperlessConfig.serial_2.function != 0);
    
    // Initialize all enabled ports
    SerialWrap.begin(115200);
    
    while (!Serial) {
        ; // Wait for main serial port to connect
    }
    
    // Choose where to send messages
    Serial.println("Main USB Serial ready!");
    SerialWrap.printAll("All enabled serial ports ready!");
}

void loop() {
    // Read from all enabled ports
    if (SerialWrap.available()) {
        String input = SerialWrap.readStringUntil('\n');
        
        // Echo to main USB Serial
        Serial.print("Main USB Echo: ");
        Serial.println(input);
        
        // Echo to all ports
        SerialWrap.printAll("All ports Echo: ");
        SerialWrap.printlnAll(input);
        
        // Send status to specific ports
        SerialWrap.printUSBSer1("USBSer1 processed: ");
        SerialWrap.printlnUSBSer1(input);
    }
    
    // Send periodic status to different destinations
    static unsigned long lastMillis = 0;
    if (millis() - lastMillis > 1000) {
        lastMillis = millis();
        
        Serial.print("Main USB Millis: ");
        Serial.println(millis());
        
        SerialWrap.printUSBSer2("USBSer2 Millis: ");
        SerialWrap.printlnUSBSer2(millis());
    }
}
```

## Notes

- **Main Serial unchanged**: `Serial.print()` behavior is exactly the same as before
- **Explicit control**: Choose exactly where each message goes using specific methods
- **Input from all ports**: Reading from `SerialWrap` collects from all enabled ports
- **Automatic initialization**: `SerialWrap.begin()` initializes all enabled ports
- **Buffer size**: 512 bytes for unified input collection
- **No performance overhead**: Only enabled ports are accessed
- **Flexible usage**: Mix and match different output destinations as needed 