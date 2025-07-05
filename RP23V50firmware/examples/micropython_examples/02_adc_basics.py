"""
ADC (Analog-to-Digital Converter) Basics
========================================

This example demonstrates basic ADC operations using the jumperless module:
- Reading individual ADC channels
- Reading all ADC channels in sequence
- Continuous monitoring
- Voltage range demonstrations

Functions demonstrated:
- adc_get(channel) - Read ADC input voltage
- oled_print(text) - Display text on OLED
- time.sleep(seconds) - Add delays for demonstration

ADC Channels:
- 0: ADC channel 0
- 1: ADC channel 1
- 2: ADC channel 2
- 3: ADC channel 3
- Voltage range: typically 0V to 3.3V (depends on hardware)
"""

import time

def adc_basic_demo():
    """Demonstrate basic ADC reading operations"""
    
    oled_clear()
    oled_print("ADC Basic Demo")
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                               ADC BASIC DEMO                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Test reading individual ADC channels
    print("\n☺ Reading individual ADC channels:")
    
    channels = [0, 1, 2, 3]
    
    for channel in channels:
        voltage = adc_get(channel)
        
        oled_clear()
        oled_print("ADC" + str(channel) + ": " + str(voltage) + "V")
        print("  ADC" + str(channel) + ": " + str(round(voltage, 3)) + "V")
        
        time.sleep(1)
    
    print("\n✓ Individual ADC reading complete")

def adc_all_channels_demo():
    """Read all ADC channels in sequence"""
    
    oled_clear()
    oled_print("All ADC Channels")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           ALL ADC CHANNELS                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Read all channels 5 times
    for reading in range(5):
        print("\n☺ Reading set " + str(reading + 1) + ":")
        
        display_text = ""
        for channel in range(4):
            voltage = adc_get(channel)
            print("  ADC" + str(channel) + ": " + str(round(voltage, 3)) + "V")
            display_text = display_text + "ADC" + str(channel) + ":" + str(round(voltage, 2)) + "V "
        
        oled_clear()
        oled_print(display_text)
        time.sleep(2)
    
    print("\n✓ All channels demo complete")

def adc_continuous_monitor():
    """Continuously monitor ADC channels for 10 seconds"""
    
    oled_clear()
    oled_print("Continuous Monitor")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         CONTINUOUS MONITORING                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    print("\n☺ Monitoring ADC channels for 10 seconds...")
    print("   Press Ctrl+C to stop early")
    
    start_time = time.time()
    sample_count = 0
    
    try:
        while time.time() - start_time < 10.0:
            sample_count += 1
            current_time = time.time() - start_time
            
            # Read all channels
            voltages = []
            for channel in range(4):
                voltage = adc_get(channel)
                voltages.append(voltage)
            
            # Display on OLED (alternate between channels)
            display_channel = sample_count % 4
            oled_clear()
            oled_print("ADC" + str(display_channel) + ": " + str(round(voltages[display_channel], 3)) + "V")
            
            # Print to console every 10 samples
            if sample_count % 10 == 0:
                print("  [" + str(round(current_time, 1)) + "s] ADC0:" + str(round(voltages[0], 3)) + "V ADC1:" + str(round(voltages[1], 3)) + "V ADC2:" + str(round(voltages[2], 3)) + "V ADC3:" + str(round(voltages[3], 3)) + "V")
            
            time.sleep(0.1)
    
    except KeyboardInterrupt:
        print("\n△ Monitoring stopped by user")
    
    print("\n✓ Continuous monitoring complete")
    print("  Total samples: " + str(sample_count))

def adc_voltage_ranges_demo():
    """Demonstrate voltage range detection"""
    
    oled_clear()
    oled_print("Voltage Ranges")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           VOLTAGE RANGES                                    │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    print("\n☺ Analyzing voltage ranges on all ADC channels:")
    
    # Read channels and categorize voltage levels
    for channel in range(4):
        voltage = adc_get(channel)
        
        # Categorize voltage levels
        if voltage < 0.5:
            level = "Low"
            color_desc = "Low"
        elif voltage < 2.0:
            level = "Medium"
            color_desc = "Med"
        else:
            level = "High"
            color_desc = "High"
        
        oled_clear()
        oled_print("ADC" + str(channel) + ": " + color_desc)
        
        print("  ADC" + str(channel) + ": " + str(round(voltage, 3)) + "V [" + level + "]")
        
        time.sleep(1.5)
    
    print("\n✓ Voltage range analysis complete")

def adc_statistics_demo():
    """Collect statistics on ADC readings"""
    
    oled_clear()
    oled_print("ADC Statistics")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            ADC STATISTICS                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    print("\n☺ Collecting 20 samples per channel for statistics...")
    
    # Collect samples for each channel
    for channel in range(4):
        print("\n  Sampling ADC" + str(channel) + "...")
        
        samples = []
        for sample in range(20):
            voltage = adc_get(channel)
            samples.append(voltage)
            
            if sample % 5 == 0:
                oled_clear()
                oled_print("ADC" + str(channel) + " Sample " + str(sample + 1))
            
            time.sleep(0.1)
        
        # Calculate statistics
        min_val = min(samples)
        max_val = max(samples)
        avg_val = sum(samples) / len(samples)
        
        print("    Min: " + str(round(min_val, 3)) + "V")
        print("    Max: " + str(round(max_val, 3)) + "V")
        print("    Avg: " + str(round(avg_val, 3)) + "V")
        
        oled_clear()
        oled_print("ADC" + str(channel) + " Avg:" + str(round(avg_val, 3)) + "V")
        time.sleep(2)
    
    print("\n✓ Statistics collection complete")

def adc_realtime_demo():
    """Real-time voltage reading with interrupt handling"""
    
    oled_clear()
    oled_print("Real-time ADC")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           REAL-TIME ADC                                     │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    print("\n☺ Real-time ADC reading (Press Ctrl+Q to quit):")
    print("   Monitoring all 4 channels continuously")
    
    try:
        while True:
            # Read all channels
            voltages = []
            for channel in range(4):
                voltage = adc_get(channel)
                voltages.append(voltage)
            
            # Format display string
            display_line = ""
            for i, v in enumerate(voltages):
                display_line = display_line + "ADC" + str(i) + ":" + str(round(v, 2)) + "V "
            
            # Update OLED
            oled_clear()
            oled_print(display_line)
            
            # Print to console
            console_line = "  "
            for i, v in enumerate(voltages):
                console_line = console_line + "ADC" + str(i) + ":" + str(round(v, 3)) + "V  "
            print(console_line)
            
            time.sleep(0.5)
    
    except KeyboardInterrupt:
        print("\n△ Real-time monitoring stopped")
    
    print("\n✓ Real-time demo complete")

def run_all_adc_demos():
    """Run all ADC demonstration functions"""
    
    print("🚀 Starting Complete ADC Demonstration")
    print("═" * 75)
    
    adc_basic_demo()
    adc_all_channels_demo()
    adc_continuous_monitor()
    adc_voltage_ranges_demo()
    adc_statistics_demo()
    
    print("\n☺ Starting real-time demo (last demo)...")
    print("   This will run until you press Ctrl+C")
    time.sleep(2)
    
    adc_realtime_demo()
    
    oled_clear()
    oled_print("ADC Demo Complete!")
    print("\n🎉 All ADC demonstrations complete!")
    print("═" * 75)

# Run the demonstration
if __name__ == "__main__":
    run_all_adc_demos() 