#!/usr/bin/env python3
"""
Jumperless Native MicroPython Module Demo

This example demonstrates using the native Jumperless MicroPython module
which integrates directly with the MicroPython interpreter without string parsing.

The module provides direct function calls like the built-in 'machine' module.
"""

import jumperless

def main():
    print("=== Jumperless Native MicroPython Module Demo ===")
    print()
    
    # DAC Operations
    print("DAC Operations:")
    jumperless.dac_set(0, 2.5)  # Set DAC channel 0 to 2.5V
    jumperless.dac_set(1, 1.8, False)  # Set DAC channel 1 to 1.8V, don't save
    voltage = jumperless.dac_get(0)  # Read DAC channel 0
    print(f"DAC 0 voltage: {voltage}V")
    print()
    
    # ADC Operations
    print("ADC Operations:")
    adc_reading = jumperless.adc_get(0)  # Read ADC channel 0
    print(f"ADC 0 reading: {adc_reading}V")
    print()
    
    # INA Sensor Operations
    print("INA Sensor Operations:")
    current = jumperless.ina_get_current(0)  # Get current from INA sensor 0
    voltage = jumperless.ina_get_voltage(0)  # Get shunt voltage
    bus_voltage = jumperless.ina_get_bus_voltage(0)  # Get bus voltage
    power = jumperless.ina_get_power(0)  # Get power
    print(f"INA 0 - Current: {current}mA, Shunt: {voltage}mV, Bus: {bus_voltage}V, Power: {power}mW")
    print()
    
    # GPIO Operations
    print("GPIO Operations:")
    jumperless.gpio_set(3, True)  # Set GPIO pin 3 HIGH
    jumperless.gpio_set(4, False)  # Set GPIO pin 4 LOW
    pin_state = jumperless.gpio_get(3)  # Read GPIO pin 3
    print(f"GPIO pin 3 state: {pin_state}")
    print()
    
    # Node Connection Operations
    print("Node Connection Operations:")
    jumperless.nodes_connect(1, 5)  # Connect node 1 to node 5
    jumperless.nodes_connect(10, 15, False)  # Connect nodes temporarily (don't save)
    jumperless.nodes_disconnect(1, 5)  # Disconnect nodes
    print("Node connections updated")
    print()
    
    # OLED Operations
    print("OLED Operations:")
    connected = jumperless.oled_connect()  # Connect to OLED
    if connected:
        jumperless.oled_print("Hello World!", 2)  # Print text with size 2
        jumperless.oled_print("MicroPython", 1)  # Print smaller text
        jumperless.oled_show()  # Update display
        print("Text displayed on OLED")
    else:
        print("OLED connection failed")
    print()
    
    # Arduino Operations
    print("Arduino Operations:")
    jumperless.arduino_reset()  # Reset Arduino
    print("Arduino reset")
    print()
    
    # Probe Operations
    print("Probe Operations:")
    jumperless.probe_tap(5)  # Simulate probe tap on node 5
    print("Probe tapped on node 5")
    print()
    
    # Clickwheel Operations
    print("Clickwheel Operations:")
    jumperless.clickwheel_up(3)  # Scroll up 3 clicks
    jumperless.clickwheel_down(1)  # Scroll down 1 click
    jumperless.clickwheel_press()  # Press the clickwheel
    print("Clickwheel operations completed")
    print()
    
    print("=== Demo Complete ===")

def test_error_handling():
    """Test the error handling in the native module"""
    print("\n=== Testing Error Handling ===")
    
    try:
        jumperless.dac_set(5, 2.5)  # Invalid channel (should be 0-3)
    except ValueError as e:
        print(f"Caught expected error: {e}")
    
    try:
        jumperless.gpio_set(15, True)  # Invalid pin (should be 1-10)  
    except ValueError as e:
        print(f"Caught expected error: {e}")
    
    try:
        jumperless.ina_get_current(3)  # Invalid sensor (should be 0-1)
    except ValueError as e:
        print(f"Caught expected error: {e}")

def comparison_demo():
    """Show the difference between old and new approaches"""
    print("\n=== Comparison: Old vs New Approach ===")
    print()
    
    print("OLD APPROACH (String-based):")
    print("  execute_sync('dac(set, 0, 2.5)')")
    print("  result = execute_sync('adc(get, 0)')")
    print("  execute_sync('nodes(connect, 1, 5)')")
    print()
    
    print("NEW APPROACH (Native calls):")
    print("  jumperless.dac_set(0, 2.5)")
    print("  result = jumperless.adc_get(0)")
    print("  jumperless.nodes_connect(1, 5)")
    print()
    
    print("Benefits of native approach:")
    print("✓ Direct function calls - no string parsing")
    print("✓ Proper Python types (float, bool, int)")
    print("✓ Native error handling with exceptions")
    print("✓ Better performance - no string processing")
    print("✓ Type safety and validation")
    print("✓ Integrated with MicroPython like built-in modules")

if __name__ == "__main__":
    main()
    test_error_handling()
    comparison_demo() 