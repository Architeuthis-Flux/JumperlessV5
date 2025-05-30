// SPDX-License-Identifier: MIT
#ifndef SERIALWRAPPER_H
#define SERIALWRAPPER_H

#include <Arduino.h>

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
extern Adafruit_USBD_CDC USBSer1;
extern Adafruit_USBD_CDC USBSer2;
#endif



// Port availability bitmask constants
#define SERIAL_PORT_MAIN    0x01  // Bit 0: Serial (main USB)
#define SERIAL_PORT_USBSER1 0x02  // Bit 1: USBSer1
#define SERIAL_PORT_SERIAL1 0x04  // Bit 2: Serial1
#define SERIAL_PORT_USBSER2 0x08  // Bit 3: USBSer2
#define SERIAL_PORT_SERIAL2 0x10  // Bit 4: Serial2

// Global volatile bitmask to control Serial redirection
// When 0, uses default Serial behavior
// When non-zero, redirects Serial calls to specified ports
extern volatile uint8_t serialTarget;

// Convenience macros for Serial redirection
#define SERIAL_REDIRECT_TO(mask) serialTarget = (mask)
#define SERIAL_REDIRECT_OFF() serialTarget = 0
#define SERIAL_REDIRECT_ALL() serialTarget = (SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1 | SERIAL_PORT_SERIAL1 | SERIAL_PORT_USBSER2 | SERIAL_PORT_SERIAL2)

class SerialWrapper : public Stream {
private:
    // Configuration flags for individual ports
    bool usbSer1Enabled = false;
    bool serial1Enabled = false;
    bool usbSer2Enabled = false;
    bool serial2Enabled = false;
    
    // Buffer for read operations
    static const size_t READ_BUFFER_SIZE = 512;
    uint8_t readBuffer[READ_BUFFER_SIZE];
    size_t readBufferHead = 0;
    size_t readBufferTail = 0;
    
        // Helper to write to all enabled serial ports
    size_t writeToAll(uint8_t c) {
        size_t bytesWritten = Serial.write(c);
        
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer1.write(c);
           //  #endif
        }
        
