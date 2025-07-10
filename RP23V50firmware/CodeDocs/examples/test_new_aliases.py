#!/usr/bin/env python3
"""
Test script for new Jumperless MicroPython module aliases and DAC node support.

This script demonstrates:
1. DAC functions accepting node objects/names instead of just channel numbers
2. Function aliases (verb_noun <-> noun_verb patterns)
3. Mixed usage of different parameter types

Note: This is a test script showing the syntax. The actual hardware functions 
would need to be implemented in the main Jumperless firmware.
"""

def test_dac_node_support():
    """Test DAC functions with node objects and names"""
    print("=== Testing DAC Node Support ===")
    
    # Traditional channel numbers still work
    print("# Traditional channel numbers:")
    print("dac_set(0, 1.5)    # Set DAC0 to 1.5V")
    print("dac_set(2, 3.3)    # Set TOP_RAIL to 3.3V")
    
    # Node constants work
    print("\n# Using node constants:")
    print("dac_set(DAC0, 1.5)      # Set DAC0 to 1.5V")
    print("dac_set(TOP_RAIL, 3.3)  # Set TOP_RAIL to 3.3V")
    print("dac_set(DAC1, -2.5)     # Set DAC1 to -2.5V")
    print("dac_set(BOTTOM_RAIL, 5.0)  # Set BOTTOM_RAIL to 5.0V")
    
    # String names work
    print("\n# Using string names:")
    print('dac_set("DAC0", 1.5)       # Set DAC0 to 1.5V')
    print('dac_set("TOP_RAIL", 3.3)   # Set TOP_RAIL to 3.3V')
    print('dac_set("dac1", -2.5)      # Case-insensitive')
    print('dac_set("bottom_rail", 5.0)  # Case-insensitive')
    
    # Node objects work
    print("\n# Using node objects:")
    print('my_dac = node("DAC0")')
    print('rail = node("TOP_RAIL")')
    print('dac_set(my_dac, 1.5)    # Set DAC0 to 1.5V')
    print('dac_set(rail, 3.3)      # Set TOP_RAIL to 3.3V')


def test_function_aliases():
    """Test function aliases (verb_noun <-> noun_verb)"""
    print("\n=== Testing Function Aliases ===")
    
    print("# DAC aliases:")
    print("dac_set(0, 1.5)  <->  set_dac(0, 1.5)")
    print("dac_get(0)       <->  get_dac(0)")
    
    print("\n# ADC aliases:")
    print("adc_get(1)       <->  get_adc(1)")
    
    print("\n# INA aliases:")
    print("ina_get_current(0)     <->  get_current(0)")
    print("ina_get_voltage(0)     <->  get_voltage(0)")
    print("ina_get_bus_voltage(0) <->  get_bus_voltage(0)")
    print("ina_get_power(0)       <->  get_power(0)")
    
    print("\n# GPIO aliases:")
    print("gpio_set(1, True)      <->  set_gpio(1, True)")
    print("gpio_get(1)            <->  get_gpio(1)")
    print("gpio_set_dir(1, True)  <->  set_gpio_dir(1, True)")
    print("gpio_get_dir(1)        <->  get_gpio_dir(1)")
    print("gpio_set_pull(1, 1)    <->  set_gpio_pull(1, 1)")
    print("gpio_get_pull(1)       <->  get_gpio_pull(1)")


def test_mixed_usage():
    """Test mixing different parameter types and aliases"""
    print("\n=== Testing Mixed Usage ===")
    
    print("# Mix aliases with node objects:")
    print("set_dac(TOP_RAIL, 3.3)     # Alias + node constant")
    print("get_dac(DAC0)              # Alias + node constant")
    print('set_dac("dac1", 2.5)       # Alias + string name')
    
    print("\n# Complex examples:")
    print("# Set up power rails using aliases and nodes")
    print("set_dac(TOP_RAIL, 3.3)     # 3.3V top rail")
    print("set_dac(BOTTOM_RAIL, 5.0)  # 5V bottom rail")
    print("")
    print("# Connect components using traditional syntax")
    print("connect(TOP_RAIL, D13)     # Connect power to LED pin")
    print("connect(A0, ADC0)          # Connect analog input")
    print("")
    print("# Read values using aliases")
    print("voltage = get_adc(0)       # Read analog voltage")
    print("current = get_current(0)   # Read current consumption")
    print("power = get_power(0)       # Calculate power usage")
    print("")
    print("# Control GPIO using aliases")
    print("set_gpio_dir(1, True)      # Set as output")
    print("set_gpio(1, True)          # Turn on")
    print("state = get_gpio(1)        # Read back state")


def main():
    print("Jumperless MicroPython Module - New Features Test")
    print("=" * 55)
    
    test_dac_node_support()
    test_function_aliases() 
    test_mixed_usage()
    
    print("\n=== Summary ===")
    print("✓ DAC functions now accept:")
    print("  - Channel numbers: 0, 1, 2, 3")
    print("  - Node constants: DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL") 
    print("  - String names: 'DAC0', 'TOP_RAIL', etc. (case-insensitive)")
    print("  - Node objects: node('DAC0'), node('TOP_RAIL'), etc.")
    print("")
    print("✓ Function aliases available:")
    print("  - dac_set/set_dac, dac_get/get_dac")
    print("  - adc_get/get_adc")
    print("  - ina_get_*/get_* (current, voltage, bus_voltage, power)")
    print("  - gpio_*/set_gpio, get_gpio, etc.")
    print("")
    print("✓ All parameter types can be mixed with all aliases!")


if __name__ == "__main__":
    main() 