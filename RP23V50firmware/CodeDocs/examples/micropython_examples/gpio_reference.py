"""
GPIO (General Purpose Input/Output) Reference
============================================

Complete reference for all GPIO functions in the Jumperless MicroPython module.
This file demonstrates every GPIO-related function with practical examples.

Functions demonstrated:
- gpio_set(pin, value) - Set GPIO pin state (HIGH/LOW)
- gpio_get(pin) - Read GPIO pin state
- gpio_set_dir(pin, direction) - Set pin direction (INPUT/OUTPUT)
- gpio_get_dir(pin) - Get pin direction
- gpio_set_pull(pin, pull) - Set pull resistor configuration
- gpio_get_pull(pin) - Get pull resistor configuration
- set_gpio(), get_gpio() - Aliases for gpio_set/get
- set_gpio_dir(), get_gpio_dir() - Aliases for gpio_set_dir/get_dir
- set_gpio_pull(), get_gpio_pull() - Aliases for gpio_set_pull/get_pull

GPIO Pins:
- Pins 1-8: GPIO 1-8 (routable GPIO)
- Pin 9: UART TX
- Pin 10: UART RX
- Values: True/False or 1/0 for HIGH/LOW
- Directions: True/False or 1/0 for OUTPUT/INPUT
- Pull resistors: 1=PULLUP, 0=NONE, -1=PULLDOWN

Usage:
  exec(open('micropython_examples/gpio_reference.py').read())
"""

import time

def gpio_basic_operations():
    """Demonstrate basic GPIO operations"""
    
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           GPIO BASIC OPERATIONS                             │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("GPIO Basic Demo")
    
    print("\n☺ Testing basic GPIO operations on pins 1-4:")
    
    # Configure pins as outputs
    for pin in range(1, 5):
        gpio_set_dir(pin, True)  # True = OUTPUT
        direction = gpio_get_dir(pin)
        print("  GPIO" + str(pin) + " set as output: " + str(direction))
    
    # Test setting and reading pin states
    print("\n☺ Testing pin states:")
    
    for pin in range(1, 5):
        # Set pin HIGH
        gpio_set(pin, True)
        state = gpio_get(pin)
        print("  GPIO" + str(pin) + " set HIGH: " + str(state))
        
        oled_clear()
        oled_print("GPIO" + str(pin) + " HIGH")
        time.sleep(0.5)
        
        # Set pin LOW
        gpio_set(pin, False)
        state = gpio_get(pin)
        print("  GPIO" + str(pin) + " set LOW: " + str(state))
        
        oled_clear()
        oled_print("GPIO" + str(pin) + " LOW")
        time.sleep(0.5)
    
    # Test using alias functions
    print("\n☺ Testing alias functions:")
    
    set_gpio(1, True)  # Using alias
    state = get_gpio(1)
    print("  set_gpio(1, True): " + str(state))
    
    set_gpio_dir(2, False)  # Set as input
    direction = get_gpio_dir(2)
    print("  set_gpio_dir(2, False): " + str(direction))
    
    print("✓ Basic GPIO operations complete")

