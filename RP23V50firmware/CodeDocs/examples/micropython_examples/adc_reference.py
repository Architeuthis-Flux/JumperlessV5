"""
ADC (Analog-to-Digital Converter) Reference
==========================================

Complete reference for all ADC functions in the Jumperless MicroPython module.
This file demonstrates every ADC-related function with practical examples.

Functions demonstrated:
- adc_get(channel) - Read ADC input voltage
- get_adc() - Alias for adc_get()

ADC Channels:
- 0-4: ADC channels 0-4
- Voltage range: Typically 0V to 3.3V (depends on hardware)

Usage:
  exec(open('micropython_examples/adc_reference.py').read())
"""

import time

def adc_basic_operations():
    """Demonstrate basic ADC operations"""
    
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            ADC BASIC OPERATIONS                             │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("ADC Basic Demo")
    
    print("\n☺ Reading all ADC channels:")
    
    # Read all ADC channels
    for channel in range(5):
        voltage = adc_get(channel)
        print("  adc_get(" + str(channel) + ") = " + str(voltage) + "V")
        
        oled_clear()
        oled_print("ADC" + str(channel) + ": " + str(voltage) + "V")
        time.sleep(0.8)
    
    # Using alias function
    print("\n☺ Using alias function:")
    voltage = get_adc(0)
    print("  get_adc(0) = " + str(voltage) + "V (alias function)")
    
    oled_clear()
    oled_print("ADC0: " + str(voltage) + "V")
    time.sleep(1)
    
    print("✓ Basic ADC operations complete")

def adc_continuous_monitoring():
    """Demonstrate continuous ADC monitoring"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         CONTINUOUS ADC MONITORING                          │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("ADC Monitoring")
    
    print("\n☺ Monitoring ADC channels (20 samples):")
    print("  Sample | ADC0     | ADC1     | ADC2     | ADC3     | ADC4")
    print("  -------|----------|----------|----------|----------|----------")
    
    for sample in range(20):
        readings = []
        for channel in range(5):
            voltage = adc_get(channel)
            readings.append(voltage)
        
        # Format readings for display
        readings_str = " | ".join([str(round(r, 3)) + "V" for r in readings])
        print("  " + str(sample + 1).rjust(6) + " | " + readings_str)
        
        # Show current sample on OLED
        oled_clear()
        oled_print("Sample " + str(sample + 1) + "/20")
        time.sleep(0.2)
    
    print("✓ Continuous monitoring complete")

def adc_statistics():
    """Calculate ADC statistics"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            ADC STATISTICS                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("ADC Statistics")
    
    print("\n☺ Collecting statistics for each ADC channel:")
    
    num_samples = 50
    
    for channel in range(5):
        print("\n  ADC" + str(channel) + " statistics (" + str(num_samples) + " samples):")
        
        # Collect readings
        readings = []
        min_val = float('inf')
        max_val = float('-inf')
        
        for sample in range(num_samples):
            voltage = adc_get(channel)
            readings.append(voltage)
            
            if voltage < min_val:
                min_val = voltage
            if voltage > max_val:
                max_val = voltage
            
            # Show progress
            if sample % 10 == 0:
                oled_clear()
                oled_print("ADC" + str(channel) + " " + str(sample) + "/" + str(num_samples))
            
            time.sleep(0.02)
        
        # Calculate statistics
        avg_val = sum(readings) / len(readings)
        
        # Calculate standard deviation
        variance = sum((x - avg_val) ** 2 for x in readings) / len(readings)
        std_dev = variance ** 0.5
        
        # Calculate noise (peak-to-peak)
        noise_pp = max_val - min_val
        
        print("    Average: " + str(round(avg_val, 4)) + "V")
        print("    Minimum: " + str(round(min_val, 4)) + "V")
        print("    Maximum: " + str(round(max_val, 4)) + "V")
        print("    Std Dev: " + str(round(std_dev, 4)) + "V")
        print("    Noise (p-p): " + str(round(noise_pp, 4)) + "V")
        
        # Show final results on OLED
        oled_clear()
        oled_print("ADC" + str(channel) + " Avg: " + str(round(avg_val, 3)) + "V")
        time.sleep(1)
    
    print("✓ Statistics collection complete")

