/*
 * Simple Example: Single Function Call for MicroPython REPL
 * 
 * This shows how to use the new enterMicroPythonREPL() function
 * from your main.cpp. It's a single blocking call that handles
 * everything until the user types 'quit'.
 */

#include <Arduino.h>
#include "Python_Proper.h"

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("Jumperless with MicroPython");
    Serial.println("Your Arduino code runs here...");
    
    // Do your normal Arduino setup
    // pinMode(LED_BUILTIN, OUTPUT);
    // setupHardware();
    // etc.
}

void loop() {
    // Your normal Arduino code
    static int counter = 0;
    Serial.printf("Arduino loop running... %d\n", counter++);
    
    // Check if user wants to enter Python
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input == "python" || input == "py") {
            Serial.println("\nEntering Colorful MicroPython REPL...\n");
            
            // This single function call handles everything:
            // - Initializes MicroPython (if not already done)
            // - Shows colorful welcome screen with menu
            // - Loads complete hardware module (jl.gpio.get(), etc.)
            // - Starts colorful REPL with proper prompts
            // - Runs until user types 'quit'
            // - Returns control back here
            enterMicroPythonREPL();
            
            Serial.println("\nBack to Arduino mode\n");
            
        } else if (input == "help") {
            Serial.println("Commands:");
            Serial.println("  python - Enter MicroPython REPL");
            Serial.println("  help   - Show this help");
            
        } else if (input.length() > 0) {
            Serial.println("Unknown command. Type 'help' for available commands.");
        }
    }
    
    // Your other Arduino code continues here
    delay(1000);
}

/*
 * Example session:
 * 
 * Jumperless with MicroPython
 * Your Arduino code runs here...
 * Arduino loop running... 0
 * Arduino loop running... 1
 * 
 * python
 * Entering Colorful MicroPython REPL...
 * 
 * [Colorful welcome screen appears]
 * MicroPython REPL with embedded Jumperless hardware control!
 * Type normal Python code, then press Enter to execute
 * 
 * Commands:
 *   'quit'     - Exit REPL
 *   'jl.help()' - Show hardware commands
 * 
 * Press enter to start REPL
 * 
 * >>> print("Hello from Python!")
 * Hello from Python!
 * >>> jl.gpio.get(3)
 * HIGH
 * >>> jl.nodes.connect(1, 30)
 * SYNC_EXEC:nodes(connect, 1, 30, save=False)
 * >>> voltage = jl.adc.get(0)
 * SYNC_EXEC:adc(get, 0)
 * >>> print(f"Voltage: {voltage}V")
 * Voltage: 3.3V
 * >>> quit
 * [MP] Exiting REPL...
 * Returned to Arduino mode
 * 
 * Back to Arduino mode
 * 
 * Arduino loop running... 2
 * Arduino loop running... 3
 * ...
 */ 