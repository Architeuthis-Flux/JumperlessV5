"""
GPIO (General Purpose Input/Output) Basics
==========================================

This example demonstrates basic GPIO operations using the jumperless module:
- Setting GPIO pin directions (input/output)
- Reading and writing GPIO pin states
- Configuring pull-up/pull-down resistors
- GPIO blinking patterns
- Multi-pin control

Functions demonstrated:
- gpio_set(pin, value) - Set GPIO pin state (HIGH/LOW)
- gpio_get(pin) - Read GPIO pin state
- gpio_set_dir(pin, direction) - Set pin direction (INPUT/OUTPUT)
- gpio_get_dir(pin) - Get pin direction
- gpio_set_pull(pin, pull) - Set pull resistor configuration
- gpio_get_pull(pin) - Get pull resistor configuration
- oled_print(text) - Display text on OLED

GPIO Pins:
- Pins 1-8: GPIO 1-8 (routable GPIO)
- Pin 9: UART TX
- Pin 10: UART RX
- Values: True/False or 1/0 for HIGH/LOW
- Directions: True/False or 1/0 for OUTPUT/INPUT
- Pull resistors: -1/0/1 for PULL_DOWN/NONE/PULL_UP
"""

import time

def gpio_basic_demo():
    """Demonstrate basic GPIO operations"""
    
    oled_clear()
    oled_print("GPIO Basic Demo")
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                              GPIO BASIC DEMO                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Test basic GPIO operations on pins 1-4
    test_pins = [1, 2, 3, 4]
    
    print("\n☺ Testing basic GPIO operations:")
    
    for pin in test_pins:
        # Set pin as output
        gpio_set_dir(pin, True)  # True = OUTPUT
        direction = gpio_get_dir(pin)
        print("  GPIO" + str(pin) + " direction: " + direction)
        
        # Set pin HIGH
        gpio_set(pin, True)
        state = gpio_get(pin)
        print("  GPIO" + str(pin) + " state: " + state)
        
        oled_clear()
        oled_print("GPIO" + str(pin) + ": " + state)
        time.sleep(1)
        
        # Set pin LOW
        gpio_set(pin, False)
        state = gpio_get(pin)
        print("  GPIO" + str(pin) + " state: " + state)
        
        oled_clear()
        oled_print("GPIO" + str(pin) + ": " + state)
        time.sleep(1)
    
    print("\n✓ Basic GPIO operations complete")

def gpio_blink_demo():
    """Demonstrate GPIO blinking patterns"""
    
    oled_clear()
    oled_print("GPIO Blink Demo")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                              GPIO BLINK DEMO                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Blink GPIO1 10 times
    blink_pin = 1
    gpio_set_dir(blink_pin, True)  # Set as output
    
    print("\n☺ Blinking GPIO" + str(blink_pin) + " 10 times:")
    
    for blink in range(10):
        # Turn on
        gpio_set(blink_pin, True)
        state = gpio_get(blink_pin)
        
        oled_clear()
        oled_print("GPIO" + str(blink_pin) + " ON")
        print("  Blink " + str(blink + 1) + "/10: " + state)
        
        time.sleep(0.3)
        
        # Turn off
        gpio_set(blink_pin, False)
        state = gpio_get(blink_pin)
        
        oled_clear()
        oled_print("GPIO" + str(blink_pin) + " OFF")
        
        time.sleep(0.3)
    
    print("\n✓ Blink demo complete")

def gpio_input_demo():
    """Demonstrate GPIO input modes with pull resistors"""
    
    oled_clear()
    oled_print("GPIO Input Demo")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                              GPIO INPUT DEMO                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    input_pin = 5
    
    # Test different pull resistor configurations
    pull_configs = [
        (1, "PULL_UP"),
        (0, "NONE"),
        (-1, "PULL_DOWN")
    ]
    
    print("\n☺ Testing GPIO" + str(input_pin) + " input modes:")
    
    for pull_value, pull_name in pull_configs:
        # Set pin as input with pull resistor
        gpio_set_dir(input_pin, False)  # False = INPUT
        gpio_set_pull(input_pin, pull_value)
        
        # Verify configuration
        direction = gpio_get_dir(input_pin)
        pull_config = gpio_get_pull(input_pin)
        
        print("  Direction: " + direction)
        print("  Pull: " + pull_config)
        
        # Read pin state for 3 seconds
        print("  Reading pin state with " + pull_name + "...")
        
        for reading in range(6):
            state = gpio_get(input_pin)
            
            oled_clear()
            oled_print("GPIO" + str(input_pin) + " " + pull_name + " " + state)
            print("    Reading " + str(reading + 1) + ": " + state)
            
            time.sleep(0.5)
        
        print("")
    
    print("✓ Input mode demo complete")