def gpio_direction_control():
    """Demonstrate GPIO direction control"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                          GPIO DIRECTION CONTROL                             │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("GPIO Direction Demo")
    
    print("\n☺ Testing GPIO direction control:")
    
    test_pin = 3
    
    # Test OUTPUT direction
    print("  Setting GPIO" + str(test_pin) + " as OUTPUT:")
    gpio_set_dir(test_pin, True)  # True = OUTPUT
    direction = gpio_get_dir(test_pin)
    print("    Direction: " + str(direction))
    
    # Test output functionality
    gpio_set(test_pin, True)
    state = gpio_get(test_pin)
    print("    Set HIGH: " + str(state))
    
    oled_clear()
    oled_print("GPIO" + str(test_pin) + " OUT HIGH")
    time.sleep(1)
    
    # Test INPUT direction
    print("  Setting GPIO" + str(test_pin) + " as INPUT:")
    gpio_set_dir(test_pin, False)  # False = INPUT
    direction = gpio_get_dir(test_pin)
    print("    Direction: " + str(direction))
    
    # Read input state
    state = gpio_get(test_pin)
    print("    Input state: " + str(state))
    
    oled_clear()
    oled_print("GPIO" + str(test_pin) + " IN " + str(state))
    time.sleep(1)
    
    # Test all pins direction control
    print("\n☺ Testing all GPIO pins direction control:")
    
    directions = [True, False, True, False, True, False]  # Alternating
    
    for pin in range(1, 7):
        if pin <= len(directions):
            direction = directions[pin - 1]
            gpio_set_dir(pin, direction)
            actual_dir = gpio_get_dir(pin)
            dir_str = "OUTPUT" if direction else "INPUT"
            print("  GPIO" + str(pin) + " set as " + dir_str + ": " + str(actual_dir))
    
    # Show current directions on OLED
    for pin in range(1, 7):
        direction = gpio_get_dir(pin)
        dir_str = "OUT" if direction == "OUTPUT" else "IN"
        oled_clear()
        oled_print("GPIO" + str(pin) + " " + dir_str)
        time.sleep(0.8)
    
    print("✓ Direction control complete")

def gpio_pull_resistor_control():
    """Demonstrate GPIO pull resistor control"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                        GPIO PULL RESISTOR CONTROL                           │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("GPIO Pull Demo")
    
    print("\n☺ Testing GPIO pull resistor configurations:")
    
    test_pin = 5
    
    # Set pin as input for pull resistor testing
    gpio_set_dir(test_pin, False)  # Input
    print("  Set GPIO" + str(test_pin) + " as input for pull resistor testing")
    
    # Test different pull configurations
    pull_configs = [
        (1, "PULL_UP"),
        (0, "NONE"),
        (-1, "PULL_DOWN")
    ]
    
    for pull_value, pull_name in pull_configs:
        print("\n  Testing " + pull_name + " configuration:")
        
        # Set pull resistor
        gpio_set_pull(test_pin, pull_value)
        actual_pull = gpio_get_pull(test_pin)
        print("    Set pull: " + str(pull_value) + " (" + pull_name + ")")
        print("    Actual pull: " + str(actual_pull))
        
        # Read pin state multiple times to show effect
        print("    Pin readings with " + pull_name + ":")
        for reading in range(5):
            state = gpio_get(test_pin)
            print("      Reading " + str(reading + 1) + ": " + str(state))
            
            oled_clear()
            oled_print("GPIO" + str(test_pin) + " " + pull_name + " " + str(state))
            time.sleep(0.5)
    
    # Test pull resistors on multiple pins
    print("\n☺ Testing pull resistors on multiple pins:")
    
    pins = [6, 7, 8]
    pull_values = [1, 0, -1]  # PULL_UP, NONE, PULL_DOWN
    pull_names = ["PULL_UP", "NONE", "PULL_DOWN"]
    
    # Set all pins as inputs
    for pin in pins:
        gpio_set_dir(pin, False)
    
    # Set different pull configurations
    for pin, pull_val, pull_name in zip(pins, pull_values, pull_names):
        gpio_set_pull(pin, pull_val)
        actual_pull = gpio_get_pull(pin)
        print("  GPIO" + str(pin) + " pull: " + pull_name + " (" + str(actual_pull) + ")")
    
    # Read all pins
    print("\n  Pin states with different pull configurations:")
    for i in range(5):
        states = []
        for pin in pins:
            state = gpio_get(pin)
            states.append(str(state))
        
        states_str = ", ".join(["GPIO" + str(pin) + ":" + state for pin, state in zip(pins, states)])
        print("    Reading " + str(i + 1) + ": " + states_str)
        
        oled_clear()
        oled_print("Pulls: " + "-".join(states))
        time.sleep(0.5)
    
    print("✓ Pull resistor control complete")

