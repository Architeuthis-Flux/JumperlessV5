"""
╭─────────────────────────────────────────────────────────────────╮
│                    Current Monitor Demo                         │
╰─────────────────────────────────────────────────────────────────╯

Monitor current consumption with INA current/power sensors.
Measures current, voltage, and power through the circuit.

🔧 Hardware Setup:
1. Connect circuit through current sensor:
   • Power source → Breadboard hole 35 (ISENSE_PLUS)
   • Your circuit → Breadboard hole 45 (ISENSE_MINUS)
   • Circuit return → GND

📊 Features:
- Real-time current monitoring (mA and µA)
- Bus voltage measurement
- Power calculation
- Statistics tracking (min, max, average)
- Threshold monitoring with alerts

📋 Usage:
  exec(open('examples/micropython_examples/current_monitor_demo.py').read())
  current_monitor_demo()
"""

import time

def current_monitor_demo():
    """Monitor current consumption with display"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                    ⚡ Current Monitor Demo                      │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    print("\n📊 Measures current through INA sensor")
    
    # Setup
    nodes_clear()
    oled_clear()
    oled_print("Current Monitor")
    
    # Connect current sensor to breadboard
    connect(ISENSE_PLUS, 35)
    connect(ISENSE_MINUS, 45)
    
    print("  ✓ ISENSE_PLUS connected to hole 35")
    print("  ✓ ISENSE_MINUS connected to hole 45")
    
    print("\n📋 Hardware Setup:")
    print("  • Connect circuit through current sensor:")
    print("    Power source → Hole 35 → Your circuit → Hole 45 → GND")
    print("  • Monitoring current flow...")
    
    max_current = 0.0
    min_current = float('inf')
    readings = []
    
    try:
        while True:
            # Read current from sensor 0
            current = get_current(0)
            voltage = get_bus_voltage(0)
            power = get_power(0)
            
            # Update statistics
            readings.append(current)
            if len(readings) > 100:  # Keep last 100 readings
                readings.pop(0)
            
            if abs(current) > abs(max_current):
                max_current = current
            if abs(current) < abs(min_current):
                min_current = current
            
            avg_current = sum(readings) / len(readings)
            
            # Display current info
            if abs(current) > 0.001:  # > 1mA
                current_str = str(round(current * 1000, 3)) + "mA"
                print("📊 Current: " + current_str + ", Voltage: " + str(round(voltage, 3)) + "V, Power: " + str(round(power * 1000, 3)) + "mW")
            else:
                current_str = str(round(current * 1000000, 0)) + "µA"
                print("📊 Current: " + current_str + ", Voltage: " + str(round(voltage, 3)) + "V, Power: " + str(round(power * 1000000, 1)) + "µW")
            
            # Update OLED
            oled_clear()
            if abs(current) > 0.001:  # > 1mA
                oled_print(str(round(current * 1000, 1)) + "mA")
            else:
                oled_print(str(round(current * 1000000, 0)) + "µA")
            
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\n✓ Monitor stopped")
        print("📈 Final Statistics:")
        print("  • Total readings: " + str(len(readings)))
        print("  • Maximum current: " + str(round(max_current * 1000, 3)) + "mA")
        print("  • Minimum current: " + str(round(min_current * 1000, 3)) + "mA")
        print("  • Average current: " + str(round(avg_current * 1000, 3)) + "mA")
        
        oled_clear()
        oled_print("Monitor Done")

def dual_sensor_monitor():
    """Monitor both current sensors simultaneously"""
    print("📊 Dual Current Sensor Monitor")
    
    # Setup
    nodes_clear()
    
    # Connect both sensors to breadboard
    connect(ISENSE_PLUS, 35)   # Sensor 0 positive
    connect(ISENSE_MINUS, 45)  # Sensor 0 negative
    
    print("  ✓ Sensor 0: ISENSE_PLUS → hole 35, ISENSE_MINUS → hole 45")
    print("  ✓ Sensor 1: Built-in sensor")
    print("  • Monitoring both current sensors...")
    
    oled_clear()
    oled_print("Dual Monitor")
    
    try:
        while True:
            # Read both sensors
            current0 = get_current(0)
            voltage0 = get_bus_voltage(0)
            power0 = get_power(0)
            
            current1 = get_current(1)
            voltage1 = get_bus_voltage(1)
            power1 = get_power(1)
            
            # Display sensor readings
            print("📊 Sensor 0: " + str(round(current0 * 1000, 3)) + "mA, " + 
                  str(round(voltage0, 3)) + "V, " + str(round(power0 * 1000, 3)) + "mW")
            print("📊 Sensor 1: " + str(round(current1 * 1000, 3)) + "mA, " + 
                  str(round(voltage1, 3)) + "V, " + str(round(power1 * 1000, 3)) + "mW")
            
            # Display on OLED (alternating sensors)
            sensor_num = int(time.time()) % 2
            if sensor_num == 0:
                oled_clear()
                oled_print("S0:" + str(round(current0 * 1000, 1)) + "mA")
            else:
                oled_clear()
                oled_print("S1:" + str(round(current1 * 1000, 1)) + "mA")
            
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\n✓ Dual sensor monitor stopped")
        oled_clear()
        oled_print("Monitor Done")

def current_threshold_monitor():
    """Monitor current with threshold alerts"""
    print("⚠️  Current Threshold Monitor")
    
    # Configuration
    LOW_THRESHOLD = 0.001    # 1mA
    HIGH_THRESHOLD = 0.050   # 50mA
    
    # Setup
    nodes_clear()
    connect(ISENSE_PLUS, 35)
    connect(ISENSE_MINUS, 45)
    
    print("  ✓ Current sensor connected")
    print("  ⚠️  Low threshold: " + str(round(LOW_THRESHOLD * 1000, 1)) + "mA")
    print("  ⚠️  High threshold: " + str(round(HIGH_THRESHOLD * 1000, 1)) + "mA")
    
    oled_clear()
    oled_print("Threshold Monitor")
    
    alert_count = 0
    
    try:
        while True:
            current = get_current(0)
            current_abs = abs(current)
            
            # Check thresholds
            if current_abs < LOW_THRESHOLD:
                print("🔻 LOW CURRENT: " + str(round(current * 1000, 3)) + "mA")
                oled_clear()
                oled_print("LOW: " + str(round(current * 1000, 1)) + "mA")
                
            elif current_abs > HIGH_THRESHOLD:
                alert_count += 1
                print("🔺 HIGH CURRENT: " + str(round(current * 1000, 3)) + "mA")
                oled_clear()
                oled_print("HIGH: " + str(round(current * 1000, 1)) + "mA")
                
            else:
                # Normal reading
                print("✅ Normal: " + str(round(current * 1000, 3)) + "mA")
                oled_clear()
                oled_print("OK: " + str(round(current * 1000, 1)) + "mA")
            
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\n✓ Threshold monitor stopped")
        print("  Total high current alerts: " + str(alert_count))
        oled_clear()
        oled_print("Monitor Done")

def power_profiling_demo():
    """Detailed power profiling with statistics"""
    print("📊 Power Profiling Demo")
    
    # Setup
    nodes_clear()
    connect(ISENSE_PLUS, 35)
    connect(ISENSE_MINUS, 45)
    
    print("  ✓ Current sensor connected")
    print("  📊 Starting power profiling...")
    
    oled_clear()
    oled_print("Power Profiling")
    
    # Statistics tracking
    current_readings = []
    voltage_readings = []
    power_readings = []
    
    try:
        while True:
            # Read sensor data
            current = get_current(0)
            voltage = get_bus_voltage(0)
            power = get_power(0)
            
            # Update statistics
            current_readings.append(current)
            voltage_readings.append(voltage)
            power_readings.append(power)
            
            # Keep last 50 readings
            if len(current_readings) > 50:
                current_readings.pop(0)
                voltage_readings.pop(0)
                power_readings.pop(0)
            
            # Calculate statistics
            avg_current = sum(current_readings) / len(current_readings)
            avg_voltage = sum(voltage_readings) / len(voltage_readings)
            avg_power = sum(power_readings) / len(power_readings)
            
            max_current = max(current_readings)
            min_current = min(current_readings)
            
            # Display detailed stats every 10 readings
            if len(current_readings) % 10 == 0:
                print("📊 Power Profile:")
                print("  • Current: " + str(round(current * 1000, 3)) + "mA " +
                      "(avg: " + str(round(avg_current * 1000, 3)) + "mA, " +
                      "min: " + str(round(min_current * 1000, 3)) + "mA, " +
                      "max: " + str(round(max_current * 1000, 3)) + "mA)")
                print("  • Voltage: " + str(round(voltage, 3)) + "V " +
                      "(avg: " + str(round(avg_voltage, 3)) + "V)")
                print("  • Power: " + str(round(power * 1000, 3)) + "mW " +
                      "(avg: " + str(round(avg_power * 1000, 3)) + "mW)")
                print("  • Readings: " + str(len(current_readings)))
            
            # Update OLED with rotating display
            display_mode = int(time.time()) % 3
            oled_clear()
            if display_mode == 0:
                oled_print("I:" + str(round(current * 1000, 1)) + "mA")
            elif display_mode == 1:
                oled_print("V:" + str(round(voltage, 2)) + "V")
            else:
                oled_print("P:" + str(round(power * 1000, 1)) + "mW")
            
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\n✓ Power profiling stopped")
        print("📈 Final Power Profile:")
        print("  • Total readings: " + str(len(current_readings)))
        print("  • Average current: " + str(round(avg_current * 1000, 3)) + "mA")
        print("  • Average voltage: " + str(round(avg_voltage, 3)) + "V")
        print("  • Average power: " + str(round(avg_power * 1000, 3)) + "mW")
        print("  • Current range: " + str(round(min_current * 1000, 3)) + "mA to " + str(round(max_current * 1000, 3)) + "mA")
        
        oled_clear()
        oled_print("Profile Done")

def quick_current_test():
    """Quick current reading test"""
    print("⚡ Quick Current Test")
    
    # Setup
    nodes_clear()
    connect(ISENSE_PLUS, 35)
    connect(ISENSE_MINUS, 45)
    
    print("  Reading current sensor...")
    
    # Take 10 quick readings
    for i in range(10):
        current = get_current(0)
        voltage = get_bus_voltage(0)
        power = get_power(0)
        
        print("  Reading " + str(i+1) + ": " + str(round(current * 1000, 3)) + "mA, " +
              str(round(voltage, 3)) + "V, " + str(round(power * 1000, 3)) + "mW")
        
        oled_clear()
        oled_print(str(round(current * 1000, 1)) + "mA")
        time.sleep(0.5)
    
    print("✓ Quick test complete")

def run_all_current_demos():
    """Run all current monitoring demos"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                  ⚡ All Current Monitor Demos                   │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    demos = [
        ("Quick Test", quick_current_test),
        ("Threshold Monitor", current_threshold_monitor),
        ("Dual Sensor Monitor", dual_sensor_monitor),
        ("Power Profiling", power_profiling_demo),
        ("Basic Current Monitor", current_monitor_demo)
    ]
    
    for name, demo_func in demos:
        print("\n🎯 Running: " + name)
        print("─" * 40)
        try:
            demo_func()
        except KeyboardInterrupt:
            print("⏭️  Skipping to next demo...")
            continue
        
        if name != "Basic Current Monitor":  # Don't pause after last demo
            print("📋 Press Enter for next demo...")
            try:
                input()
            except KeyboardInterrupt:
                break
    
    print("\n✅ All current monitor demos complete!")

if __name__ == "__main__":
    print("Current Monitor Demo loaded!")
    print("\nAvailable functions:")
    print("  current_monitor_demo()     - Real-time current monitoring")
    print("  dual_sensor_monitor()      - Monitor both current sensors")
    print("  current_threshold_monitor() - Monitor with threshold alerts")
    print("  power_profiling_demo()     - Detailed power profiling")
    print("  quick_current_test()       - Quick current reading test")
    print("  run_all_current_demos()    - Run all current monitor demos")
else:
    print("Current monitor demo loaded. Try current_monitor_demo()!") 