        if (serial1Enabled) {
            Serial1.write(c);
        }
        
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer2.write(c);
           //  #endif
        }
        
        if (serial2Enabled) {
            Serial2.write(c);
        }
        
        return bytesWritten;
    }
    
    size_t writeToAll(const uint8_t* buffer, size_t size) {
        size_t bytesWritten = Serial.write(buffer, size);
        
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer1.write(buffer, size);
           //  #endif
        }
        
        if (serial1Enabled) {
            Serial1.write(buffer, size);
        }
        
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer2.write(buffer, size);
           //  #endif
        }
        
        if (serial2Enabled) {
            Serial2.write(buffer, size);
        }
        
        return bytesWritten;
    }
    
     
     
     // Individual port availability checks
     bool availableSerial() {
         return Serial.available() > 0;
     }
     
     bool availableUSBSer1() {
         if (usbSer1Enabled) {
            //  #ifdef USE_TINYUSB
             return USBSer1.available() > 0;
            //  #endif
         }
         return false;
     }
     
     bool availableSerial1() {
         if (serial1Enabled) {
             return Serial1.available() > 0;
         }
         return false;
     }
     
     bool availableUSBSer2() {
         if (usbSer2Enabled) {
            //  #ifdef USE_TINYUSB
             return USBSer2.available() > 0;
            //  #endif
         }
         return false;
     }
     
     bool availableSerial2() {
         if (serial2Enabled) {
             return Serial2.available() > 0;
         }
         return false;
     }
     
        
    
    // Helper to write to USBSer1 only
    size_t writeToUSBSer1(uint8_t c) {
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            return USBSer1.write(c);
           //  #endif
        }
        return 0;
    }
    
    size_t writeToUSBSer1(const uint8_t* buffer, size_t size) {
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            return USBSer1.write(buffer, size);
           //  #endif
        }
        return 0;
    }
    
    // Helper to write to Serial1 only
    size_t writeToSerial1(uint8_t c) {
        if (serial1Enabled) {
            return Serial1.write(c);
        }
        return 0;
    }
    
    size_t writeToSerial1(const uint8_t* buffer, size_t size) {
        if (serial1Enabled) {
            return Serial1.write(buffer, size);
        }
        return 0;
    }
    
    // Helper to write to USBSer2 only
    size_t writeToUSBSer2(uint8_t c) {
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            return USBSer2.write(c);
           //  #endif
        }
        return 0;
    }
    
    size_t writeToUSBSer2(const uint8_t* buffer, size_t size) {
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            return USBSer2.write(buffer, size);
           //  #endif
        }
        return 0;
    }
    
    // Helper to write to Serial2 only
    size_t writeToSerial2(uint8_t c) {
        if (serial2Enabled) {
            return Serial2.write(c);
        }
        return 0;
    }
    
    size_t writeToSerial2(const uint8_t* buffer, size_t size) {
        if (serial2Enabled) {
            return Serial2.write(buffer, size);
        }
        return 0;
    }
    
    // Helper to collect available data from all sources
    void collectAvailableData() {
        // Read from main Serial
        while (Serial.available() && ((readBufferHead + 1) % READ_BUFFER_SIZE) != readBufferTail) {
            readBuffer[readBufferHead] = Serial.read();
            readBufferHead = (readBufferHead + 1) % READ_BUFFER_SIZE;
        }
        
        // Read from USBSer1 if enabled
        if (usbSer1Enabled) {
          //  #ifdef USE_TINYUSB
            while (USBSer1.available() && ((readBufferHead + 1) % READ_BUFFER_SIZE) != readBufferTail) {
                readBuffer[readBufferHead] = USBSer1.read();
                readBufferHead = (readBufferHead + 1) % READ_BUFFER_SIZE;
            }
          //  #endif
        }
        
        // Read from Serial1 if enabled
        if (serial1Enabled) {
            while (Serial1.available() && ((readBufferHead + 1) % READ_BUFFER_SIZE) != readBufferTail) {
                readBuffer[readBufferHead] = Serial1.read();
                readBufferHead = (readBufferHead + 1) % READ_BUFFER_SIZE;
            }
        }
        
        // Read from USBSer2 if enabled
        if (usbSer2Enabled) {
          //  #ifdef USE_TINYUSB
            while (USBSer2.available() && ((readBufferHead + 1) % READ_BUFFER_SIZE) != readBufferTail) {
                readBuffer[readBufferHead] = USBSer2.read();
                readBufferHead = (readBufferHead + 1) % READ_BUFFER_SIZE;
            }
          //  #endif
        }
        
        // Read from Serial2 if enabled
        if (serial2Enabled) {
            while (Serial2.available() && ((readBufferHead + 1) % READ_BUFFER_SIZE) != readBufferTail) {
                readBuffer[readBufferHead] = Serial2.read();
                readBufferHead = (readBufferHead + 1) % READ_BUFFER_SIZE;
            }
        }
    }