def gpio_blink_patterns():
    """Demonstrate GPIO blinking patterns"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           GPIO BLINK PATTERNS                               │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("GPIO Blink Demo")
    
    # Set up GPIO pins as outputs
    blink_pins = [1, 2, 3, 4]
    
    print("\n☺ Setting up GPIO pins 1-4 as outputs:")
    for pin in blink_pins:
        gpio_set_dir(pin, True)
        direction = gpio_get_dir(pin)
        print("  GPIO" + str(pin) + " direction: " + str(direction))
    
    # Pattern 1: Simple blink
    print("\n☺ Pattern 1: Simple blink on GPIO1:")
    for blink in range(10):
        gpio_set(1, True)
        oled_clear()
        oled_print("GPIO1 ON")
        time.sleep(0.3)
        
        gpio_set(1, False)
        oled_clear()
        oled_print("GPIO1 OFF")
        time.sleep(0.3)
    
    # Pattern 2: Running light
    print("\n☺ Pattern 2: Running light:")
    for cycle in range(5):
        for pin in blink_pins:
            # Turn on current pin, turn off others
            for p in blink_pins:
                gpio_set(p, p == pin)
            
            oled_clear()
            oled_print("Running: GPIO" + str(pin))
            time.sleep(0.2)
    
    # Pattern 3: Binary counter
    print("\n☺ Pattern 3: Binary counter (0-15):")
    for count in range(16):
        binary_str = format(count, '04b')  # 4-bit binary
        
        print("  Count " + str(count) + " (binary: " + binary_str + "):")
        
        for i, pin in enumerate(blink_pins):
            bit_value = int(binary_str[i])
            gpio_set(pin, bit_value)
            state = gpio_get(pin)
            print("    GPIO" + str(pin) + ": " + str(state))
        
        oled_clear()
        oled_print("Count: " + str(count) + " (" + binary_str + ")")
        time.sleep(0.5)
    
    # Pattern 4: Alternating blink
    print("\n☺ Pattern 4: Alternating blink:")
    for cycle in range(10):
        # Even pins on, odd pins off
        for pin in blink_pins:
            gpio_set(pin, pin % 2 == 0)
        
        oled_clear()
        oled_print("Alt: Even ON")
        time.sleep(0.3)
        
        # Odd pins on, even pins off
        for pin in blink_pins:
            gpio_set(pin, pin % 2 == 1)
        
        oled_clear()
        oled_print("Alt: Odd ON")
        time.sleep(0.3)
    
    # Turn off all pins
    for pin in blink_pins:
        gpio_set(pin, False)
    
    print("✓ Blink patterns complete")

def gpio_speed_test():
    """Test GPIO switching speed"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            GPIO SPEED TEST                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("GPIO Speed Test")
    
    print("\n☺ Testing GPIO switching speed:")
    
    test_pin = 1
    gpio_set_dir(test_pin, True)  # Set as output
    
    # Test single pin toggle speed
    print("  Testing single pin toggle speed (1000 toggles):")
    
    num_toggles = 1000
    start_time = time.time()
    
    for i in range(num_toggles):
        gpio_set(test_pin, True)
        gpio_set(test_pin, False)
        
        if i % 100 == 0:
            oled_clear()
            oled_print("Toggle " + str(i + 1) + "/" + str(num_toggles))
    
    end_time = time.time()
    duration = end_time - start_time
    toggle_rate = num_toggles / duration
    
    print("    Toggles: " + str(num_toggles))
    print("    Duration: " + str(round(duration, 3)) + " seconds")
    print("    Toggle rate: " + str(round(toggle_rate, 1)) + " toggles/second")
    
    # Test multi-pin simultaneous switching
    print("\n  Testing multi-pin simultaneous switching:")
    
    test_pins = [1, 2, 3, 4]
    for pin in test_pins:
        gpio_set_dir(pin, True)
    
    num_switches = 500
    start_time = time.time()
    
    for i in range(num_switches):
        # Set all pins high
        for pin in test_pins:
            gpio_set(pin, True)
        
        # Set all pins low
        for pin in test_pins:
            gpio_set(pin, False)
        
        if i % 50 == 0:
            oled_clear()
            oled_print("Switch " + str(i + 1) + "/" + str(num_switches))
    
    end_time = time.time()
    duration = end_time - start_time
    switch_rate = num_switches / duration
    
    print("    Switches: " + str(num_switches))
    print("    Duration: " + str(round(duration, 3)) + " seconds")
    print("    Switch rate: " + str(round(switch_rate, 1)) + " switches/second")
    
    oled_clear()
    oled_print("Speed: " + str(round(toggle_rate, 0)) + " T/s")
    time.sleep(2)
    
    print("✓ Speed test complete")

