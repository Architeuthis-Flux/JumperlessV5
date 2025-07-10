"""
╭─────────────────────────────────────────────────────────────────╮
│                    Voltage Monitor Demo                         │
╰─────────────────────────────────────────────────────────────────╯

Monitor voltage on ADC with real-time OLED display.
Continuously reads and displays voltage measurements.

🔧 Hardware Setup:
1. Connect voltage source to breadboard hole 20
2. Voltage range: 0V to 3.3V (ADC input range)
3. For higher voltages, use a voltage divider

📊 Features:
- Real-time voltage monitoring
- OLED display with large voltage readout
- Console logging with timestamps
- Statistics tracking

📋 Usage:
  exec(open('examples/micropython_examples/voltage_monitor.py').read())
  voltage_monitor()
"""

import time

def voltage_monitor():
    """Monitor voltage on ADC with OLED display"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                    📊 Voltage Monitor Demo                      │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    print("\n📋 Connect voltage source to ADC0 (breadboard hole 20)")
    
    # Setup connections
    nodes_clear()
    connect(ADC0, 20)  # Connect ADC0 to breadboard hole 20
    print("  ✓ ADC0 connected to hole 20")
    print("\n📋 Hardware Setup:")
    print("  • Connect voltage source to hole 20")
    print("  • Voltage range: 0V to 3.3V")
    print("  • For higher voltages, use voltage divider")
    print("  • Monitoring voltage on ADC0...")
    
    oled_clear()
    oled_print("Voltage Monitor")
    time.sleep(2)
    
    # Statistics tracking
    readings = []
    max_voltage = 0.0
    min_voltage = 3.3
    
    try:
        while True:
            voltage = adc_get(0)
            
            # Update statistics
            readings.append(voltage)
            if len(readings) > 100:  # Keep last 100 readings
                readings.pop(0)
            
            if voltage > max_voltage:
                max_voltage = voltage
            if voltage < min_voltage:
                min_voltage = voltage
            
            avg_voltage = sum(readings) / len(readings)
            
            # Display on OLED and console
            oled_clear()
            oled_print(str(round(voltage, 3)) + "V")
            
            # Detailed console output every 10 readings
            if len(readings) % 10 == 0:
                print("📊 Voltage: " + str(round(voltage, 3)) + "V | " +
                      "Avg: " + str(round(avg_voltage, 3)) + "V | " +
                      "Min: " + str(round(min_voltage, 3)) + "V | " +
                      "Max: " + str(round(max_voltage, 3)) + "V")
            
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\n✓ Monitor stopped")
        print("📈 Final Statistics:")
        print("  • Total readings: " + str(len(readings)))
        print("  • Average voltage: " + str(round(avg_voltage, 3)) + "V")
        print("  • Minimum voltage: " + str(round(min_voltage, 3)) + "V")
        print("  • Maximum voltage: " + str(round(max_voltage, 3)) + "V")
        print("  • Voltage range: " + str(round(max_voltage - min_voltage, 3)) + "V")
        
        oled_clear()
        oled_print("Monitor Done")

def multi_channel_monitor():
    """Monitor multiple ADC channels simultaneously"""
    print("📊 Multi-Channel Voltage Monitor")
    
    # Setup connections for multiple channels
    nodes_clear()
    connect(ADC0, 20)  # Channel 0 to hole 20
    connect(ADC1, 21)  # Channel 1 to hole 21
    connect(ADC2, 22)  # Channel 2 to hole 22
    connect(ADC3, 23)  # Channel 3 to hole 23
    
    print("  ✓ ADC0 → hole 20")
    print("  ✓ ADC1 → hole 21")
    print("  ✓ ADC2 → hole 22")
    print("  ✓ ADC3 → hole 23")
    
    oled_clear()
    oled_print("Multi-Channel")
    
    try:
        while True:
            voltages = []
            for channel in range(4):
                voltage = adc_get(channel)
                voltages.append(voltage)
            
            # Display on OLED (rotating channels)
            channel_index = int(time.time()) % 4
            oled_clear()
            oled_print("ADC" + str(channel_index) + ": " + 
                      str(round(voltages[channel_index], 3)) + "V")
            
            # Console output
            output = "📊 "
            for i, voltage in enumerate(voltages):
                output += "ADC" + str(i) + ":" + str(round(voltage, 3)) + "V "
            print(output)
            
            time.sleep(1)
            
    except KeyboardInterrupt:
        print("\n✓ Multi-channel monitor stopped")
        oled_clear()
        oled_print("Monitor Done")

def voltage_threshold_monitor():
    """Monitor voltage with threshold alerts"""
    print("⚠ Voltage Threshold Monitor")
    
    # Configuration
    LOW_THRESHOLD = 1.0   # Alert if voltage drops below this
    HIGH_THRESHOLD = 2.5  # Alert if voltage goes above this
    
    # Setup
    nodes_clear()
    connect(ADC0, 20)
    
    print("  ✓ ADC0 connected to hole 20")
    print("  ⚠ Low threshold: " + str(LOW_THRESHOLD) + "V")
    print("  ⚠ High threshold: " + str(HIGH_THRESHOLD) + "V")
    
    oled_clear()
    oled_print("Threshold Monitor")
    
    alert_count = 0
    
    try:
        while True:
            voltage = adc_get(0)
            
            # Check thresholds
            if voltage < LOW_THRESHOLD:
                alert_count += 1
                print("🔻 LOW ALERT: " + str(round(voltage, 3)) + "V < " + 
                      str(LOW_THRESHOLD) + "V")
                oled_clear()
                oled_print("LOW: " + str(round(voltage, 3)) + "V")
                
            elif voltage > HIGH_THRESHOLD:
                alert_count += 1
                print("🔺 HIGH ALERT: " + str(round(voltage, 3)) + "V > " + 
                      str(HIGH_THRESHOLD) + "V")
                oled_clear()
                oled_print("HIGH: " + str(round(voltage, 3)) + "V")
                
            else:
                # Normal reading
                oled_clear()
                oled_print("OK: " + str(round(voltage, 3)) + "V")
                print("✅ Normal: " + str(round(voltage, 3)) + "V")
            
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\n✓ Threshold monitor stopped")
        print("  Total alerts: " + str(alert_count))
        oled_clear()
        oled_print("Monitor Done")

def quick_voltage_test():
    """Quick voltage reading test"""
    print("⚡ Quick Voltage Test")
    
    # Setup
    nodes_clear()
    connect(ADC0, 20)
    
    print("  Reading ADC0 voltage...")
    
    # Take 10 quick readings
    for i in range(10):
        voltage = adc_get(0)
        print("  Reading " + str(i+1) + ": " + str(round(voltage, 3)) + "V")
        oled_clear()
        oled_print(str(round(voltage, 3)) + "V")
        time.sleep(0.5)
    
    print("✓ Quick test complete")

def run_all_voltage_demos():
    """Run all voltage monitoring demos"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                  📊 All Voltage Monitor Demos                   │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    demos = [
        ("Quick Test", quick_voltage_test),
        ("Threshold Monitor", voltage_threshold_monitor),
        ("Multi-Channel", multi_channel_monitor),
        ("Continuous Monitor", voltage_monitor)
    ]
    
    for name, demo_func in demos:
        print("\n🎯 Running: " + name)
        print("─" * 40)
        try:
            demo_func()
        except KeyboardInterrupt:
            print("⏭️  Skipping to next demo...")
            continue
        
        if name != "Continuous Monitor":  # Don't pause after continuous demo
            print("📋 Press Enter for next demo...")
            try:
                input()
            except KeyboardInterrupt:
                break
    
    print("\n✅ All voltage demos complete!")

if __name__ == "__main__":
    print("Voltage Monitor Demo loaded!")
    print("\nAvailable functions:")
    print("  voltage_monitor()          - Real-time voltage monitoring")
    print("  multi_channel_monitor()    - Monitor multiple ADC channels")
    print("  voltage_threshold_monitor() - Monitor with threshold alerts")
    print("  quick_voltage_test()       - Quick voltage reading test")
    print("  run_all_voltage_demos()    - Run all voltage demos")
else:
    print("Voltage monitor loaded. Try voltage_monitor()!") 