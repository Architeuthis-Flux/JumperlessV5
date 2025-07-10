"""
Simple Jumperless Demos
======================

Short practical examples combining different Jumperless functions.
These examples demonstrate real-world usage patterns.

Usage:
  exec(open('examples/simple_demos.py').read())
"""

import time

def led_brightness_control():
    """Control LED brightness using DAC and probe touch"""
    print("🔆 LED Brightness Control Demo")
    print("Touch breadboard pads 1-10 to control LED brightness")
    
    # Setup
    nodes_clear()
    oled_clear()
    oled_print("LED Brightness")
    
    # Connect DAC0 to breadboard for LED control
    connect(DAC0, 15)
    print("  Connect LED anode to hole 15")
    print("  Connect LED cathode to GND through resistor")
    print("  Press Enter when ready...")
    input()
    
    try:
        while True:
            # Read probe (non-blocking)
            pad = probe_read(False)
            
            if pad and pad != -1:
                try:
                    # Get pad number
                    if hasattr(pad, 'value'):
                        pad_num = pad.value
                    else:
                        pad_num = int(str(pad))
                    
                    # Map pad 1-10 to voltage 0-3.3V
                    if 1 <= pad_num <= 10:
                        voltage = (pad_num / 10.0) * 3.3
                        dac_set(DAC0, voltage)
                        
                        print("Pad " + str(pad_num) + ": " + str(voltage) + "V")
                        oled_clear()
                        oled_print("Bright: " + str(pad_num) + "/10")
                        
                except (ValueError, AttributeError):
                    pass
            
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\n✓ Demo stopped")
        dac_set(DAC0, 0)  # Turn off LED
        oled_clear()
        oled_print("Demo Done")

def voltage_monitor():
    """Monitor voltage on ADC with OLED display"""
    print("📊 Voltage Monitor Demo")
    print("Connect voltage source to ADC0 (pin on breadboard)")
    
    # Setup connections
    nodes_clear()
    connect(ADC0, 20)  # Connect ADC0 to breadboard hole 20
    print("  Connect voltage source to hole 20")
    print("  Monitoring voltage on ADC0...")
    
    oled_clear()
    oled_print("Voltage Monitor")
    time.sleep(1)
    
    try:
        while True:
            voltage = adc_get(0)
            
            # Display on OLED and console
            oled_clear()
            oled_print(str(voltage) + "V")
            print("Voltage: " + str(voltage) + "V")
            
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\n✓ Monitor stopped")
        oled_clear()
        oled_print("Monitor Done")

def gpio_button_led():
    """GPIO button input controls LED output"""
    print("🔘 GPIO Button + LED Demo")
    print("Uses GPIO1 as input (button) and GPIO2 as output (LED)")
    
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
    
    print("  Connect button between hole 25 and GND")
    print("  Connect LED from hole 30 to GND (with resistor)")
    print("  Press button to toggle LED...")
    
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
                
                print("Button pressed - LED " + ("ON" if led_state else "OFF"))
                oled_clear()
                oled_print("LED " + ("ON" if led_state else "OFF"))
                
            elif button_state == True:
                # Button released
                button_pressed = False
            
            time.sleep(0.05)  # Debounce delay
            
    except KeyboardInterrupt:
        print("\n✓ Demo stopped")
        gpio_set(2, False)  # Turn off LED
        oled_clear()
        oled_print("Demo Done")

def power_supply_demo():
    """Configure multi-voltage power supply"""
    print("⚡ Multi-Voltage Power Supply Demo")
    
    # Setup different voltages
    voltages = {
        "3.3V Rail": (TOP_RAIL, 3.3),
        "5V Rail": (BOTTOM_RAIL, 5.0),
        "1.8V": (DAC0, 1.8),
        "2.5V": (DAC1, 2.5)
    }
    
    nodes_clear()
    
    print("Setting up power rails:")
    for name, (rail, voltage) in voltages.items():
        dac_set(rail, voltage)
        actual = dac_get(rail)
        print("  " + name + ": " + str(voltage) + "V (actual: " + str(actual) + "V)")
    
    # Connect rails to breadboard for easy access
    connect(TOP_RAIL, 10)    # 3.3V to hole 10
    connect(BOTTOM_RAIL, 20) # 5V to hole 20
    connect(DAC0, 30)        # 1.8V to hole 30
    connect(DAC1, 40)        # 2.5V to hole 40
    connect(GND, 50)         # GND to hole 50
    
    print("\nPower rails connected to breadboard:")
    print("  Hole 10: 3.3V")
    print("  Hole 20: 5.0V") 
    print("  Hole 30: 1.8V")
    print("  Hole 40: 2.5V")
    print("  Hole 50: GND")
    
    oled_clear()
    oled_print("Power Ready")
    
    print("\nPress Enter to continue or Ctrl+C to keep this setup...")
    try:
        input()
    except KeyboardInterrupt:
        print("Power supply configuration kept active")
        return
    
    # Reset all to safe values
    for rail, _ in voltages.values():
        dac_set(rail, 0.0)
    print("✓ Power supply reset")

