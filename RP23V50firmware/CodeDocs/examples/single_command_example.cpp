/*
 * Example: Single MicroPython Command Execution from main.cpp
 * 
 * This example demonstrates how to execute individual MicroPython commands
 * from main.cpp without entering the full REPL interface.
 * 
 * Features:
 * - Quiet MicroPython initialization (no output)
 * - Automatic jumperless. prefix handling
 * - Simple function calls for common use cases
 */

#include <Arduino.h>
#include "Python_Proper.h"

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000); // Wait for serial connection
    
    Serial.println("=== Single Command Example ===");
    
    // The global stream is used for MicroPython output
    setGlobalStream(&Serial);
    
    // Example 1: Execute commands with automatic prefix handling
    Serial.println("\n1. Basic command execution:");
    
    // These commands will automatically get "jumperless." prefix
    bool success = executeSinglePythonCommand("dac_set(0, 2.5)");
    Serial.printf("Set DAC 0 to 2.5V: %s\n", success ? "OK" : "FAILED");
    
    success = executeSinglePythonCommand("gpio_set(3, True)");
    Serial.printf("Set GPIO 3 HIGH: %s\n", success ? "OK" : "FAILED");
    
    // Example 2: Get numeric results
    Serial.println("\n2. Reading sensor values:");
    
    float adc_value = 0.0f;
    success = executeSinglePythonCommandFloat("adc_get(0)", &adc_value);
    Serial.printf("ADC 0 reading: %.3fV (%s)\n", adc_value, success ? "OK" : "FAILED");
    
    float gpio_state = 0.0f;
    success = executeSinglePythonCommandFloat("gpio_get(2)", &gpio_state);
    Serial.printf("GPIO 2 state: %.0f (%s)\n", gpio_state, success ? "OK" : "FAILED");
    
    // Example 3: Quick command for simple readings
    Serial.println("\n3. Quick command function:");
    
    float quick_result = quickPythonCommand("ina_get_current(0)");
    Serial.printf("INA current reading: %.3fmA\n", quick_result);
    
    // Example 4: Commands that already have prefix or are standard Python
    Serial.println("\n4. Mixed command types:");
    
    // This already has prefix - won't add another
    executeSinglePythonCommand("jumperless.oled_print('Hello!')");
    
    // Standard Python command - no prefix added
    executeSinglePythonCommand("print('From Python!')");
    
    // Example 5: Node connections
    Serial.println("\n5. Node connections:");
    
    executeSinglePythonCommand("connect(1, 5)");  // Will become jumperless.connect(1, 5)
    Serial.println("Connected nodes 1 and 5");
    
    delay(1000);
    
    executeSinglePythonCommand("disconnect(1, 5)");
    Serial.println("Disconnected nodes 1 and 5");
    
    Serial.println("\n=== Example Complete ===");
}

void loop() {
    // In main loop, you can execute commands as needed
    static unsigned long lastRead = 0;
    
    if (millis() - lastRead > 5000) {  // Every 5 seconds
        lastRead = millis();
        
        // Read some sensors
        float adc_reading = quickPythonCommand("adc_get(0)");
        float current_reading = quickPythonCommand("ina_get_current(0)");
        
        Serial.printf("ADC: %.3fV, Current: %.3fmA\n", adc_reading, current_reading);
        
        // You could also update an OLED display
        char oled_text[32];
        snprintf(oled_text, sizeof(oled_text), "oled_print('ADC: %.2fV')", adc_reading);
        executeSinglePythonCommand(oled_text);
    }
    
    // Other main loop code here...
    delay(10);
}

/*
 * Usage Notes:
 * 
 * 1. executeSinglePythonCommand(command, buffer, size)
 *    - Executes any MicroPython command
 *    - Automatically adds "jumperless." prefix for hardware functions
 *    - Returns true/false for success
 *    - Optional result buffer for string results
 * 
 * 2. executeSinglePythonCommandFloat(command, &result)
 *    - Same as above but returns numeric result in float pointer
 *    - Useful for sensor readings, GPIO states, etc.
 * 
 * 3. quickPythonCommand(command)
 *    - Convenience function that returns float result directly
 *    - Returns 0.0f on error
 *    - Best for simple sensor readings
 * 
 * 4. Automatic Prefix Handling:
 *    - "gpio_get(2)" becomes "jumperless.gpio_get(2)"
 *    - "jumperless.dac_set(0, 3.3)" stays unchanged
 *    - "print('test')" stays unchanged (standard Python)
 * 
 * 5. Supported Hardware Functions (get automatic prefix):
 *    - DAC: dac_set, dac_get
 *    - ADC: adc_get
 *    - INA: ina_get_current, ina_get_voltage, ina_get_bus_voltage, ina_get_power
 *    - GPIO: gpio_set, gpio_get, gpio_set_dir, gpio_get_dir, gpio_set_pull, gpio_get_pull
 *    - Nodes: connect, disconnect, nodes_clear, is_connected
 *    - OLED: oled_print, oled_clear, oled_show, oled_connect, oled_disconnect
 *    - Other: arduino_reset, probe_tap, clickwheel_up, clickwheel_down, help
 */ 