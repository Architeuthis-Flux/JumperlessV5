/*
 * Jumperless MicroPython Proper Implementation Demo
 * 
 * This demonstrates the proper MicroPython porting approach using
 * character-by-character I/O instead of the custom REPL parser.
 * 
 * Benefits over the old implementation:
 * - Uses MicroPython's built-in REPL with all standard features
 * - Proper auto-completion, multi-line input, command history
 * - Character-by-character processing integrates with Arduino loop()
 * - Much simpler and more maintainable code
 * - Follows official MicroPython porting guidelines
 */

#include <Arduino.h>
#include "Python_Proper.h"

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        delay(10);
    }
    
    Serial.println("=== Jumperless MicroPython Proper Demo ===");
    Serial.println("Initializing MicroPython with proper porting approach...");
    
    // Initialize MicroPython properly
    if (initMicroPythonProper()) {
        Serial.println("MicroPython initialized successfully!");
        
        // Add Jumperless hardware control functions
        addJumperlessPythonFunctions();
        
        // Show some example usage
        Serial.println("\nTesting basic Python execution:");
        executePythonCodeProper("print('Hello from properly ported MicroPython!')");
        executePythonCodeProper("import sys; print('Version:', sys.version)");
        
        // Demonstrate hardware functions
        Serial.println("\nTesting hardware integration:");
        executePythonCodeProper("help_jumperless()");
        
        Serial.println("\n=== MicroPython Ready ===");
        Serial.println("Type 'python' to start Python REPL");
        Serial.println("Type 'status' to show MicroPython status");
        Serial.println("Other Arduino code continues running...");
        
    } else {
        Serial.println("Failed to initialize MicroPython!");
    }
}

void loop() {
    // Process MicroPython input if REPL is active
    processMicroPythonInput();
    
    // Handle commands when REPL is not active
    if (!isMicroPythonREPLActive() && Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command == "python" || command == "repl") {
            Serial.println("=== Starting Python REPL ===");
            Serial.println("Standard Python REPL features available:");
            Serial.println("- Tab completion, multi-line input, command history");
            Serial.println("- Type 'help()' for Python help");
            Serial.println("- Type 'help_jumperless()' for hardware commands");
            Serial.println("- Type 'quit' or 'exit' to return to Arduino mode");
            Serial.println();
            startMicroPythonREPL();
            
        } else if (command == "status") {
            printMicroPythonStatus();
            
        } else if (command == "help") {
            Serial.println("\nAvailable commands:");
            Serial.println("  python - Start Python REPL");
            Serial.println("  status - Show MicroPython status");
            Serial.println("  help   - Show this help");
            Serial.println();
            
        } else if (command.length() > 0) {
            Serial.println("Unknown command. Type 'help' for available commands.");
        }
    }
    
    // Your other Arduino code goes here
    // This runs continuously when REPL is not active
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 1000) {
        // Example: blink LED to show Arduino is running
        // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastBlink = millis();
    }
    
    // Small delay to prevent overwhelming the system
    delayMicroseconds(100);
}

/*
 * Example session you can try:
 * 
 * MicroPython Ready
 * Type 'python' to start Python REPL
 * 
 * help
 * Available commands:
 *   python - Start Python REPL
 *   status - Show MicroPython status
 *   help   - Show this help
 * 
 * python
 * === Starting Python REPL ===
 * >>> print("Hello Jumperless!")
 * Hello Jumperless!
 * >>> for i in range(3):
 * ...     connect(i, i+10)
 * ...     print(f"Connected {i} to {i+10}")
 * ... 
 * Connected 0 to 10
 * Connected 1 to 11
 * Connected 2 to 12
 * >>> voltage(5, 3.3)
 * OK
 * >>> help_jumperless()
 * Jumperless Hardware Commands:
 *   connect(node1, node2)  - Connect two nodes
 *   ...
 * >>> quit
 * [MP] Exiting REPL...
 * (Back to Arduino mode - your other code continues running)
 */ 