def current_monitor_demo():
    """Monitor current consumption with display"""
    print("⚡ Current Monitor Demo")
    print("Measures current through INA sensor")
    
    # Setup
    nodes_clear()
    oled_clear()
    oled_print("Current Monitor")
    
    # Connect current sensor to breadboard
    connect(ISENSE_PLUS, 35)
    connect(ISENSE_MINUS, 45)
    
    print("  Connect circuit through current sensor:")
    print("  Power source → Hole 35 → Your circuit → Hole 45 → GND")
    print("  Monitoring current...")
    
    max_current = 0.0
    readings = []
    
    try:
        while True:
            # Read current from sensor 0
            current = get_current(0)
            voltage = get_bus_voltage(0)
            power = get_power(0)
            
            # Track maximum
            if abs(current) > abs(max_current):
                max_current = current
            
            # Keep last 10 readings for average
            readings.append(current)
            if len(readings) > 10:
                readings.pop(0)
            avg_current = sum(readings) / len(readings)
            
            # Display current info
            print("Current: " + str(current*1000) + "mA, Voltage: " + str(voltage) + "V, Power: " + str(power*1000) + "mW")
            
            oled_clear()
            if abs(current) > 0.001:  # > 1mA
                oled_print(str(current*1000) + "mA")
            else:
                oled_print(str(current*1000000) + "µA")
            
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\n✓ Monitor stopped")
        print("Maximum current: " + str(max_current*1000) + "mA")
        print("Average current: " + str(avg_current*1000) + "mA")
        oled_clear()
        oled_print("Monitor Done")

def probe_connection_helper():
    """Interactive tool to help make connections using the probe"""
    print("🔌 Probe Connection Helper")
    print("Touch pads to automatically connect them")
    
    oled_clear()
    oled_print("Touch 2 pads")
    
    nodes_clear()
    connections = []
    first_pad = None
    
    print("  Touch first pad...")
    
    try:
        while True:
            # Wait for probe touch (blocking)
            pad = probe_read()
            
            if pad and pad != -1:
                try:
                    if hasattr(pad, 'value'):
                        pad_num = pad.value
                    else:
                        pad_num = int(str(pad))
                    
                    if first_pad is None:
                        first_pad = pad_num
                        print("  First pad: " + str(first_pad))
                        oled_clear()
                        oled_print("Pad " + str(first_pad))
                        print("  Touch second pad...")
                        
                    else:
                        second_pad = pad_num
                        
                        if first_pad != second_pad:
                            # Make connection
                            connect(first_pad, second_pad)
                            connections.append((first_pad, second_pad))
                            
                            print("  ✓ Connected " + str(first_pad) + " ↔ " + str(second_pad))
                            oled_clear()
                            oled_print(str(first_pad) + "↔" + str(second_pad))
                            
                            time.sleep(1)
                            
                            # Reset for next connection
                            first_pad = None
                            oled_clear()
                            oled_print("Touch 2 pads")
                            print("  Touch next pair of pads (or Ctrl+C to finish)...")
                        else:
                            print("  Same pad touched, try again...")
                            
                except (ValueError, AttributeError):    
                    print("  Invalid pad, try again...")
                    
    except KeyboardInterrupt:
        print("\n✓ Connection helper finished")
        print("Made " + str(len(connections)) + " connections:")
        for conn in connections:
            print("  " + str(conn[0]) + " ↔ " + str(conn[1]))
        
        oled_clear()
        oled_print(str(len(connections)) + " connections")

def all_demos():
    """Run all demos in sequence"""
    demos = [
        ("LED Brightness", led_brightness_control),
        ("Voltage Monitor", voltage_monitor),
        ("GPIO Button+LED", gpio_button_led),
        ("Power Supply", power_supply_demo),
        ("Current Monitor", current_monitor_demo),
        ("Probe Helper", probe_connection_helper)
    ]
    
    print("🎯 Simple Demos Collection")
    print("=" * 40)
    
    for i, (name, func) in enumerate(demos, 1):
        print(str(i) + ". " + name)
    
    print("\nRunning all demos in sequence...")
    print("Press Ctrl+C during any demo to move to the next one")
    
    for name, func in demos:
        print("\n" + "="*20 + " " + name + " " + "="*20)
        try:
            func()
        except KeyboardInterrupt:
            print("\n⏭  Skipping to next demo...")
            continue
        
        print("Press Enter for next demo...")
        try:
            input()
        except KeyboardInterrupt:
            break
    
    print("\n✅ All demos complete!")

# Quick access functions
def quick_led():
    """Quick LED brightness demo"""
    led_brightness_control()

def quick_monitor():
    """Quick voltage monitor"""
    voltage_monitor()

def quick_current():
    """Quick current monitor"""
    current_monitor_demo()

if __name__ == "__main__":
    print("Simple Jumperless Demos loaded!")
    print("\nAvailable demos:")
    print("  led_brightness_control() - Control LED with probe")
    print("  voltage_monitor()        - Monitor voltage on OLED")
    print("  gpio_button_led()        - Button controls LED")
    print("  power_supply_demo()      - Multi-voltage power supply")
    print("  current_monitor_demo()   - Current consumption monitor")
    print("  probe_connection_helper() - Interactive connection tool")
    print("  all_demos()              - Run all demos")
    print("\nQuick access:")
    print("  quick_led()    quick_monitor()    quick_current()")
else:
    print("Simple demos loaded. Try led_brightness_control() or all_demos()!") 