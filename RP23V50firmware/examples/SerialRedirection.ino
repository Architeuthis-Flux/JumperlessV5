#include "SerialWrapper.h"

// Replace Serial with SerialWrap for automatic redirection
#define Serial SerialWrap

void setup() {
    // Enable additional serial ports
    SerialWrap.enableUSBSer1(true);
    SerialWrap.enableSerial1(true);
    SerialWrap.enableUSBSer2(true);
    SerialWrap.enableSerial2(true);
    
    // Start serial communication
    SerialWrap.begin(115200);
    
    delay(1000); // Wait for serial ports to initialize
    
    // Default behavior - only main Serial
    Serial.println("This goes to main Serial only (default behavior)");
    
    // Redirect to all ports
    SERIAL_REDIRECT_ALL();
    Serial.println("This goes to ALL enabled serial ports!");
    
    // Redirect to specific ports
    SERIAL_REDIRECT_TO(SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1);
    Serial.println("This goes to main Serial and USBSer1");
    
    // Redirect to just USBSer2
    SERIAL_REDIRECT_TO(SERIAL_PORT_USBSER2);
    Serial.println("This goes to USBSer2 only");
    
    // Turn off redirection - back to default
    SERIAL_REDIRECT_OFF();
    Serial.println("Back to main Serial only");
}

void loop() {
    // Example of dynamic redirection based on conditions
    static unsigned long lastToggle = 0;
    static bool useAllPorts = false;
    
    if (millis() - lastToggle > 5000) { // Toggle every 5 seconds
        useAllPorts = !useAllPorts;
        
        if (useAllPorts) {
            SERIAL_REDIRECT_ALL();
            Serial.println("Switched to ALL ports");
        } else {
            SERIAL_REDIRECT_OFF();
            Serial.println("Switched to main Serial only");
        }
        
        lastToggle = millis();
    }
    
    // Your main code here
    if (Serial.available()) {
        String input = Serial.readString();
        input.trim();
        
        if (input == "all") {
            SERIAL_REDIRECT_ALL();
            Serial.println("Redirecting to all ports");
        } else if (input == "main") {
            SERIAL_REDIRECT_OFF();
            Serial.println("Using main Serial only");
        } else if (input == "usb1") {
            SERIAL_REDIRECT_TO(SERIAL_PORT_USBSER1);
            Serial.println("Redirecting to USBSer1 only");
        } else if (input == "dual") {
            SERIAL_REDIRECT_TO(SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1);
            Serial.println("Redirecting to main Serial and USBSer1");
        } else if (input == "status") {
            Serial.print("Current serialTarget: 0x");
            Serial.println(SerialWrap.getSerialTarget(), HEX);
        } else if (input == "test") {
            // Demonstrate individual port methods
            SerialWrap.printUSBSer1("This goes to USBSer1 only");
            SerialWrap.printlnSerial1("This goes to Serial1 only");
            SerialWrap.printAll("This goes to all enabled ports");
        } else {
            Serial.println("Commands: all, main, usb1, dual, status, test");
        }
    }
    
    delay(100);
} 