def gpio_status_monitor():
    """Monitor GPIO status in real time"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           GPIO STATUS MONITOR                               │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("GPIO Status Monitor")
    
    print("\n☺ Current GPIO status for pins 1-8:")
    
    # Set up different configurations for demonstration
    configs = [
        (1, True, True),    # Output, High
        (2, True, False),   # Output, Low
        (3, False, 1),      # Input, Pull-up
        (4, False, 0),      # Input, No pull
        (5, False, -1),     # Input, Pull-down
        (6, True, True),    # Output, High
        (7, True, False),   # Output, Low
        (8, False, 1)       # Input, Pull-up
    ]
    
    print("  Setting up test configurations:")
    for pin, is_output, value_or_pull in configs:
        gpio_set_dir(pin, is_output)
        if is_output:
            gpio_set(pin, value_or_pull)
            print("    GPIO" + str(pin) + ": OUTPUT, " + str(value_or_pull))
        else:
            gpio_set_pull(pin, value_or_pull)
            pull_names = {1: "PULL_UP", 0: "NONE", -1: "PULL_DOWN"}
            print("    GPIO" + str(pin) + ": INPUT, " + pull_names[value_or_pull])
    
    print("\n☺ Monitoring GPIO status:")
    print("  Pin | Direction | State | Pull")
    print("  ----|-----------|-------|----------")
    
    # Monitor for several cycles
    for cycle in range(10):
        print("  Cycle " + str(cycle + 1) + ":")
        
        for pin in range(1, 9):
            try:
                direction = gpio_get_dir(pin)
                state = gpio_get(pin)
                pull = gpio_get_pull(pin)
                
                dir_str = "OUT" if direction == "OUTPUT" else "IN"
                state_str = str(state)
                pull_str = str(pull)
                
                print("   " + str(pin) + "  | " + dir_str + "       | " + state_str + "     | " + pull_str)
                
                # Show current pin on OLED
                oled_clear()
                oled_print("GPIO" + str(pin) + ": " + dir_str + " " + state_str)
                time.sleep(0.3)
                
            except Exception as e:
                print("   " + str(pin) + "  | ERROR: " + str(e))
        
        print("  ---")
        time.sleep(1)
    
    print("✓ Status monitoring complete")

def gpio_advanced_patterns():
    """Demonstrate advanced GPIO patterns"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         ADVANCED GPIO PATTERNS                              │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Advanced Patterns")
    
    pins = [1, 2, 3, 4, 5, 6]
    
    # Set all pins as outputs
    print("\n☺ Setting up GPIO pins 1-6 as outputs:")
    for pin in pins:
        gpio_set_dir(pin, True)
        direction = gpio_get_dir(pin)
        print("  GPIO" + str(pin) + " direction: " + str(direction))
    
    # Pattern 1: Sine wave approximation
    print("\n☺ Pattern 1: Sine wave approximation:")
    import math
    
    steps = 20
    cycles = 3
    
    for cycle in range(cycles):
        print("  Cycle " + str(cycle + 1) + ":")
        for step in range(steps):
            angle = (step / steps) * 2 * math.pi
            sine_value = math.sin(angle)
            
            # Convert sine value to pin pattern
            for i, pin in enumerate(pins):
                threshold = (i / len(pins)) * 2 - 1  # -1 to 1
                gpio_set(pin, sine_value > threshold)
            
            oled_clear()
            oled_print("Sine: " + str(round(sine_value, 2)))
            time.sleep(0.1)
    
    # Pattern 2: Shift register simulation
    print("\n☺ Pattern 2: Shift register simulation:")
    
    # Start with pattern 10000000
    pattern = [True] + [False] * (len(pins) - 1)
    
    for shift in range(20):
        # Set pins according to pattern
        for i, pin in enumerate(pins):
            gpio_set(pin, pattern[i])
        
        # Display pattern
        pattern_str = "".join(["1" if p else "0" for p in pattern])
        print("  Shift " + str(shift + 1) + ": " + pattern_str)
        
        oled_clear()
        oled_print("Shift: " + pattern_str)
        time.sleep(0.3)
        
        # Shift pattern (rotate right)
        pattern = [pattern[-1]] + pattern[:-1]
    
    # Pattern 3: PWM simulation
    print("\n☺ Pattern 3: PWM simulation:")
    
    pwm_pin = 1
    duty_cycles = [10, 25, 50, 75, 90]  # Duty cycle percentages
    
    for duty in duty_cycles:
        print("  PWM " + str(duty) + "% duty cycle:")
        
        for cycle in range(20):
            # Simple PWM: on for duty% of cycle, off for rest
            on_time = duty / 100.0
            
            # On period
            gpio_set(pwm_pin, True)
            oled_clear()
            oled_print("PWM " + str(duty) + "% ON")
            time.sleep(on_time * 0.1)
            
            # Off period
            gpio_set(pwm_pin, False)
            oled_clear()
            oled_print("PWM " + str(duty) + "% OFF")
            time.sleep((1.0 - on_time) * 0.1)
    
    # Turn off all pins
    for pin in pins:
        gpio_set(pin, False)
    
    print("✓ Advanced patterns complete")