def adc_calibration_test():
    """Test ADC calibration using known DAC voltages"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                          ADC CALIBRATION TEST                              │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("ADC Calibration")
    
    print("\n☺ Testing ADC accuracy using DAC reference:")
    print("  This test connects DAC0 to ADC0 to compare voltages")
    
    # Clear any existing connections
    nodes_clear()
    
    # Connect DAC0 to ADC0 for calibration
    connect(DAC0, ADC0)
    print("  Connected DAC0 to ADC0")
    
    # Test voltages
    test_voltages = [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0]
    
    print("\n  DAC Set  | DAC Read | ADC Read | Error    | Error %")
    print("  ---------|----------|----------|----------|----------")
    
    for target_voltage in test_voltages:
        # Set DAC voltage
        dac_set(DAC0, target_voltage)
        time.sleep(0.1)  # Allow settling
        
        # Read both DAC and ADC
        dac_actual = dac_get(DAC0)
        adc_reading = adc_get(0)
        
        # Calculate error
        error = adc_reading - dac_actual
        error_percent = (error / dac_actual) * 100 if dac_actual > 0 else 0
        
        print("  " + str(target_voltage) + "V     | " + str(dac_actual) + "V  | " + str(adc_reading) + "V  | " + str(error) + "V | " + str(error_percent) + "%")
        
        oled_clear()
        oled_print("Test: " + str(target_voltage) + "V")
        time.sleep(0.5)
    
    # Clean up
    dac_set(DAC0, 0.0)
    disconnect(DAC0, ADC0)
    
    print("  ✓ Calibration test complete")
    print("  ✓ DAC0 disconnected from ADC0")

def adc_voltage_divider_test():
    """Test ADC with voltage divider circuits"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                        VOLTAGE DIVIDER TEST                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Voltage Divider Test")
    
    print("\n☺ Testing ADC with different voltage sources:")
    print("  Connect voltage sources to ADC inputs for testing")
    
    # Set up different voltage sources using DACs
    voltage_sources = [
        (DAC0, 1.0, "1V Reference"),
        (DAC1, 2.0, "2V Reference"),
        (TOP_RAIL, 3.3, "3.3V Logic"),
        (BOTTOM_RAIL, 1.8, "1.8V Logic")
    ]
    
    print("\n  Setting up voltage sources:")
    for source, voltage, description in voltage_sources:
        dac_set(source, voltage)
        actual = dac_get(source)
        print("    " + description + ": " + str(actual) + "V")
    
    # Connect sources to ADC inputs via breadboard
    print("\n  Connecting sources to ADC inputs:")
    connect(DAC0, 10)      # 1V to breadboard
    connect(DAC1, 20)      # 2V to breadboard
    connect(TOP_RAIL, 30)  # 3.3V to breadboard
    connect(BOTTOM_RAIL, 40)  # 1.8V to breadboard
    
    # Connect ADC inputs to breadboard
    connect(ADC0, 15)      # ADC0 to breadboard
    connect(ADC1, 25)      # ADC1 to breadboard
    connect(ADC2, 35)      # ADC2 to breadboard
    connect(ADC3, 45)      # ADC3 to breadboard
    
    print("    Manual connections needed:")
    print("      Connect hole 10 to hole 15 (1V → ADC0)")
    print("      Connect hole 20 to hole 25 (2V → ADC1)")
    print("      Connect hole 30 to hole 35 (3.3V → ADC2)")
    print("      Connect hole 40 to hole 45 (1.8V → ADC3)")
    
    print("\n  Press Enter when connections are made...")
    input()
    
    # Read all ADC channels
    print("\n  Reading ADC channels:")
    for channel in range(4):
        voltage = adc_get(channel)
        print("    ADC" + str(channel) + ": " + str(voltage) + "V")
        
        oled_clear()
        oled_print("ADC" + str(channel) + ": " + str(voltage) + "V")
        time.sleep(1)
    
    # Clean up
    nodes_clear()
    for source, _, _ in voltage_sources:
        dac_set(source, 0.0)
    
    print("  ✓ Voltage divider test complete")

