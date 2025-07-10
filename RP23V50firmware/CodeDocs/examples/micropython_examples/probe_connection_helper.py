"""
╭─────────────────────────────────────────────────────────────────╮
│                    Probe Connection Helper                      │
╰─────────────────────────────────────────────────────────────────╯

Interactive tool to help make connections using the probe.
Touch two breadboard pads to automatically connect them.

🔧 How It Works:
1. Touch first breadboard pad with probe
2. Touch second breadboard pad with probe
3. Connection is automatically created
4. Repeat for additional connections

🎮 Controls:
- Touch two different pads to connect them
- Touch same pad twice to ignore
- Ctrl+C to exit and show connection summary

📋 Usage:
  exec(open('examples/micropython_examples/probe_connection_helper.py').read())
  probe_connection_helper()
"""

import time

def probe_connection_helper():
    """Interactive tool to help make connections using the probe"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                    🔌 Probe Connection Helper                   │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    print("\n🎮 Touch pads to automatically connect them")
    
    oled_clear()
    oled_print("Touch 2 pads")
    
    nodes_clear()
    connections = []
    first_pad = None
    
    print("  • Touch first pad...")
    
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
                        print("  ✓ First pad: " + str(first_pad))
                        oled_clear()
                        oled_print("Pad " + str(first_pad))
                        print("  • Touch second pad...")
                        
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
                            print("  • Touch next pair of pads (or Ctrl+C to finish)...")
                        else:
                            print("  ⚠️  Same pad touched, try again...")
                            
                except (ValueError, AttributeError):
                    print("  ⚠️  Invalid pad, try again...")
                    
    except KeyboardInterrupt:
        print("\n✓ Connection helper finished")
        print("📊 Summary:")
        print("  • Made " + str(len(connections)) + " connections:")
        for i, conn in enumerate(connections):
            print("    " + str(i+1) + ". " + str(conn[0]) + " ↔ " + str(conn[1]))
        
        oled_clear()
        oled_print(str(len(connections)) + " connections")

def advanced_probe_helper():
    """Advanced probe helper with connection verification"""
    print("🔌 Advanced Probe Connection Helper")
    
    print("\n🎮 Advanced features:")
    print("  • Connection verification")
    print("  • Connection status tracking")
    print("  • Undo last connection")
    
    oled_clear()
    oled_print("Advanced Helper")
    
    nodes_clear()
    connections = []
    first_pad = None
    
    print("  • Touch first pad...")
    
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
                        print("  ✓ First pad: " + str(first_pad))
                        oled_clear()
                        oled_print("1st: " + str(first_pad))
                        print("  • Touch second pad...")
                        
                    else:
                        second_pad = pad_num
                        
                        if first_pad != second_pad:
                            # Check if connection already exists
                            if is_connected(first_pad, second_pad):
                                print("  ⚠️  Connection already exists: " + str(first_pad) + " ↔ " + str(second_pad))
                                oled_clear()
                                oled_print("Already connected")
                            else:
                                # Make new connection
                                connect(first_pad, second_pad)
                                connections.append((first_pad, second_pad))
                                
                                # Verify connection
                                if is_connected(first_pad, second_pad):
                                    print("  ✅ Connected " + str(first_pad) + " ↔ " + str(second_pad) + " (verified)")
                                    oled_clear()
                                    oled_print("✅ " + str(first_pad) + "↔" + str(second_pad))
                                else:
                                    print("  ❌ Connection failed: " + str(first_pad) + " ↔ " + str(second_pad))
                                    oled_clear()
                                    oled_print("❌ Failed")
                            
                            time.sleep(1.5)
                            
                            # Reset for next connection
                            first_pad = None
                            oled_clear()
                            oled_print("Touch 2 pads")
                            print("  • Touch next pair (Ctrl+C to finish)...")
                        else:
                            print("  ⚠️  Same pad touched, try again...")
                            
                except (ValueError, AttributeError):
                    print("  ⚠️  Invalid pad, try again...")
                    
    except KeyboardInterrupt:
        print("\n✓ Advanced helper finished")
        print("📊 Connection Summary:")
        print("  • Total connections made: " + str(len(connections)))
        
        # Verify all connections
        verified = 0
        for first, second in connections:
            if is_connected(first, second):
                verified += 1
                print("    ✅ " + str(first) + " ↔ " + str(second))
            else:
                print("    ❌ " + str(first) + " ↔ " + str(second) + " (broken)")
        
        print("  • Verified connections: " + str(verified) + "/" + str(len(connections)))
        
        oled_clear()
        oled_print("✅ " + str(verified) + "/" + str(len(connections)))

def guided_circuit_builder():
    """Guided circuit building with probe"""
    print("🔧 Guided Circuit Builder")
    
    print("\n📋 Available circuit templates:")
    print("  1. Simple LED circuit")
    print("  2. Voltage divider")
    print("  3. Current sensor test")
    print("  4. Custom connections")
    
    try:
        choice = input("Select circuit (1-4): ")
        choice_num = int(choice)
    except (ValueError, KeyboardInterrupt):
        print("Invalid choice")
        return
    
    if choice_num == 1:
        build_led_circuit()
    elif choice_num == 2:
        build_voltage_divider()
    elif choice_num == 3:
        build_current_sensor_test()
    elif choice_num == 4:
        build_custom_circuit()
    else:
        print("Invalid choice")

def build_led_circuit():
    """Build a simple LED circuit with probe guidance"""
    print("💡 Building LED Circuit")
    
    nodes_clear()
    oled_clear()
    oled_print("LED Circuit")
    
    # Setup DAC for LED control
    dac_set(DAC0, 3.3)
    print("  ✓ DAC0 set to 3.3V")
    
    # Guided connections
    connections = [
        (DAC0, None, "Touch pad for LED positive (anode)"),
        (GND, None, "Touch pad for LED negative (cathode)")
    ]
    
    made_connections = []
    
    for source, target, instruction in connections:
        print("  📋 " + instruction)
        oled_clear()
        oled_print("Touch pad")
        
        try:
            pad = probe_read()
            if pad and pad != -1:
                pad_num = int(str(pad))
                connect(source, pad_num)
                made_connections.append((source, pad_num))
                
                print("  ✓ Connected " + str(source) + " to pad " + str(pad_num))
                oled_clear()
                oled_print("Connected!")
                time.sleep(1)
        except (ValueError, AttributeError, KeyboardInterrupt):
            print("  ⚠️  Connection skipped")
    
    print("\n💡 LED Circuit Complete!")
    print("  • Connect LED between the touched pads")
    print("  • Include a current-limiting resistor (220Ω-1kΩ)")
    
    oled_clear()
    oled_print("Circuit Ready")

def build_voltage_divider():
    """Build a voltage divider circuit with probe guidance"""
    print("📊 Building Voltage Divider")
    
    nodes_clear()
    oled_clear()
    oled_print("Voltage Divider")
    
    # Setup voltages
    dac_set(TOP_RAIL, 5.0)
    print("  ✓ TOP_RAIL set to 5.0V")
    
    connections = [
        (TOP_RAIL, None, "Touch pad for voltage input (+5V)"),
        (GND, None, "Touch pad for ground connection"),
        (ADC0, None, "Touch pad for voltage measurement")
    ]
    
    made_connections = []
    
    for source, target, instruction in connections:
        print("  📋 " + instruction)
        oled_clear()
        oled_print("Touch pad")
        
        try:
            pad = probe_read()
            if pad and pad != -1:
                pad_num = int(str(pad))
                connect(source, pad_num)
                made_connections.append((source, pad_num))
                
                print("  ✓ Connected " + str(source) + " to pad " + str(pad_num))
                oled_clear()
                oled_print("Connected!")
                time.sleep(1)
        except (ValueError, AttributeError, KeyboardInterrupt):
            print("  ⚠️  Connection skipped")
    
    print("\n📊 Voltage Divider Circuit Complete!")
    print("  • Connect resistors between the pads to create voltage divider")
    print("  • Measure output voltage with ADC")
    
    oled_clear()
    oled_print("Circuit Ready")

def build_current_sensor_test():
    """Build a current sensor test circuit"""
    print("⚡ Building Current Sensor Test")
    
    nodes_clear()
    oled_clear()
    oled_print("Current Sensor")
    
    # Setup current sensor
    connections = [
        (ISENSE_PLUS, None, "Touch pad for current sensor input"),
        (ISENSE_MINUS, None, "Touch pad for current sensor output"),
        (TOP_RAIL, None, "Touch pad for power source"),
        (GND, None, "Touch pad for ground")
    ]
    
    made_connections = []
    
    for source, target, instruction in connections:
        print("  📋 " + instruction)
        oled_clear()
        oled_print("Touch pad")
        
        try:
            pad = probe_read()
            if pad and pad != -1:
                pad_num = int(str(pad))
                connect(source, pad_num)
                made_connections.append((source, pad_num))
                
                print("  ✓ Connected " + str(source) + " to pad " + str(pad_num))
                oled_clear()
                oled_print("Connected!")
                time.sleep(1)
        except (ValueError, AttributeError, KeyboardInterrupt):
            print("  ⚠️  Connection skipped")
    
    print("\n⚡ Current Sensor Test Complete!")
    print("  • Connect your test load through the current sensor")
    print("  • Power flows: Source → Sensor In → Load → Sensor Out → Ground")
    
    oled_clear()
    oled_print("Sensor Ready")

def build_custom_circuit():
    """Build custom circuit with probe helper"""
    print("🔧 Custom Circuit Builder")
    
    nodes_clear()
    oled_clear()
    oled_print("Custom Circuit")
    
    print("  • Build your own circuit using the probe")
    print("  • Touch pads to make connections")
    print("  • Ctrl+C when finished")
    
    probe_connection_helper()

def quick_connect():
    """Quick two-pad connection"""
    print("⚡ Quick Connect")
    
    print("  Touch first pad...")
    oled_clear()
    oled_print("Touch 1st pad")
    
    try:
        pad1 = probe_read()
        if pad1 and pad1 != -1:
            pad1_num = int(str(pad1))
            print("  ✓ First pad: " + str(pad1_num))
            
            print("  Touch second pad...")
            oled_clear()
            oled_print("Touch 2nd pad")
            
            pad2 = probe_read()
            if pad2 and pad2 != -1:
                pad2_num = int(str(pad2))
                print("  ✓ Second pad: " + str(pad2_num))
                
                if pad1_num != pad2_num:
                    connect(pad1_num, pad2_num)
                    print("  ✅ Connected " + str(pad1_num) + " ↔ " + str(pad2_num))
                    oled_clear()
                    oled_print("✅ Connected")
                else:
                    print("  ⚠️  Same pad touched")
                    oled_clear()
                    oled_print("Same pad")
            else:
                print("  ⚠️  No second pad detected")
        else:
            print("  ⚠️  No first pad detected")
            
    except (ValueError, AttributeError, KeyboardInterrupt):
        print("  ⚠️  Quick connect cancelled")

def run_all_probe_demos():
    """Run all probe connection demos"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                  🔌 All Probe Connection Demos                  │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    demos = [
        ("Quick Connect", quick_connect),
        ("Guided Circuit Builder", guided_circuit_builder),
        ("Advanced Probe Helper", advanced_probe_helper),
        ("Basic Connection Helper", probe_connection_helper)
    ]
    
    for name, demo_func in demos:
        print("\n🎯 Running: " + name)
        print("─" * 40)
        try:
            demo_func()
        except KeyboardInterrupt:
            print("⏭️  Skipping to next demo...")
            continue
        
        if name != "Basic Connection Helper":  # Don't pause after last demo
            print("📋 Press Enter for next demo...")
            try:
                input()
            except KeyboardInterrupt:
                break
    
    print("\n✅ All probe connection demos complete!")

if __name__ == "__main__":
    print("Probe Connection Helper Demo loaded!")
    print("\nAvailable functions:")
    print("  probe_connection_helper() - Basic connection helper")
    print("  advanced_probe_helper()   - Advanced helper with verification")
    print("  guided_circuit_builder()  - Guided circuit building")
    print("  quick_connect()           - Quick two-pad connection")
    print("  run_all_probe_demos()     - Run all probe demos")
else:
    print("Probe connection helper loaded. Try probe_connection_helper()!") 