def run_all_gpio_demos():
    """Run all GPIO demonstration functions"""
    
    print("🚀 Starting Complete GPIO Reference Demonstration")
    print("═" * 75)
    
    demos = [
        ("Basic Operations", gpio_basic_operations),
        ("Direction Control", gpio_direction_control),
        ("Pull Resistor Control", gpio_pull_resistor_control),
        ("Blink Patterns", gpio_blink_patterns),
        ("Speed Test", gpio_speed_test),
        ("Status Monitor", gpio_status_monitor),
        ("Advanced Patterns", gpio_advanced_patterns)
    ]
    
    for name, demo_func in demos:
        print("\n📍 Running: " + name)
        print("─" * 50)
        try:
            demo_func()
            print("✓ " + name + " completed successfully")
        except Exception as e:
            print("❌ " + name + " failed: " + str(e))
        
        time.sleep(2)
    
    oled_clear()
    oled_print("GPIO Reference Complete!")
    print("\n🎉 All GPIO demonstrations complete!")
    print("═" * 75)

def gpio_quick_test():
    """Quick test of GPIO functions"""
    
    print("⚡ Quick GPIO Test")
    print("─" * 20)
    
    # Test basic operations on GPIO1
    gpio_set_dir(1, True)
    gpio_set(1, True)
    state = gpio_get(1)
    print("GPIO1 set HIGH: " + str(state))
    
    gpio_set(1, False)
    state = gpio_get(1)
    print("GPIO1 set LOW: " + str(state))
    
    print("✓ Quick GPIO test complete")

# Run the demonstration
if __name__ == "__main__":
    run_all_gpio_demos() 