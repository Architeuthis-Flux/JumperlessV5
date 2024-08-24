# SafeString
This SafeString library is designed for beginners to be a safe, robust and debuggable replacement for string processing in Arduino and provides non-blocking text I/O and parsing and testing for Real World use.

This library includes:-  
* **SafeString**, a safe, robust and debuggable replacement for string processing in Arduino  
* **SafeStringReader**, a non-blocking tokenizing text reader replacement for Serial read()  
* **BufferedOutput**, non-blocking replacement for Serial print()  
* **SafeStringStream**, a stream to provide test inputs for repeated testing of I/O sketches   
* **BufferedInput**, extra buffering for text input  
* **loopTimer**, (loopTimerClass) to track of the maximum and average run times for the loop()  
* **millisDelay**, a non-blocking delay replacement, with single-shot, repeating, restart and stop facilities.  
* **PinFlasher**, a non-blocking flashing of an output pin.  
* **SerialComs**, to send messages between Arduinos via Serial

  To create SafeStrings use one of the four (4) macros **createSafeString** or **cSF**, **createSafeStringFromCharArray** or **cSFA**, **createSafeStringFromCharPtr** or **cSFP**, **createSafeStringFromCharPtrWithSize** or **cSFPS**<br> 
  For example sketches see SafeString_ConstructorAndDebugging.ino, SafeStringFromCharArray.ino, SafeStringFromCharPtr.ino and SafeStringFromCharPtrWithSize.ino<br>
  and the [SafeString Tutorial](https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html)
    
# How-To
See [SafeString Documentation](https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/docs/html/index.html)  
See [SafeString Tutorial](https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html)  
See [Arduino to Arduino/PC via Serial](https://www.forward.com.au/pfod/ArduinoProgramming/SoftwareSolutions/ComsPair.html)  
See [Arduino Text I/O for the Real World](https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html)  
See [Simple Multitasking Arduino](https://www.forward.com.au/pfod/ArduinoProgramming/RealTimeArduino/index.html)  
See [How to code Timers and Delays in Arduino](https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html)  

# Sparkfun SAMD compile issue  
See issue https://github.com/PowerBroker2/SafeString/issues/73  

Sparkfun's SAMD board support does not provide a define to distinguish it from Arduino's SAMD boards.  
This causes SafeString compile to fail with  
    
 **error: expected class-name before '{' token 73 | class SafeStringReader : public SafeString {**      

This fix is to remove the contents of these three header files  
SafeStringNameSpace.h  
SafeStringNameSpaceEnd.h  
SafeStringNameSpaceStart.h  

That is just have empty files for those three headers.  

# PlatformIO support
This library is primarily an Arduino IDE library, but users have had success using it with PlatformIO.  
See the PlatformIO subdirectory for the two PlatformIO versions.  One for boards that use the arduino namespace, e.g. Arduino Zero, and one for boards that don't, e.g. UNO.
To see which board yours is, check the Print.h file and see if it includes the lines   

    namespace arduino {  
    class Print    

If it does, then unzip the SafeStringIO_namespace.zip and use that library.  If it does not, then unzip the SafeStringIO.zip and use that library.

# Software License
See the top of each file for its license

# Note
Note, this is NOT my work, I am simply hosting it for easy access. The original code belongs to [Forward Computing and Control Pty. Ltd](https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html).

# Revisions
V4.1.33 fix for bool for SAM boards  
V4.1.32 added support for Adafruit SAMD boards  
V4.1.31 added support for other Arduino MBED boards like Opta   
V4.1.30 added default initializations   
V4.1.29 removed F() macro and FlashStringHelper class from SafeString.h.  SafeString now depends of the board's core to define these.   
V4.1.28 fixes for Arduino UNO R4 and R4 WiFi   
V4.1.27 revised defines for Arduino Zero   
V4.1.26 fixed F()for Arduino IDE 2 and ESP32     
V4.1.25 added NULL detection to readFrom(const char*, unsigned int maxCharsToRead)     
V4.1.24 added NULL detection to readFrom(const char*)     
V4.1.23 added readFrom(const char*)     
V4.1.22 added link to docs in README.md     
V4.1.21 minor change document correction     
V4.1.20 fixed for ESP32 V2.0.3 dtostrf     
V4.1.19 minor change to PinFlasher to support ESP32_WS2812Flasher extension     
V4.1.18 changed firstToken() arguments now defaults to returning empty tokens     
V4.1.17 restored firstToken() method     
V4.1.16 minor text corrections, SafeStringReader.getDelimiter() now returns int, fix for SafeStringReader.flushInput()     
V4.1.15 added support for ARDUINO_ARCH_NRF52 and ARDUINO_ARCH_NRF5    
V4.1.14 minor fix PinFlasher  
V4.1.13 minor change PinFlasher to simplify loop logic    
V4.1.12 adds PinFlasher class  
V4.1.11 adds support for Ardafruit M4 and Moteino M4  
V4.1.10 fixed SafeStringStream to handle data >= 0x80  
V4.1.9 added toUnsignedLong coversions, removed firstToken()  
V4.1.7-8 added MegaTinyCore support 
V4.1.6 added Teensy2 to Teensy4.1 support (dtostrf)  
V4.1.5 SerialComs timeout 5sec, added firstToken()  
V4.1.4 fixed dtostrf support  
V4.1.3 added Arduino Due support  
V4.1.2 fixed SerialComs when msg times out without delimiter, fixed support for Adafruit Feather nRF52 (V0.21.0)  
V4.1.1 fixed nullpointer, check for Out-Of-Memory on createSafeString, support for Earl Philhower's pi pico board package  
V4.1.0 added SerialComs class for Arduino to Arduino/PC via Serial, added fixed width formatting, print(value,decPlaces,width) see example SafeString_Tests/SafeString_fixedWidthFormat.ino  
V4.0.5 added returnEmptyTokens() option to SafeStringReader  
V4.0.4 adds support for Raspberry Pi Pico using Arduino Mbed OS RP2040 V2.0.0 board package ,nextToken() now returns last un-terminated token by default (can be overridded by optional arg), option to return empty tokens  
V4.0.3 allow createSafeString for small sizes, fixed bool for DUE (ARDUINO_ARCH_SAM)  
V4.0.2 added flushInput() method to SafeStringReader  
V4.0.1 fixed SafeStringReader timeout and NanoBLE F() macro  
V4.0.0 changes method returns to better match Arduino String methods, main change is indexOf now returns int and returns -1 if not found  
V3.1.0 adds hasError() method  
V3.0.6 adds support for Arduino megaAVR boards  
V3.0.5 adds support for SparkFun Redboard Turbo,but may interfer with other SAM ZERO based boards, also adds support for Due and STM32F1 and STM32F4  