def adc_noise_analysis():
    """Analyze ADC noise characteristics"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           ADC NOISE ANALYSIS                               │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Noise Analysis")
    
    print("\n☺ Analyzing ADC noise with stable reference:")
    
    # Use DAC0 as stable reference
    reference_voltage = 2.5
    dac_set(DAC0, reference_voltage)
    connect(DAC0, ADC0)
    
    print("  Reference voltage: " + str(reference_voltage) + "V")
    print("  Collecting 100 samples...")
    
    # Collect many samples
    samples = []
    for i in range(100):
        voltage = adc_get(0)
        samples.append(voltage)
        
        if i % 10 == 0:
            oled_clear()
            oled_print("Sample " + str(i + 1) + "/100")
        
        time.sleep(0.01)
    
    # Analyze noise
    avg_voltage = sum(samples) / len(samples)
    min_voltage = min(samples)
    max_voltage = max(samples)
    
    # Calculate RMS noise
    rms_noise = (sum((x - avg_voltage) ** 2 for x in samples) / len(samples)) ** 0.5
    
    # Peak-to-peak noise
    pp_noise = max_voltage - min_voltage
    
    print("\n  Noise Analysis Results:")
    print("    Average voltage: " + str(round(avg_voltage, 5)) + "V")
    print("    Minimum voltage: " + str(round(min_voltage, 5)) + "V")
    print("    Maximum voltage: " + str(round(max_voltage, 5)) + "V")
    print("    RMS noise: " + str(round(rms_noise * 1000, 3)) + "mV")
    print("    Peak-to-peak noise: " + str(round(pp_noise * 1000, 3)) + "mV")
    
    # Show noise histogram (simplified)
    print("\n  Noise distribution (simplified histogram):")
    
    # Create bins for histogram
    bins = 10
    bin_width = pp_noise / bins
    histogram = [0] * bins
    
    for sample in samples:
        bin_index = int((sample - min_voltage) / bin_width)
        if bin_index >= bins:
            bin_index = bins - 1
        histogram[bin_index] += 1
    
    for i, count in enumerate(histogram):
        bin_voltage = min_voltage + i * bin_width
        bar = "█" * (count // 2)  # Scale down for display
        print("    " + str(round(bin_voltage, 4)) + "V: " + bar + " (" + str(count) + ")")
    
    # Clean up
    dac_set(DAC0, 0.0)
    disconnect(DAC0, ADC0)
    
    oled_clear()
    oled_print("Noise: " + str(round(rms_noise * 1000, 1)) + "mV RMS")
    time.sleep(2)
    
    print("  ✓ Noise analysis complete")

def adc_speed_test():
    """Test ADC sampling speed"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            ADC SPEED TEST                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Speed Test")
    
    print("\n☺ Testing ADC sampling speed:")
    
    # Test single channel speed
    print("  Testing single channel (ADC0) speed:")
    
    num_samples = 1000
    start_time = time.time()
    
    for i in range(num_samples):
        voltage = adc_get(0)
        
        if i % 100 == 0:
            oled_clear()
            oled_print("Sample " + str(i + 1) + "/" + str(num_samples))
    
    end_time = time.time()
    duration = end_time - start_time
    sample_rate = num_samples / duration
    
    print("    Samples: " + str(num_samples))
    print("    Duration: " + str(round(duration, 3)) + " seconds")
    print("    Sample rate: " + str(round(sample_rate, 1)) + " samples/second")
    
    # Test all channels speed
    print("\n  Testing all channels speed:")
    
    num_cycles = 200
    start_time = time.time()
    
    for cycle in range(num_cycles):
        for channel in range(5):
            voltage = adc_get(channel)
        
        if cycle % 20 == 0:
            oled_clear()
            oled_print("Cycle " + str(cycle + 1) + "/" + str(num_cycles))
    
    end_time = time.time()
    duration = end_time - start_time
    total_samples = num_cycles * 5
    sample_rate = total_samples / duration
    
    print("    Cycles: " + str(num_cycles))
    print("    Total samples: " + str(total_samples))
    print("    Duration: " + str(round(duration, 3)) + " seconds")
    print("    Sample rate: " + str(round(sample_rate, 1)) + " samples/second")
    
    oled_clear()
    oled_print("Speed: " + str(round(sample_rate, 0)) + " S/s")
    time.sleep(2)
    
    print("  ✓ Speed test complete")

def run_all_adc_demos():
    """Run all ADC demonstration functions"""
    
    print("🚀 Starting Complete ADC Reference Demonstration")
    print("═" * 75)
    
    demos = [
        ("Basic Operations", adc_basic_operations),
        ("Continuous Monitoring", adc_continuous_monitoring),
        ("Statistics", adc_statistics),
        ("Calibration Test", adc_calibration_test),
        ("Voltage Divider Test", adc_voltage_divider_test),
        ("Noise Analysis", adc_noise_analysis),
        ("Speed Test", adc_speed_test)
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
    oled_print("ADC Reference Complete!")
    print("\n🎉 All ADC demonstrations complete!")
    print("═" * 75)

def adc_quick_test():
    """Quick test of ADC functions"""
    
    print("⚡ Quick ADC Test")
    print("─" * 20)
    
    # Test all channels briefly
    for channel in range(5):
        voltage = adc_get(channel)
        print("ADC" + str(channel) + ": " + str(voltage) + "V")
    
    print("✓ Quick ADC test complete")

# Run the demonstration
if __name__ == "__main__":
    run_all_adc_demos() 