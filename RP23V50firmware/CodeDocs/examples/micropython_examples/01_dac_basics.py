"""
DAC (Digital-to-Analog Converter) Basics
========================================

This example demonstrates basic DAC operations using the jumperless module:
- Setting voltages on different DAC channels
- Reading back DAC values
- Voltage sweeping
- Using both channel numbers and constants

Functions demonstrated:
- dac_set(channel, voltage) - Set DAC output voltage
- dac_get(channel) - Get current DAC voltage
- oled_print(text) - Display text on OLED
- time.sleep(seconds) - Add delays for demonstration

DAC Channels:
- 0 or DAC0: DAC channel 0
- 1 or DAC1: DAC channel 1  
- 2 or TOP_RAIL: Top power rail
- 3 or BOTTOM_RAIL: Bottom power rail
- Voltage range: -8.0V to +8.0V
"""

import time

def dac_basic_demo():
    """Demonstrate basic DAC operations"""
    
    oled_clear()
    oled_print("DAC Basic Demo")
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                               DAC BASIC DEMO                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Test setting different voltages on different channels
    voltages = [0.0, 1.5, 3.3, 5.0, -2.5]
    channels = [0, 1, TOP_RAIL, BOTTOM_RAIL]
    
    for voltage in voltages:
        oled_clear()
        oled_print("Setting " + str(voltage) + "V")
        print("\n☺ Setting voltage: " + str(voltage) + "V")
        
        for channel in channels:
            dac_set(channel, voltage)
            readback = dac_get(channel)
            channel_name = get_channel_name(channel)
            print("  " + channel_name + ": " + str(readback) + "V")
        
        time.sleep(2)
    
    print("\n✓ Basic DAC test complete")

def dac_sweep_demo():
    """Demonstrate voltage sweeping"""
    
    oled_clear()
    oled_print("DAC Sweep Demo")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                               DAC SWEEP DEMO                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Sweep DAC0 from 0V to 5V in 0.5V steps
    print("\n☺ Sweeping DAC0 from 0V to 5V...")
    
    for voltage in [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0]:
        dac_set(DAC0, voltage)
        readback = dac_get(DAC0)
        
        oled_clear()
        oled_print("DAC0: " + str(readback) + "V")
        print("  DAC0 set to: " + str(voltage) + "V, reads: " + str(readback) + "V")
        
        time.sleep(0.5)
    
    # Sweep back down
    print("\n☺ Sweeping DAC0 back down to 0V...")
    
    for voltage in [4.5, 4.0, 3.5, 3.0, 2.5, 2.0, 1.5, 1.0, 0.5, 0.0]:
        dac_set(DAC0, voltage)
        readback = dac_get(DAC0)
        
        oled_clear()
        oled_print("DAC0: " + str(readback) + "V")
        print("  DAC0 set to: " + str(voltage) + "V, reads: " + str(readback) + "V")
        
        time.sleep(0.5)
    
    print("\n✓ DAC sweep complete")

def dac_rail_demo():
    """Demonstrate power rail control"""
    
    oled_clear()
    oled_print("Power Rail Demo")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                             POWER RAIL DEMO                                 │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Set common power rail voltages
    rail_voltages = [
        (TOP_RAIL, 5.0, "Top Rail 5V"),
        (BOTTOM_RAIL, 3.3, "Bottom Rail 3.3V"),
        (TOP_RAIL, 3.3, "Top Rail 3.3V"),
        (BOTTOM_RAIL, 1.8, "Bottom Rail 1.8V"),
        (TOP_RAIL, 0.0, "Top Rail 0V"),
        (BOTTOM_RAIL, 0.0, "Bottom Rail 0V")
    ]
    
    for rail, voltage, description in rail_voltages:
        dac_set(rail, voltage)
        readback = dac_get(rail)
        
        oled_clear()
        oled_print(description)
        print("☺ " + description + ": " + str(readback) + "V")
        
        time.sleep(2)
    
    print("\n✓ Power rail demo complete")

def dac_all_channels_demo():
    """Test all DAC channels simultaneously"""
    
    oled_clear()
    oled_print("All Channels")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           ALL CHANNELS DEMO                                 │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Set different voltages on all channels
    settings = [
        (DAC0, 2.5, "DAC0"),
        (DAC1, 3.3, "DAC1"), 
        (TOP_RAIL, 5.0, "TOP_RAIL"),
        (BOTTOM_RAIL, 1.8, "BOTTOM_RAIL")
    ]
    
    print("\n☺ Setting all channels to different voltages...")
    
    for channel, voltage, name in settings:
        dac_set(channel, voltage)
        readback = dac_get(channel)
        print("  " + name + ": " + str(readback) + "V")
    
    oled_clear()
    oled_print("All channels set!")
    time.sleep(3)
    
    # Read back all channels
    print("\n☺ Reading all channels...")
    display_text = ""
    for channel, expected, name in settings:
        actual = dac_get(channel)
        print("  " + name + ": " + str(actual) + "V")
        display_text = display_text + name + ":" + str(actual) + "V "
    
    oled_clear()
    oled_print(display_text)
    time.sleep(3)
    
    print("\n✓ All channels demo complete")

def dac_reset_demo():
    """Reset all DACs to 0V"""
    
    oled_clear()
    oled_print("Resetting DACs")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                              RESET ALL DACS                                 │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    channels = [DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL]
    names = ["DAC0", "DAC1", "TOP_RAIL", "BOTTOM_RAIL"]
    
    print("\n☺ Resetting all DAC channels to 0V...")
    
    for channel, name in zip(channels, names):
        dac_set(channel, 0.0)
        readback = dac_get(channel)
        print("  " + name + ": " + str(readback) + "V")
    
    oled_clear()
    oled_print("All DACs reset!")
    time.sleep(2)
    
    print("\n✓ DAC reset complete")

def get_channel_name(channel):
    """Helper function to get channel name"""
    if channel == 0:
        return "DAC0"
    elif channel == 1:
        return "DAC1"
    elif channel == 2:
        return "TOP_RAIL"
    elif channel == 3:
        return "BOTTOM_RAIL"
    else:
        return "Channel " + str(channel)

def run_all_dac_demos():
    """Run all DAC demonstration functions"""
    
    print("🚀 Starting Complete DAC Demonstration")
    print("═" * 75)
    
    dac_basic_demo()
    dac_sweep_demo()
    dac_rail_demo()
    dac_all_channels_demo()
    dac_reset_demo()
    
    oled_clear()
    oled_print("DAC Demo Complete!")
    print("\n🎉 All DAC demonstrations complete!")
    print("═" * 75)

# Run the demonstration
if __name__ == "__main__":
    run_all_dac_demos() 