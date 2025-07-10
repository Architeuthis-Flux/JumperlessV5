"""
╭─────────────────────────────────────────────────────────────────╮
│                    Power Supply Demo                            │
╰─────────────────────────────────────────────────────────────────╯

Configure multi-voltage power supply using DAC outputs.
Creates multiple voltage rails for powering different circuits.

⚡ Voltage Rails:
- 3.3V Rail (TOP_RAIL) → Breadboard hole 10
- 5V Rail (BOTTOM_RAIL) → Breadboard hole 20
- 1.8V (DAC0) → Breadboard hole 30
- 2.5V (DAC1) → Breadboard hole 40
- GND → Breadboard hole 50

🔧 Features:
- Multiple precision voltage outputs
- Real-time voltage verification
- Safe startup and shutdown
- Configurable voltage levels

📋 Usage:
  exec(open('examples/micropython_examples/power_supply_demo.py').read())
  power_supply_demo()
"""

import time

def power_supply_demo():
    """Configure multi-voltage power supply"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                    ⚡ Multi-Voltage Power Supply Demo            │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    # Setup different voltages
    voltages = {
        "3.3V Rail": (TOP_RAIL, 3.3),
        "5V Rail": (BOTTOM_RAIL, 5.0),
        "1.8V": (DAC0, 1.8),
        "2.5V": (DAC1, 2.5)
    }
    
    nodes_clear()
    
    print("\n🔧 Setting up power rails:")
    oled_clear()
    oled_print("Power Setup")
    
    for name, (rail, voltage) in voltages.items():
        print("  Setting " + name + " to " + str(voltage) + "V...")
        dac_set(rail, voltage)
        time.sleep(0.5)  # Allow voltage to stabilize
        
        actual = dac_get(rail)
        print("  ✓ " + name + ": " + str(voltage) + "V (actual: " + str(round(actual, 3)) + "V)")
    
    # Connect rails to breadboard for easy access
    print("\n🔌 Connecting rails to breadboard:")
    connect(TOP_RAIL, 10)    # 3.3V to hole 10
    connect(BOTTOM_RAIL, 20) # 5V to hole 20
    connect(DAC0, 30)        # 1.8V to hole 30
    connect(DAC1, 40)        # 2.5V to hole 40
    connect(GND, 50)         # GND to hole 50
    
    print("  ✓ 3.3V → Hole 10")
    print("  ✓ 5.0V → Hole 20") 
    print("  ✓ 1.8V → Hole 30")
    print("  ✓ 2.5V → Hole 40")
    print("  ✓ GND  → Hole 50")
    
    oled_clear()
    oled_print("Power Ready")
    
    print("\n📋 Power rails connected to breadboard:")
    print("  Hole 10: 3.3V")
    print("  Hole 20: 5.0V") 
    print("  Hole 30: 1.8V")
    print("  Hole 40: 2.5V")
    print("  Hole 50: GND")
    
    print("\n⚡ Power supply is now active!")
    print("  Press Enter to continue or Ctrl+C to keep this setup...")
    
    try:
        input()
        
        # Reset all to safe values
        print("\n🔄 Shutting down power supply...")
        for rail, _ in voltages.values():
            dac_set(rail, 0.0)
        print("✓ Power supply reset to safe values")
        
    except KeyboardInterrupt:
        print("\n✓ Power supply configuration kept active")
        return
    
    oled_clear()
    oled_print("Power Off")

def custom_power_supply():
    """Create custom voltage configuration"""
    print("⚡ Custom Power Supply Configuration")
    
    # Get custom voltages from user
    custom_voltages = {}
    rails = ["TOP_RAIL", "BOTTOM_RAIL", "DAC0", "DAC1"]
    
    print("\n🔧 Configure custom voltages:")
    print("  Enter voltages for each rail (0V to disable)")
    print("  Valid range: -8.0V to +8.0V")
    
    for rail in rails:
        while True:
            try:
                voltage_str = input("  " + rail + " voltage: ")
                voltage = float(voltage_str)
                if -8.0 <= voltage <= 8.0:
                    custom_voltages[rail] = voltage
                    break
                else:
                    print("    Error: Voltage must be between -8.0V and +8.0V")
            except ValueError:
                print("    Error: Please enter a valid number")
            except KeyboardInterrupt:
                print("\n⚠️  Custom configuration cancelled")
                return
    
    # Apply custom configuration
    nodes_clear()
    
    print("\n⚡ Applying custom power configuration:")
    rail_objects = {
        "TOP_RAIL": TOP_RAIL,
        "BOTTOM_RAIL": BOTTOM_RAIL,
        "DAC0": DAC0,
        "DAC1": DAC1
    }
    
    for rail_name, voltage in custom_voltages.items():
        if voltage != 0:
            rail_obj = rail_objects[rail_name]
            dac_set(rail_obj, voltage)
            actual = dac_get(rail_obj)
            print("  ✓ " + rail_name + ": " + str(voltage) + "V (actual: " + str(round(actual, 3)) + "V)")
        else:
            print("  • " + rail_name + ": Disabled")
    
    # Connect active rails to breadboard
    hole_map = {"TOP_RAIL": 10, "BOTTOM_RAIL": 20, "DAC0": 30, "DAC1": 40}
    
    print("\n🔌 Connecting active rails:")
    for rail_name, voltage in custom_voltages.items():
        if voltage != 0:
            rail_obj = rail_objects[rail_name]
            hole = hole_map[rail_name]
            connect(rail_obj, hole)
            print("  ✓ " + rail_name + " (" + str(voltage) + "V) → Hole " + str(hole))
    
    connect(GND, 50)
    print("  ✓ GND → Hole 50")
    
    oled_clear()
    oled_print("Custom Power Ready")
    
    print("\n✅ Custom power supply active!")
    print("  Press Enter to shutdown or Ctrl+C to keep active...")
    
    try:
        input()
        
        # Shutdown
        print("\n🔄 Shutting down custom power supply...")
        for rail_name in custom_voltages:
            rail_obj = rail_objects[rail_name]
            dac_set(rail_obj, 0.0)
        print("✓ Custom power supply shutdown complete")
        
    except KeyboardInterrupt:
        print("\n✓ Custom power supply kept active")

def power_monitoring_demo():
    """Monitor power supply voltages continuously"""
    print("📊 Power Supply Monitoring Demo")
    
    # Setup standard voltages
    voltages = {
        "3.3V Rail": (TOP_RAIL, 3.3),
        "5V Rail": (BOTTOM_RAIL, 5.0),
        "1.8V": (DAC0, 1.8),
        "2.5V": (DAC1, 2.5)
    }
    
    nodes_clear()
    
    print("\n⚡ Starting power monitoring...")
    for name, (rail, voltage) in voltages.items():
        dac_set(rail, voltage)
    
    # Connect to breadboard
    connect(TOP_RAIL, 10)
    connect(BOTTOM_RAIL, 20)
    connect(DAC0, 30)
    connect(DAC1, 40)
    connect(GND, 50)
    
    oled_clear()
    oled_print("Power Monitor")
    
    try:
        while True:
            print("\n📊 Power Supply Status:")
            
            for name, (rail, expected) in voltages.items():
                actual = dac_get(rail)
                error = abs(actual - expected)
                status = "✓" if error < 0.1 else "⚠️"
                
                print("  " + status + " " + name + ": " + str(round(actual, 3)) + "V " +
                      "(expected: " + str(expected) + "V, error: " + str(round(error, 3)) + "V)")
            
            # Display rotating status on OLED
            rail_names = list(voltages.keys())
            current_rail = rail_names[int(time.time()) % len(rail_names)]
            rail_obj, expected = voltages[current_rail]
            actual = dac_get(rail_obj)
            
            oled_clear()
            oled_print(current_rail.split()[0] + ": " + str(round(actual, 2)) + "V")
            
            time.sleep(2)
            
    except KeyboardInterrupt:
        print("\n✓ Power monitoring stopped")
        
        # Shutdown
        print("🔄 Shutting down power supply...")
        for rail, _ in voltages.values():
            dac_set(rail, 0.0)
        
        oled_clear()
        oled_print("Power Off")

def safe_power_test():
    """Test power supply with safety checks"""
    print("🛡️  Safe Power Supply Test")
    
    # Test voltages in safe progression
    test_sequence = [
        (TOP_RAIL, [0.0, 1.0, 2.0, 3.0, 3.3, 0.0]),
        (BOTTOM_RAIL, [0.0, 1.0, 3.0, 5.0, 0.0]),
        (DAC0, [0.0, 0.5, 1.0, 1.5, 1.8, 0.0]),
        (DAC1, [0.0, 0.5, 1.0, 2.0, 2.5, 0.0])
    ]
    
    nodes_clear()
    
    for rail, voltage_steps in test_sequence:
        rail_name = "TOP_RAIL" if rail == TOP_RAIL else "BOTTOM_RAIL" if rail == BOTTOM_RAIL else "DAC0" if rail == DAC0 else "DAC1"
        
        print("\n🔧 Testing " + rail_name + ":")
        
        for voltage in voltage_steps:
            print("  Setting " + str(voltage) + "V...")
            dac_set(rail, voltage)
            time.sleep(0.5)  # Allow stabilization
            
            actual = dac_get(rail)
            print("    Actual: " + str(round(actual, 3)) + "V")
            
            oled_clear()
            oled_print(rail_name + ": " + str(voltage) + "V")
            
            time.sleep(1)
    
    print("\n✅ Safe power test complete")
    oled_clear()
    oled_print("Test Complete")

def run_all_power_demos():
    """Run all power supply demos"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                  ⚡ All Power Supply Demos                      │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    demos = [
        ("Safe Power Test", safe_power_test),
        ("Power Monitoring", power_monitoring_demo),
        ("Custom Power Supply", custom_power_supply),
        ("Standard Power Supply", power_supply_demo)
    ]
    
    for name, demo_func in demos:
        print("\n🎯 Running: " + name)
        print("─" * 40)
        try:
            demo_func()
        except KeyboardInterrupt:
            print("⏭️  Skipping to next demo...")
            continue
        
        if name != "Standard Power Supply":  # Don't pause after last demo
            print("📋 Press Enter for next demo...")
            try:
                input()
            except KeyboardInterrupt:
                break
    
    print("\n✅ All power supply demos complete!")

if __name__ == "__main__":
    print("Power Supply Demo loaded!")
    print("\nAvailable functions:")
    print("  power_supply_demo()     - Standard multi-voltage power supply")
    print("  custom_power_supply()   - Configure custom voltages")
    print("  power_monitoring_demo() - Monitor power supply voltages")
    print("  safe_power_test()       - Test power supply safely")
    print("  run_all_power_demos()   - Run all power supply demos")
else:
    print("Power supply demo loaded. Try power_supply_demo()!") 