def gpio_multipin_demo():
    """Demonstrate multi-pin GPIO control"""
    
    oled_clear()
    oled_print("Multi-pin Demo")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                             MULTI-PIN DEMO                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Use GPIO pins 1-4 for binary counting
    pins = [1, 2, 3, 4]
    
    print("\n☺ Setting up GPIO pins 1-4 as outputs:")
    
    # Set all pins as outputs
    for pin in pins:
        gpio_set_dir(pin, True)
        direction = gpio_get_dir(pin)
        print("  GPIO" + str(pin) + " direction: " + direction)
    
    # Binary counting from 0 to 15
    print("\n☺ Binary counting demo (0-15):")
    
    for count in range(16):
        # Convert count to binary and set pins
        binary_str = format(count, '04b')  # 4-bit binary
        
        print("  Count " + str(count) + " (binary: " + binary_str + "):")
        
        for i, pin in enumerate(pins):
            bit_value = int(binary_str[i])
            gpio_set(pin, bit_value)
            state = gpio_get(pin)
            print("    GPIO" + str(pin) + ": " + state)
        
        oled_clear()
        oled_print("Count: " + str(count) + " (" + binary_str + ")")
        time.sleep(0.8)
    
    # Reset all pins to LOW
    print("\n☺ Resetting all pins to LOW:")
    for pin in pins:
        gpio_set(pin, False)
        state = gpio_get(pin)
        print("  GPIO" + str(pin) + ": " + state)
    
    print("\n✓ Multi-pin demo complete")

def gpio_status_demo():
    """Show status of all GPIO pins"""
    
    oled_clear()
    oled_print("GPIO Status")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                              GPIO STATUS                                     │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    print("\n☺ Current status of all GPIO pins:")
    
    # Check status of pins 1-8
    for pin in range(1, 9):
        try:
            direction = gpio_get_dir(pin)
            state = gpio_get(pin)
            pull = gpio_get_pull(pin)
            
            print("  GPIO" + str(pin) + " - Dir: " + direction + ", State: " + state + ", Pull: " + pull)
            
            # Show on OLED (cycle through pins)
            oled_clear()
            oled_print("GPIO" + str(pin) + ": " + direction + " " + state)
            time.sleep(1)
            
        except Exception as e:
            print("  GPIO" + str(pin) + " - Error reading: " + str(e))
    
    print("\n✓ GPIO status check complete")

def gpio_advanced_patterns():
    """Demonstrate advanced GPIO patterns"""
    
    oled_clear()
    oled_print("Advanced Patterns")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           ADVANCED PATTERNS                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    pins = [1, 2, 3, 4, 5, 6]
    
    # Set all pins as outputs
    print("\n☺ Setting up GPIO pins 1-6 as outputs:")
    for pin in pins:
        gpio_set_dir(pin, True)
    
    # Pattern 1: Running light
    print("\n☺ Running light pattern:")
    for cycle in range(3):
        for pin in pins:
            # Turn on current pin, turn off others
            for p in pins:
                gpio_set(p, p == pin)
            
            oled_clear()
            oled_print("Running: GPIO" + str(pin))
            time.sleep(0.2)
    
    # Pattern 2: Binary up/down counter
    print("\n☺ Binary up/down counter:")
    # Count up
    for count in range(8):
        binary_str = format(count, '03b')
        for i in range(3):
            gpio_set(pins[i], int(binary_str[i]))
        
        oled_clear()
        oled_print("Up: " + str(count) + " (" + binary_str + ")")
        time.sleep(0.4)
    
    # Count down
    for count in range(7, -1, -1):
        binary_str = format(count, '03b')
        for i in range(3):
            gpio_set(pins[i], int(binary_str[i]))
        
        oled_clear()
        oled_print("Down: " + str(count) + " (" + binary_str + ")")
        time.sleep(0.4)
    
    # Turn off all pins
    print("\n☺ Turning off all pins:")
    for pin in pins:
        gpio_set(pin, False)
    
    print("\n✓ Advanced patterns complete")

def run_all_gpio_demos():
    """Run all GPIO demonstration functions"""
    
    print("🚀 Starting Complete GPIO Demonstration")
    print("═" * 75)
    
    gpio_basic_demo()
    gpio_blink_demo()
    gpio_input_demo()
    gpio_multipin_demo()
    gpio_status_demo()
    gpio_advanced_patterns()
    
    oled_clear()
    oled_print("GPIO Demo Complete!")
    print("\n🎉 All GPIO demonstrations complete!")
    print("═" * 75)

# Run the demonstration
if __name__ == "__main__":
    run_all_gpio_demos() 