"""
╭─────────────────────────────────────────────────────────────────╮
│                    GPIO Button + LED Demo                       │
╰─────────────────────────────────────────────────────────────────╯

GPIO button input controls LED output demonstration.
Uses GPIO1 as input (button) and GPIO2 as output (LED).

🔧 Hardware Setup:
1. Connect button between breadboard hole 25 and GND
2. Connect LED from breadboard hole 30 to GND (with current-limiting resistor)
3. GPIO1 configured as input with pull-up resistor
4. GPIO2 configured as output for LED control

🎮 Controls:
- Press button to toggle LED on/off
- Button uses pull-up resistor (pressed = LOW, released = HIGH)
- LED state displayed on OLED and console

📋 Usage:
  exec(open('examples/micropython_examples/gpio_button_led.py').read())
  gpio_button_led()
"""

import time

def gpio_button_led():
    """GPIO button input controls LED output"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                    🔘 GPIO Button + LED Demo                    │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    print("\n🔧 Uses GPIO1 as input (button) and GPIO2 as output (LED)")
    
    # Setup
    nodes_clear()
    
    # Configure GPIO1 as input with pullup for button
    gpio_set_dir(1, False)  # Input
    gpio_set_pull(1, 1)     # Pull-up resistor
    
    # Configure GPIO2 as output for LED
    gpio_set_dir(2, True)   # Output
    gpio_set(2, False)      # Start off
    
    # Connect to breadboard
    connect(GPIO_1, 25)  # Button input
    connect(GPIO_2, 30)  # LED output
    
    print("  ✓ GPIO1 configured as input with pull-up")
    print("  ✓ GPIO2 configured as output")
    print("  ✓ GPIO1 connected to hole 25 (button)")
    print("  ✓ GPIO2 connected to hole 30 (LED)")
    
    print("\n📋 Hardware Setup:")
    print("  • Connect button between hole 25 and GND")
    print("  • Connect LED from hole 30 to GND (with resistor)")
    print("  • Press button to toggle LED...")
    
    oled_clear()
    oled_print("Button Ready")
    
    button_pressed = False
    led_state = False
    
    try:
        while True:
            # Read button state
            button_state = gpio_get(1)
            
            # Detect button press (low when pressed due to pullup)
            if button_state == False and not button_pressed:
                # Button just pressed
                button_pressed = True
                led_state = not led_state
                gpio_set(2, led_state)
                
                state_text = "ON" if led_state else "OFF"
                print("🔘 Button pressed - LED " + state_text)
                oled_clear()
                oled_print("LED " + state_text)
                
            elif button_state == True:
                # Button released
                button_pressed = False
            
            time.sleep(0.05)  # Debounce delay
            
    except KeyboardInterrupt:
        print("\n✓ Demo stopped")
        gpio_set(2, False)  # Turn off LED
        oled_clear()
        oled_print("Demo Done")

def gpio_multi_button_demo():
    """Multiple buttons controlling different LEDs"""
    print("🔘 Multi-Button LED Demo")
    
    # Setup
    nodes_clear()
    
    # Configure multiple GPIO pins
    buttons = [1, 2, 3]  # GPIO1, GPIO2, GPIO3 as buttons
    leds = [4, 5, 6]     # GPIO4, GPIO5, GPIO6 as LEDs
    
    for btn in buttons:
        gpio_set_dir(btn, False)  # Input
        gpio_set_pull(btn, 1)     # Pull-up
        
    for led in leds:
        gpio_set_dir(led, True)   # Output
        gpio_set(led, False)      # Start off
    
    # Connect to breadboard
    connect(GPIO_1, 25)  # Button 1
    connect(GPIO_2, 26)  # Button 2
    connect(GPIO_3, 27)  # Button 3
    connect(GPIO_4, 35)  # LED 1
    connect(GPIO_5, 36)  # LED 2
    connect(GPIO_6, 37)  # LED 3
    
    print("  ✓ 3 buttons connected to holes 25-27")
    print("  ✓ 3 LEDs connected to holes 35-37")
    print("  • Press buttons to toggle corresponding LEDs")
    
    oled_clear()
    oled_print("Multi-Button Ready")
    
    button_states = [False, False, False]
    led_states = [False, False, False]
    
    try:
        while True:
            for i, (btn, led) in enumerate(zip(buttons, leds)):
                # Read button state
                current_state = gpio_get(btn)
                
                # Detect button press
                if current_state == False and not button_states[i]:
                    # Button just pressed
                    button_states[i] = True
                    led_states[i] = not led_states[i]
                    gpio_set(led, led_states[i])
                    
                    state_text = "ON" if led_states[i] else "OFF"
                    print("🔘 Button " + str(i+1) + " pressed - LED " + str(i+1) + " " + state_text)
                    
                    # Update OLED
                    oled_clear()
                    status = ""
                    for j, state in enumerate(led_states):
                        status += "L" + str(j+1) + ("1" if state else "0") + " "
                    oled_print(status)
                    
                elif current_state == True:
                    # Button released
                    button_states[i] = False
            
            time.sleep(0.05)  # Debounce delay
            
    except KeyboardInterrupt:
        print("\n✓ Multi-button demo stopped")
        # Turn off all LEDs
        for led in leds:
            gpio_set(led, False)
        oled_clear()
        oled_print("Demo Done")

def gpio_pattern_demo():
    """LED pattern controlled by single button"""
    print("🌟 GPIO Pattern Demo")
    
    # Setup
    nodes_clear()
    
    # Configure GPIO
    gpio_set_dir(1, False)  # Button input
    gpio_set_pull(1, 1)     # Pull-up
    
    leds = [2, 3, 4, 5]  # 4 LEDs
    for led in leds:
        gpio_set_dir(led, True)   # Output
        gpio_set(led, False)      # Start off
    
    # Connect to breadboard
    connect(GPIO_1, 25)  # Button
    connect(GPIO_2, 32)  # LED 1
    connect(GPIO_3, 33)  # LED 2
    connect(GPIO_4, 34)  # LED 3
    connect(GPIO_5, 35)  # LED 4
    
    print("  ✓ Button connected to hole 25")
    print("  ✓ 4 LEDs connected to holes 32-35")
    print("  • Press button to cycle through patterns")
    
    patterns = [
        ([True, False, False, False], "Pattern 1"),
        ([False, True, False, False], "Pattern 2"),
        ([False, False, True, False], "Pattern 3"),
        ([False, False, False, True], "Pattern 4"),
        ([True, True, False, False], "Pattern 5"),
        ([False, False, True, True], "Pattern 6"),
        ([True, False, True, False], "Pattern 7"),
        ([False, True, False, True], "Pattern 8"),
        ([True, True, True, True], "All ON"),
        ([False, False, False, False], "All OFF")
    ]
    
    pattern_index = 0
    button_pressed = False
    
    oled_clear()
    oled_print("Pattern Demo")
    
    try:
        while True:
            # Read button state
            button_state = gpio_get(1)
            
            # Detect button press
            if button_state == False and not button_pressed:
                # Button just pressed - advance pattern
                button_pressed = True
                pattern_index = (pattern_index + 1) % len(patterns)
                
                pattern, name = patterns[pattern_index]
                
                # Set LEDs according to pattern
                for i, (led, state) in enumerate(zip(leds, pattern)):
                    gpio_set(led, state)
                
                print("🌟 " + name + ": " + str(pattern))
                oled_clear()
                oled_print(name)
                
            elif button_state == True:
                # Button released
                button_pressed = False
            
            time.sleep(0.05)  # Debounce delay
            
    except KeyboardInterrupt:
        print("\n✓ Pattern demo stopped")
        # Turn off all LEDs
        for led in leds:
            gpio_set(led, False)
        oled_clear()
        oled_print("Demo Done")

def gpio_status_monitor():
    """Monitor GPIO pin states"""
    print("📊 GPIO Status Monitor")
    
    # Setup
    nodes_clear()
    
    # Configure some pins as inputs and outputs
    inputs = [1, 2, 3]
    outputs = [4, 5, 6]
    
    for pin in inputs:
        gpio_set_dir(pin, False)  # Input
        gpio_set_pull(pin, 1)     # Pull-up
        
    for pin in outputs:
        gpio_set_dir(pin, True)   # Output
        gpio_set(pin, False)      # Start off
    
    # Connect to breadboard
    for i, pin in enumerate(inputs):
        connect(pin, 25 + i)  # Holes 25-27
    for i, pin in enumerate(outputs):
        connect(pin, 35 + i)  # Holes 35-37
    
    print("  ✓ Input pins 1-3 connected to holes 25-27")
    print("  ✓ Output pins 4-6 connected to holes 35-37")
    print("  • Monitoring pin states...")
    
    oled_clear()
    oled_print("GPIO Monitor")
    
    try:
        while True:
            # Read all pin states
            input_states = []
            for pin in inputs:
                state = gpio_get(pin)
                input_states.append(state)
            
            output_states = []
            for pin in outputs:
                state = gpio_get(pin)
                output_states.append(state)
            
            # Display status
            print("📊 GPIO Status:")
            print("  Inputs:  " + str(input_states))
            print("  Outputs: " + str(output_states))
            
            # Update OLED
            oled_clear()
            status = "I:" + "".join(["1" if s else "0" for s in input_states])
            oled_print(status)
            
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\n✓ GPIO monitor stopped")
        oled_clear()
        oled_print("Monitor Done")

def run_all_gpio_demos():
    """Run all GPIO button and LED demos"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                  🔘 All GPIO Button + LED Demos                 │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    demos = [
        ("GPIO Status Monitor", gpio_status_monitor),
        ("Pattern Demo", gpio_pattern_demo),
        ("Multi-Button Demo", gpio_multi_button_demo),
        ("Basic Button + LED", gpio_button_led)
    ]
    
    for name, demo_func in demos:
        print("\n🎯 Running: " + name)
        print("─" * 40)
        try:
            demo_func()
        except KeyboardInterrupt:
            print("⏭️  Skipping to next demo...")
            continue
        
        if name != "Basic Button + LED":  # Don't pause after last demo
            print("📋 Press Enter for next demo...")
            try:
                input()
            except KeyboardInterrupt:
                break
    
    print("\n✅ All GPIO demos complete!")

if __name__ == "__main__":
    print("GPIO Button + LED Demo loaded!")
    print("\nAvailable functions:")
    print("  gpio_button_led()      - Basic button controls LED")
    print("  gpio_multi_button_demo() - Multiple buttons and LEDs")
    print("  gpio_pattern_demo()    - Button cycles through LED patterns")
    print("  gpio_status_monitor()  - Monitor GPIO pin states")
    print("  run_all_gpio_demos()   - Run all GPIO demos")
else:
    print("GPIO button + LED loaded. Try gpio_button_led()!") 