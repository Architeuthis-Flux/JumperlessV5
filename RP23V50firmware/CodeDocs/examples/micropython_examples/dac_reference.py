"""
╭─────────────────────────────────────────────────────────────────╮
│                       DAC Reference Guide                       │
╰─────────────────────────────────────────────────────────────────╯

Complete Digital-to-Analog Converter (DAC) functions reference.
Covers all DAC operations, voltage control, and advanced patterns.

🔧 DAC Channels:
- DAC0 (0): Primary DAC output
- DAC1 (1): Secondary DAC output  
- TOP_RAIL (2): Top power rail
- BOTTOM_RAIL (3): Bottom power rail

⚡ Voltage Range: -8.0V to +8.0V
🎯 Resolution: 12-bit (4096 levels)

📋 Usage:
  exec(open('examples/micropython_examples/dac_reference.py').read())
  run_all_dac_demos()
"""

import time

def dac_basic_operations():
    """Demonstrate basic DAC operations"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                    📊 DAC Basic Operations                      │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("DAC Basic Ops")
    
    print("\n🔧 Basic DAC Functions:")
    print("  • dac_set(channel, voltage) - Set DAC output voltage")
    print("  • dac_get(channel) - Get current DAC voltage")
    print("  • set_dac(channel, voltage) - Alternative syntax")
    print("  • get_dac(channel) - Alternative syntax")
    
    # Test basic operations on each channel
    channels = [
        (DAC0, "DAC0"),
        (DAC1, "DAC1"),
        (TOP_RAIL, "TOP_RAIL"),
        (BOTTOM_RAIL, "BOTTOM_RAIL")
    ]
    
    test_voltages = [0.0, 1.0, 2.5, 3.3, 5.0, -1.0, -2.5]
    
    for channel, name in channels:
        print("\n📊 Testing " + name + ":")
        oled_clear()
        oled_print("Testing " + name)
        
        for voltage in test_voltages:
            print("  Setting " + str(voltage) + "V...")
            dac_set(channel, voltage)
            time.sleep(0.2)
            
            # Read back and verify
            actual = dac_get(channel)
            error = abs(actual - voltage)
            status = "✓" if error < 0.1 else "⚠️"
            
            print("    " + status + " Set: " + str(voltage) + "V, Read: " + str(round(actual, 3)) + "V")
            
            oled_clear()
            oled_print(name + ": " + str(round(actual, 2)) + "V")
            time.sleep(0.3)
    
    # Reset all channels
    print("\n🔄 Resetting all channels to 0V...")
    for channel, name in channels:
        dac_set(channel, 0.0)
    
    print("✅ Basic DAC operations complete!")

def dac_voltage_sweeping():
    """Demonstrate voltage sweeping on DAC channels"""
    print("🌊 DAC Voltage Sweeping Demo")
    
    oled_clear()
    oled_print("Voltage Sweep")
    
    # Sweep configurations
    sweeps = [
        (DAC0, 0.0, 3.3, 0.1, "DAC0 Low Sweep"),
        (DAC1, 0.0, 5.0, 0.2, "DAC1 Mid Sweep"),
        (TOP_RAIL, -2.0, 2.0, 0.1, "TOP_RAIL Bipolar"),
        (BOTTOM_RAIL, 0.0, 8.0, 0.4, "BOTTOM_RAIL High Sweep")
    ]
    
    for channel, start, end, step, name in sweeps:
        print("\n🌊 " + name + ":")
        print("  Range: " + str(start) + "V to " + str(end) + "V, Step: " + str(step) + "V")
        
        oled_clear()
        oled_print(name)
        
        # Sweep up
        voltage = start
        while voltage <= end:
            dac_set(channel, voltage)
            actual = dac_get(channel)
            
            print("    " + str(round(voltage, 1)) + "V → " + str(round(actual, 3)) + "V")
            
            oled_clear()
            oled_print(str(round(actual, 2)) + "V")
            
            voltage += step
            time.sleep(0.1)
        
        # Sweep down
        voltage = end
        while voltage >= start:
            dac_set(channel, voltage)
            actual = dac_get(channel)
            
            oled_clear()
            oled_print(str(round(actual, 2)) + "V")
            
            voltage -= step
            time.sleep(0.1)
        
        # Reset
        dac_set(channel, 0.0)
        time.sleep(0.5)
    
    print("\n✅ Voltage sweeping complete!")

def dac_precision_testing():
    """Test DAC precision and accuracy"""
    print("🎯 DAC Precision Testing")
    
    oled_clear()
    oled_print("Precision Test")
    
    # Test precision at different voltage levels
    test_points = [
        0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.3, 4.0, 5.0, 
        -0.5, -1.0, -1.5, -2.0, -2.5
    ]
    
    channels = [(DAC0, "DAC0"), (DAC1, "DAC1")]
    
    for channel, name in channels:
        print("\n🎯 Testing " + name + " precision:")
        errors = []
        
        for target_voltage in test_points:
            # Set voltage
            dac_set(channel, target_voltage)
            time.sleep(0.1)  # Allow settling
            
            # Take multiple readings
            readings = []
            for _ in range(5):
                reading = dac_get(channel)
                readings.append(reading)
                time.sleep(0.01)
            
            # Calculate statistics
            avg_reading = sum(readings) / len(readings)
            error = abs(avg_reading - target_voltage)
            errors.append(error)
            
            # Check repeatability
            max_reading = max(readings)
            min_reading = min(readings)
            repeatability = max_reading - min_reading
            
            print("  Target: " + str(target_voltage) + "V")
            print("    Average: " + str(round(avg_reading, 4)) + "V")
            print("    Error: " + str(round(error, 4)) + "V")
            print("    Repeatability: " + str(round(repeatability, 4)) + "V")
            
            oled_clear()
            oled_print(name + " Test")
            time.sleep(0.2)
        
        # Overall statistics
        max_error = max(errors)
        avg_error = sum(errors) / len(errors)
        
        print("  📊 " + name + " Summary:")
        print("    Max error: " + str(round(max_error, 4)) + "V")
        print("    Avg error: " + str(round(avg_error, 4)) + "V")
        print("    Test points: " + str(len(test_points)))
        
        # Reset
        dac_set(channel, 0.0)
    
    print("\n✅ Precision testing complete!")

def dac_waveform_generation():
    """Generate various waveforms using DAC"""
    print("🌊 DAC Waveform Generation")
    
    oled_clear()
    oled_print("Waveform Gen")
    
    # Waveform parameters
    amplitude = 2.0
    offset = 0.0
    steps = 50
    delay = 0.05
    
    # Generate sine wave
    print("\n🌊 Generating sine wave on DAC0...")
    oled_clear()
    oled_print("Sine Wave")
    
    for cycle in range(2):  # 2 complete cycles
        for i in range(steps):
            angle = (i * 2 * 3.14159) / steps
            voltage = offset + amplitude * ((2 * 3.14159 * angle) / (2 * 3.14159) - 
                                           (2 * 3.14159 * angle) ** 3 / (6 * (2 * 3.14159) ** 3))  # Rough sine approximation
            # Simpler sine approximation
            if i < steps // 4:
                voltage = offset + amplitude * (4 * i / steps)
            elif i < 3 * steps // 4:
                voltage = offset + amplitude * (2 - 4 * i / steps)
            else:
                voltage = offset + amplitude * (4 * i / steps - 4)
            
            dac_set(DAC0, voltage)
            time.sleep(delay)
    
    dac_set(DAC0, 0.0)
    
    # Generate triangle wave
    print("\n📐 Generating triangle wave on DAC1...")
    oled_clear()
    oled_print("Triangle Wave")
    
    for cycle in range(2):
        # Rising edge
        for i in range(steps // 2):
            voltage = offset + amplitude * (2 * i / steps)
            dac_set(DAC1, voltage)
            time.sleep(delay)
        
        # Falling edge
        for i in range(steps // 2):
            voltage = offset + amplitude * (1 - 2 * i / steps)
            dac_set(DAC1, voltage)
            time.sleep(delay)
    
    dac_set(DAC1, 0.0)
    
    # Generate square wave
    print("\n⬜ Generating square wave on TOP_RAIL...")
    oled_clear()
    oled_print("Square Wave")
    
    for cycle in range(4):
        # High level
        dac_set(TOP_RAIL, offset + amplitude)
        time.sleep(delay * 10)
        
        # Low level
        dac_set(TOP_RAIL, offset - amplitude)
        time.sleep(delay * 10)
    
    dac_set(TOP_RAIL, 0.0)
    
    print("\n✅ Waveform generation complete!")

def dac_channel_comparison():
    """Compare different DAC channels"""
    print("📊 DAC Channel Comparison")
    
    oled_clear()
    oled_print("Channel Compare")
    
    # Test same voltage on all channels
    test_voltage = 2.5
    channels = [
        (DAC0, "DAC0"),
        (DAC1, "DAC1"),
        (TOP_RAIL, "TOP_RAIL"),
        (BOTTOM_RAIL, "BOTTOM_RAIL")
    ]
    
    print("\n📊 Setting all channels to " + str(test_voltage) + "V:")
    
    results = []
    for channel, name in channels:
        dac_set(channel, test_voltage)
        time.sleep(0.1)
        
        actual = dac_get(channel)
        error = abs(actual - test_voltage)
        results.append((name, actual, error))
        
        print("  " + name + ": " + str(round(actual, 4)) + "V (error: " + str(round(error, 4)) + "V)")
    
    # Find best and worst
    best_channel = min(results, key=lambda x: x[2])
    worst_channel = max(results, key=lambda x: x[2])
    
    print("\n📈 Channel Comparison Results:")
    print("  Best accuracy: " + best_channel[0] + " (error: " + str(round(best_channel[2], 4)) + "V)")
    print("  Worst accuracy: " + worst_channel[0] + " (error: " + str(round(worst_channel[2], 4)) + "V)")
    
    # Test voltage range for each channel
    print("\n📊 Testing voltage ranges:")
    
    for channel, name in channels:
        print("  Testing " + name + " range:")
        oled_clear()
        oled_print(name + " Range")
        
        # Test positive range
        max_positive = 0.0
        for voltage in [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0]:
            dac_set(channel, voltage)
            time.sleep(0.1)
            actual = dac_get(channel)
            if abs(actual - voltage) < 0.5:  # Within reasonable range
                max_positive = voltage
        
        # Test negative range
        max_negative = 0.0
        for voltage in [-1.0, -2.0, -3.0, -4.0, -5.0, -6.0, -7.0, -8.0]:
            dac_set(channel, voltage)
            time.sleep(0.1)
            actual = dac_get(channel)
            if abs(actual - voltage) < 0.5:  # Within reasonable range
                max_negative = voltage
        
        print("    Positive range: 0V to " + str(max_positive) + "V")
        print("    Negative range: 0V to " + str(max_negative) + "V")
        
        # Reset
        dac_set(channel, 0.0)
    
    print("\n✅ Channel comparison complete!")

def dac_power_rail_demo():
    """Demonstrate power rail configuration"""
    print("⚡ DAC Power Rail Configuration")
    
    oled_clear()
    oled_print("Power Rails")
    
    # Common power rail voltages
    rail_configs = [
        (TOP_RAIL, 3.3, "3.3V Logic"),
        (BOTTOM_RAIL, 5.0, "5V Logic"),
        (TOP_RAIL, 1.8, "1.8V Low Power"),
        (BOTTOM_RAIL, 12.0, "12V Motor Supply"),
        (TOP_RAIL, -5.0, "Negative Supply"),
        (BOTTOM_RAIL, 0.0, "Ground Reference")
    ]
    
    for rail, voltage, description in rail_configs:
        rail_name = "TOP_RAIL" if rail == TOP_RAIL else "BOTTOM_RAIL"
        
        print("\n⚡ " + description + ":")
        print("  Setting " + rail_name + " to " + str(voltage) + "V")
        
        oled_clear()
        oled_print(rail_name + ": " + str(voltage) + "V")
        
        dac_set(rail, voltage)
        time.sleep(0.5)
        
        actual = dac_get(rail)
        error = abs(actual - voltage)
        
        print("  Actual: " + str(round(actual, 3)) + "V")
        print("  Error: " + str(round(error, 3)) + "V")
        
        time.sleep(1)
    
    # Reset rails
    print("\n🔄 Resetting power rails to safe values...")
    dac_set(TOP_RAIL, 0.0)
    dac_set(BOTTOM_RAIL, 0.0)
    
    oled_clear()
    oled_print("Rails Reset")
    
    print("✅ Power rail demo complete!")

def dac_alternative_syntax():
    """Demonstrate alternative DAC function syntax"""
    print("🔄 DAC Alternative Syntax Demo")
    
    oled_clear()
    oled_print("Alt Syntax")
    
    print("\n🔄 DAC functions have alternative syntax:")
    print("  dac_set() ≡ set_dac()")
    print("  dac_get() ≡ get_dac()")
    
    # Test both syntaxes
    test_pairs = [
        (DAC0, 2.5, "DAC0"),
        (DAC1, 1.8, "DAC1")
    ]
    
    for channel, voltage, name in test_pairs:
        print("\n📊 Testing " + name + " with both syntaxes:")
        
        # Using dac_set/dac_get
        print("  Using dac_set/dac_get:")
        dac_set(channel, voltage)
        reading1 = dac_get(channel)
        print("    dac_set(" + str(channel) + ", " + str(voltage) + ")")
        print("    dac_get(" + str(channel) + ") = " + str(round(reading1, 3)) + "V")
        
        time.sleep(0.5)
        
        # Using set_dac/get_dac
        print("  Using set_dac/get_dac:")
        set_dac(channel, voltage + 0.1)  # Slightly different to show change
        reading2 = get_dac(channel)
        print("    set_dac(" + str(channel) + ", " + str(voltage + 0.1) + ")")
        print("    get_dac(" + str(channel) + ") = " + str(round(reading2, 3)) + "V")
        
        oled_clear()
        oled_print(name + " OK")
        time.sleep(1)
        
        # Reset
        dac_set(channel, 0.0)
    
    print("\n✅ Alternative syntax demo complete!")

def quick_dac_test():
    """Quick DAC functionality test"""
    print("⚡ Quick DAC Test")
    
    oled_clear()
    oled_print("Quick Test")
    
    # Test each channel quickly
    channels = [DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL]
    channel_names = ["DAC0", "DAC1", "TOP_RAIL", "BOTTOM_RAIL"]
    
    for channel, name in zip(channels, channel_names):
        print("  Testing " + name + "...")
        
        # Set voltage
        dac_set(channel, 2.0)
        time.sleep(0.1)
        
        # Read back
        actual = dac_get(channel)
        
        # Check
        if abs(actual - 2.0) < 0.1:
            print("    ✅ " + name + ": " + str(round(actual, 3)) + "V")
        else:
            print("    ❌ " + name + ": " + str(round(actual, 3)) + "V (expected 2.0V)")
        
        oled_clear()
        oled_print(name + " OK")
        time.sleep(0.3)
        
        # Reset
        dac_set(channel, 0.0)
    
    print("✅ Quick DAC test complete!")

def run_all_dac_demos():
    """Run all DAC demonstration functions"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                   📊 Complete DAC Reference                     │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    demos = [
        ("Quick Test", quick_dac_test),
        ("Basic Operations", dac_basic_operations),
        ("Alternative Syntax", dac_alternative_syntax),
        ("Voltage Sweeping", dac_voltage_sweeping),
        ("Precision Testing", dac_precision_testing),
        ("Channel Comparison", dac_channel_comparison),
        ("Power Rail Demo", dac_power_rail_demo),
        ("Waveform Generation", dac_waveform_generation)
    ]
    
    print("\n🎯 DAC Reference Guide - All Functions")
    print("  This demo covers every DAC function in the jumperless module")
    print("  Press Ctrl+C during any demo to skip to the next one")
    
    for name, demo_func in demos:
        print("\n" + "="*60)
        print("🎯 Running: " + name)
        print("="*60)
        
        try:
            demo_func()
        except KeyboardInterrupt:
            print("⏭️  Skipping to next demo...")
            continue
        
        if name != "Waveform Generation":  # Don't pause after last demo
            print("\n📋 Press Enter for next demo...")
            try:
                input()
            except KeyboardInterrupt:
                break
    
    print("\n✅ Complete DAC reference guide finished!")
    print("📚 All DAC functions demonstrated:")
    print("  • dac_set() / set_dac() - Set DAC voltage")
    print("  • dac_get() / get_dac() - Read DAC voltage")
    print("  • All channels: DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL")
    print("  • Voltage range: -8.0V to +8.0V")

if __name__ == "__main__":
    print("DAC Reference Guide loaded!")
    print("\nAvailable functions:")
    print("  dac_basic_operations()   - Basic DAC set/get operations")
    print("  dac_voltage_sweeping()   - Voltage sweep demonstrations")
    print("  dac_precision_testing()  - Precision and accuracy testing")
    print("  dac_waveform_generation() - Generate sine, triangle, square waves")
    print("  dac_channel_comparison() - Compare all DAC channels")
    print("  dac_power_rail_demo()    - Power rail configuration examples")
    print("  dac_alternative_syntax() - Alternative function syntax")
    print("  quick_dac_test()         - Quick functionality test")
    print("  run_all_dac_demos()      - Run complete DAC reference guide")
else:
    print("DAC reference loaded. Try run_all_dac_demos() for complete guide!") 