public:
    // Constructor
    SerialWrapper() {}
    
    // Enable/disable individual serial ports
    void enableUSBSer1(bool enable = true) {
        usbSer1Enabled = enable;
    }
    
    void enableSerial1(bool enable = true) {
        serial1Enabled = enable;
    }
    
    void enableUSBSer2(bool enable = true) {
        usbSer2Enabled = enable;
    }
    
    void enableSerial2(bool enable = true) {
        serial2Enabled = enable;
    }
    
    // Begin serial communication
    void begin(unsigned long baud) {
        Serial.begin(baud);
        
        // Initialize enabled serial ports with the same baud rate
        if (usbSer1Enabled) {
           // #ifdef USE_TINYUSB
            USBSer1.begin(baud);
          //  #endif
        }
        
        if (serial1Enabled) {
            Serial1.begin(baud);
        }
        
        if (usbSer2Enabled) {
          //  #ifdef USE_TINYUSB
            USBSer2.begin(baud);
          //  #endif
        }
        
        if (serial2Enabled) {
            Serial2.begin(baud);
        }
    }
    
    void begin(unsigned long baud, uint16_t serialConfig) {
        Serial.begin(baud, serialConfig);
        
        // Initialize enabled serial ports with the same settings
        if (usbSer1Enabled) {
         //   #ifdef USE_TINYUSB
            USBSer1.begin(baud, serialConfig);
          //  #endif
        }
        
        if (serial1Enabled) {
            Serial1.begin(baud, serialConfig);
        }
        
        if (usbSer2Enabled) {
          //  #ifdef USE_TINYUSB
            USBSer2.begin(baud, serialConfig);
          //  #endif
        }
        
        if (serial2Enabled) {
            Serial2.begin(baud, serialConfig);
        }
    }
    
    // End serial communication
    void end() {
        Serial.end();
        
        // End enabled serial ports
        if (usbSer1Enabled) {
          //  #ifdef USE_TINYUSB
            USBSer1.end();
          //  #endif
        }
        
        if (serial1Enabled) {
            Serial1.end();
        }
        
        if (usbSer2Enabled) {
          //  #ifdef USE_TINYUSB
            USBSer2.end();
          //  #endif
        }
        
        if (serial2Enabled) {
            Serial2.end();
        }
    }

        // Returns which port has data available (bitmask)
     // Bit 0: Serial (main USB)
     // Bit 1: USBSer1 
     // Bit 2: Serial1
     // Bit 3: USBSer2
     // Bit 4: Serial2
     uint8_t availablePort() {
         uint8_t portMask = 0;
         
         if (Serial.available()) {
             portMask |= 0x01;  // Bit 0
         }
         
         if (usbSer1Enabled) {
            //  #ifdef USE_TINYUSB
             if (USBSer1.available()) {
                 portMask |= 0x02;  // Bit 1
             }
            //  #endif
         }
         
         if (serial1Enabled) {
             if (Serial1.available()) {
                 portMask |= 0x04;  // Bit 2
             }
         }
         
         if (usbSer2Enabled) {
            //  #ifdef USE_TINYUSB
             if (USBSer2.available()) {
                 portMask |= 0x08;  // Bit 3
             }
            //  #endif
         }
         
         if (serial2Enabled) {
             if (Serial2.available()) {
                 portMask |= 0x10;  // Bit 4
             }
         }
         
         return portMask;
     }
    
    // Stream interface implementation
    virtual int available(void) override {
        // Use serialTarget if set, otherwise default to main Serial
        uint8_t targetMask = serialTarget ? serialTarget : SERIAL_PORT_MAIN;
        return available(targetMask);
    }
    
    // Available with specific bitmask
    int available(uint8_t portMask) {
        if (portMask == SERIAL_PORT_MAIN) {
            // For main USB only, use the unified buffer
            collectAvailableData();
            return (readBufferHead >= readBufferTail) ? 
                   (readBufferHead - readBufferTail) : 
                   (READ_BUFFER_SIZE - readBufferTail + readBufferHead);
        } else {
            // For other port combinations, return direct count
            int totalAvailable = 0;
            
            if (portMask & SERIAL_PORT_MAIN) {
                totalAvailable += Serial.available();
            }
            
            if (usbSer1Enabled && (portMask & SERIAL_PORT_USBSER1)) {
               //  #ifdef USE_TINYUSB
                totalAvailable += USBSer1.available();
               //  #endif
            }
            
            if (serial1Enabled && (portMask & SERIAL_PORT_SERIAL1)) {
                totalAvailable += Serial1.available();
            }
            
            if (usbSer2Enabled && (portMask & SERIAL_PORT_USBSER2)) {
               //  #ifdef USE_TINYUSB
                totalAvailable += USBSer2.available();
               //  #endif
            }
            
            if (serial2Enabled && (portMask & SERIAL_PORT_SERIAL2)) {
                totalAvailable += Serial2.available();
            }
            
            return totalAvailable;
        }
    }
    
    virtual int read(void) override {
        // If serialTarget is 0 or only main Serial, use unified buffer
        uint8_t targetMask = serialTarget ? serialTarget : SERIAL_PORT_MAIN;
        if (targetMask == SERIAL_PORT_MAIN) {
            collectAvailableData();
            if (readBufferHead == readBufferTail) {
                return -1; // No data available
            }
            
            uint8_t data = readBuffer[readBufferTail];
            readBufferTail = (readBufferTail + 1) % READ_BUFFER_SIZE;
            return data;
        } else {
            // For multiple ports, read from first available port in priority order
            if ((targetMask & SERIAL_PORT_MAIN) && Serial.available()) {
                return Serial.read();
            }
            if (usbSer1Enabled && (targetMask & SERIAL_PORT_USBSER1) && USBSer1.available()) {
                return USBSer1.read();
            }
            if (serial1Enabled && (targetMask & SERIAL_PORT_SERIAL1) && Serial1.available()) {
                return Serial1.read();
            }
            if (usbSer2Enabled && (targetMask & SERIAL_PORT_USBSER2) && USBSer2.available()) {
                return USBSer2.read();
            }
            if (serial2Enabled && (targetMask & SERIAL_PORT_SERIAL2) && Serial2.available()) {
                return Serial2.read();
            }
            return -1; // No data available
        }
    }
    
    virtual int peek(void) override {
        // If serialTarget is 0 or only main Serial, use unified buffer
        uint8_t targetMask = serialTarget ? serialTarget : SERIAL_PORT_MAIN;
        if (targetMask == SERIAL_PORT_MAIN) {
            collectAvailableData();
            if (readBufferHead == readBufferTail) {
                return -1; // No data available
            }
            return readBuffer[readBufferTail];
        } else {
            // For multiple ports, peek from first available port in priority order
            if ((targetMask & SERIAL_PORT_MAIN) && Serial.available()) {
                return Serial.peek();
            }
            if (usbSer1Enabled && (targetMask & SERIAL_PORT_USBSER1) && USBSer1.available()) {
                return USBSer1.peek();
            }
            if (serial1Enabled && (targetMask & SERIAL_PORT_SERIAL1) && Serial1.available()) {
                return Serial1.peek();
            }
            if (usbSer2Enabled && (targetMask & SERIAL_PORT_USBSER2) && USBSer2.available()) {
                return USBSer2.peek();
            }
            if (serial2Enabled && (targetMask & SERIAL_PORT_SERIAL2) && Serial2.available()) {
                return Serial2.peek();
            }
            return -1; // No data available
        }
    }
    
    virtual size_t write(uint8_t c) override {
        // Use serialTarget if set, otherwise default to main Serial
        uint8_t targetMask = serialTarget ? serialTarget : SERIAL_PORT_MAIN;
        return write(c, targetMask);
    }
    
    virtual size_t write(const uint8_t* buffer, size_t size) override {
        // Use serialTarget if set, otherwise default to main Serial
        uint8_t targetMask = serialTarget ? serialTarget : SERIAL_PORT_MAIN;
        return write(buffer, size, targetMask);
    }
    
    // Write methods with specific bitmask
    size_t write(uint8_t c, uint8_t portMask) {
        size_t bytesWritten = 0;
        
        if (portMask & SERIAL_PORT_MAIN) {
            bytesWritten += Serial.write(c);
        }
        
        if (usbSer1Enabled && (portMask & SERIAL_PORT_USBSER1)) {
           //  #ifdef USE_TINYUSB
            bytesWritten += USBSer1.write(c);
           //  #endif
        }
        
        if (serial1Enabled && (portMask & SERIAL_PORT_SERIAL1)) {
            bytesWritten += Serial1.write(c);
        }
        
        if (usbSer2Enabled && (portMask & SERIAL_PORT_USBSER2)) {
           //  #ifdef USE_TINYUSB
            bytesWritten += USBSer2.write(c);
           //  #endif
        }
        
        if (serial2Enabled && (portMask & SERIAL_PORT_SERIAL2)) {
            bytesWritten += Serial2.write(c);
        }
        
        return bytesWritten;
    }
    
    size_t write(const uint8_t* buffer, size_t size, uint8_t portMask) {
        size_t bytesWritten = 0;
        
        if (portMask & SERIAL_PORT_MAIN) {
            bytesWritten += Serial.write(buffer, size);
        }
        
        if (usbSer1Enabled && (portMask & SERIAL_PORT_USBSER1)) {
           //  #ifdef USE_TINYUSB
            bytesWritten += USBSer1.write(buffer, size);
           //  #endif
        }
        
        if (serial1Enabled && (portMask & SERIAL_PORT_SERIAL1)) {
            bytesWritten += Serial1.write(buffer, size);
        }
        
        if (usbSer2Enabled && (portMask & SERIAL_PORT_USBSER2)) {
           //  #ifdef USE_TINYUSB
            bytesWritten += USBSer2.write(buffer, size);
           //  #endif
        }
        
        if (serial2Enabled && (portMask & SERIAL_PORT_SERIAL2)) {
            bytesWritten += Serial2.write(buffer, size);
        }
        
        return bytesWritten;
    }
    
        virtual void flush() override {
        // Use serialTarget if set, otherwise default to main Serial
        uint8_t targetMask = serialTarget ? serialTarget : SERIAL_PORT_MAIN;
        flush(targetMask);
    }
    
    void flushAll(void) {
        Serial.flush();
        
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer1.flush();
           //  #endif
        }
        
        if (serial1Enabled) {
            Serial1.flush();
        }
        
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer2.flush();
           //  #endif
        }
        
        if (serial2Enabled) {
            Serial2.flush();
        }
    }
    
    // Flush methods with specific bitmask
    void flush(uint8_t portMask) {
        if (portMask & SERIAL_PORT_MAIN) {
            Serial.flush();
        }
        
        if (usbSer1Enabled && (portMask & SERIAL_PORT_USBSER1)) {
           //  #ifdef USE_TINYUSB
            USBSer1.flush();
           //  #endif
        }
        
        if (serial1Enabled && (portMask & SERIAL_PORT_SERIAL1)) {
            Serial1.flush();
        }
        
        if (usbSer2Enabled && (portMask & SERIAL_PORT_USBSER2)) {
           //  #ifdef USE_TINYUSB
            USBSer2.flush();
           //  #endif
        }
        
        if (serial2Enabled && (portMask & SERIAL_PORT_SERIAL2)) {
            Serial2.flush();
        }
    }
    
    // Individual flush methods (kept for backward compatibility)
    void flushUSBSer1(void) {
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer1.flush();
           //  #endif
        }
    }
    
    void flushSerial1(void) {
        if (serial1Enabled) {
            Serial1.flush();
        }
    }
    
    void flushUSBSer2(void) {
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer2.flush();
           //  #endif
        }
    }
    
    void flushSerial2(void) {
        if (serial2Enabled) {
            Serial2.flush();
        }
    }
    
    // Additional Serial methods
    unsigned long baud(void) {
        return Serial.baud();
    }
    
    uint8_t stopbits(void) {
        return Serial.stopbits();
    }
    
    uint8_t paritytype(void) {
        return Serial.paritytype();
    }
    
    uint8_t numbits(void) {
        return Serial.numbits();
    }
    
    bool dtr(void) {
        return Serial.dtr();
    }
    
    // DTR for USBSer1/USBSer2
    bool dtrUSBSer1(void) {
      //          #ifdef USE_TINYUSB
        return USBSer1.dtr();
      //  #else
      //  return false;
      //  #endif
    }
    
    bool dtrUSBSer2(void    ) {
      //  #ifdef USE_TINYUSB
        return USBSer2.dtr();
      //  #else
      //  return false;
      //  #endif
    }
    
    operator bool(void) {
        return Serial;
    }
    
    // Read methods with timeout
    size_t readBytes(char* buffer, size_t length) {
        return Stream::readBytes(buffer, length);
    }
    
    size_t readBytes(uint8_t* buffer, size_t length) {
        return Stream::readBytes((char*)buffer, length);
    }
    
    size_t readBytesUntil(char terminator, char* buffer, size_t length) {
        return Stream::readBytesUntil(terminator, buffer, length);
    }
    
    size_t readBytesUntil(char terminator, uint8_t* buffer, size_t length) {
        return Stream::readBytesUntil(terminator, (char*)buffer, length);
    }
    
    String readString() {
        return Stream::readString();
    }
    
    String readStringUntil(char terminator) {
        return Stream::readStringUntil(terminator);
    }
    
    // Timeout control
    void setTimeout(unsigned long timeout) {
        Stream::setTimeout(timeout);
    }
    
    // Find methods
    bool find(const char* target) {
        return Stream::find(target);
    }
    
    bool find(const char* target, size_t length) {
        return Stream::find(target, length);
    }
    
    bool findUntil(const char* target, const char* terminator) {
        return Stream::findUntil(target, terminator);
    }
    
    bool findUntil(const char* target, size_t targetLen, const char* terminator, size_t termLen) {
        return Stream::findUntil(target, targetLen, terminator, termLen);
    }
    
    // parseInt and parseFloat
    long parseInt() {
        return Stream::parseInt();
    }
    
    long parseInt(LookaheadMode lookahead) {
        return Stream::parseInt(lookahead);
    }
    
    long parseInt(LookaheadMode lookahead, char ignore) {
        return Stream::parseInt(lookahead, ignore);
    }
    
    float parseFloat() {
        return Stream::parseFloat();
    }
    
    float parseFloat(LookaheadMode lookahead) {
        return Stream::parseFloat(lookahead);
    }
    
    float parseFloat(LookaheadMode lookahead, char ignore) {
        return Stream::parseFloat(lookahead, ignore);
    }
    
    // Print methods (inherited from Print class through Stream)
    using Print::print;
    using Print::println;
    using Print::write;
    using Print::printf;
    
    // Specific port printing methods
    template<typename T>
    size_t printAll(T val) {
        size_t bytesWritten = Serial.print(val);
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer1.print(val);
           //  #endif
        }
        if (serial1Enabled) {
            Serial1.print(val);
        }
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer2.print(val);
           //  #endif
        }
        if (serial2Enabled) {
            Serial2.print(val);
        }
        return bytesWritten;
    }
    
    template<typename T>
    size_t printlnAll(T val) {
        size_t bytesWritten = Serial.println(val);
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer1.println(val);
           //  #endif
        }
        if (serial1Enabled) {
            Serial1.println(val);
        }
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            USBSer2.println(val);
           //  #endif
        }
        if (serial2Enabled) {
            Serial2.println(val);
        }
        return bytesWritten;
    }
    
    // Print methods with specific bitmask - removed to avoid ambiguity with Arduino Print class
    // Use individual port methods or printAll() instead
    
    // Print to individual ports
    template<typename T>
    size_t printUSBSer1(T val) {
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            return USBSer1.print(val);
           //  #endif
        }
        return 0;
    }
    
    template<typename T>
    size_t printlnUSBSer1(T val) {
        if (usbSer1Enabled) {
           //  #ifdef USE_TINYUSB
            return USBSer1.println(val);
           //  #endif
        }
        return 0;
    }
    
    template<typename T>
    size_t printSerial1(T val) {
        if (serial1Enabled) {
            return Serial1.print(val);
        }
        return 0;
    }
    
    template<typename T>
    size_t printlnSerial1(T val) {
        if (serial1Enabled) {
            return Serial1.println(val);
        }
        return 0;
    }
    
    template<typename T>
    size_t printUSBSer2(T val) {
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            return USBSer2.print(val);
           //  #endif
        }
        return 0;
    }
    
    template<typename T>
    size_t printlnUSBSer2(T val) {
        if (usbSer2Enabled) {
           //  #ifdef USE_TINYUSB
            return USBSer2.println(val);
           //  #endif
        }
        return 0;
    }
    
    template<typename T>
    size_t printSerial2(T val) {
        if (serial2Enabled) {
            return Serial2.print(val);
        }
        return 0;
    }
    
    template<typename T>
    size_t printlnSerial2(T val) {
        if (serial2Enabled) {
            return Serial2.println(val);
        }
        return 0;
    }
    
    // Write methods for specific ports
    size_t writeAll(uint8_t c) {
        return writeToAll(c);
    }
    
    size_t writeAll(const uint8_t* buffer, size_t size) {
        return writeToAll(buffer, size);
    }
    

    
    // Convenience methods for common operations
    int availableAny() {
        return available(SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1 | SERIAL_PORT_SERIAL1 | SERIAL_PORT_USBSER2 | SERIAL_PORT_SERIAL2);
    }
    
    void flushMain() {
        flush(SERIAL_PORT_MAIN);
    }
    
    size_t writeToMain(uint8_t c) {
        return write(c, SERIAL_PORT_MAIN);
    }
    
    size_t writeToMain(const uint8_t* buffer, size_t size) {
        return write(buffer, size, SERIAL_PORT_MAIN);
    }
    
    // Removed printToMain and printlnToMain to avoid ambiguity
    // Use Serial.print() directly (respects serialTarget) or individual port methods
    
    // Write to individual ports
    size_t writeUSBSer1(uint8_t c) {
        return writeToUSBSer1(c);
    }
    
    size_t writeUSBSer1(const uint8_t* buffer, size_t size) {
        return writeToUSBSer1(buffer, size);
    }
    
    size_t writeSerial1(uint8_t c) {
        return writeToSerial1(c);
    }
    
    size_t writeSerial1(const uint8_t* buffer, size_t size) {
        return writeToSerial1(buffer, size);
    }
    
    size_t writeUSBSer2(uint8_t c) {
        return writeToUSBSer2(c);
    }
    
    size_t writeUSBSer2(const uint8_t* buffer, size_t size) {
        return writeToUSBSer2(buffer, size);
    }
    
    size_t writeSerial2(uint8_t c) {
        return writeToSerial2(c);
    }
    
    size_t writeSerial2(const uint8_t* buffer, size_t size) {
        return writeToSerial2(buffer, size);
    }
    
    // Additional helper methods
    void clearReadBuffer() {
        readBufferHead = 0;
        readBufferTail = 0;
    }
    
    void fillReadBuffer(const char* data, size_t length) {
        for (size_t i = 0; i < length; ++i) {
            // Check if buffer is full (cannot write one more byte and then increment head)
            if (((readBufferHead + 1) % READ_BUFFER_SIZE) == readBufferTail) {
                // Buffer is full, stop adding data
                break; 
            }
            readBuffer[readBufferHead] = static_cast<uint8_t>(data[i]);
            readBufferHead = (readBufferHead + 1) % READ_BUFFER_SIZE;
        }
    }
    
    bool isUSBSer1Enabled() const {
        return usbSer1Enabled;
    }
    
    bool isSerial1Enabled() const {
        return serial1Enabled;
    }
    
    bool isUSBSer2Enabled() const {
        return usbSer2Enabled;
    }
    
    bool isSerial2Enabled() const {
        return serial2Enabled;
    }
    
    // Methods to control Serial redirection
    static void setSerialTarget(uint8_t targetMask) {
        serialTarget = targetMask;
    }
    
    static uint8_t getSerialTarget() {
        return serialTarget;
    }
    
    static void enableSerialRedirection(uint8_t targetMask) {
        serialTarget = targetMask;
    }
    
    static void disableSerialRedirection() {
        serialTarget = 0;
    }
    
    static bool isSerialRedirectionEnabled() {
        return serialTarget != 0;
    }
};

// Global instance for additional serial functionality
extern SerialWrapper SerialWrap;

#endif // SERIALWRAPPER_H
