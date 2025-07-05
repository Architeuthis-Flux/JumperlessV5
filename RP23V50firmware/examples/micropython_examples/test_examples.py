"""
Jumperless MicroPython Examples Test Runner
==========================================

This script runs all the example demonstrations in sequence.
Use this to quickly test all the jumperless module functionality.

Usage:
  exec(open('test_examples.py').read())
"""

import time

def run_all_examples():
    """Run all example demonstrations"""
    
    print("Starting Jumperless MicroPython Examples Test")
    print("=" * 50)
    
    oled_clear()
    oled_print("Running All Examples")
    
    examples = [
        ("DAC Basics", "01_dac_basics.py"),
        ("ADC Basics", "02_adc_basics.py"),
        ("GPIO Basics", "03_gpio_basics.py"),
        ("Node Connections", "04_node_connections.py")
    ]
    
    for i, (name, filename) in enumerate(examples):
        print("\nExample " + str(i + 1) + "/4: " + name)
        print("-" * 40)
        
        try:
            exec(open(filename).read())
            print("Example completed successfully")
        except Exception as e:
            print("Example failed: " + str(e))
        
        if i < len(examples) - 1:
            print("Pausing 3 seconds before next example...")
            time.sleep(3)
    
    oled_clear()
    oled_print("All Examples Complete!")
    print("\nAll examples completed!")
    print("=" * 50)

def quick_test():
    """Quick test of key functions"""
    
    print("Quick Jumperless Function Test")
    print("-" * 30)
    
    oled_clear()
    oled_print("Quick Test")
    
    # Quick DAC test
    print("DAC Test: Setting TOP_RAIL to 3.3V")
    dac_set(TOP_RAIL, 3.3)
    voltage = dac_get(TOP_RAIL)
    print("  TOP_RAIL: " + str(voltage) + "V")
    
    # Quick ADC test
    print("ADC Test: Reading ADC0")
    adc_voltage = adc_get(0)
    print("  ADC0: " + str(adc_voltage) + "V")
    
    # Quick GPIO test
    print("GPIO Test: Testing GPIO1")
    gpio_set_dir(1, True)  # Set as output
    gpio_set(1, True)
    state = gpio_get(1)
    print("  GPIO1: " + str(state))  # Convert GPIOState to string
    gpio_set(1, False)
    
    # Quick connection test
    print("Connection Test: Connecting holes 1-30")
    result = connect(1, 30)
    print("  Connect 1-30: " + str(result))  # Convert result to string
    
    connected = is_connected(1, 30)
    print("  Verification: " + str(connected))  # Convert result to string
    
    # Cleanup
    disconnect(1, 30)
    dac_set(TOP_RAIL, 0.0)
    
    oled_clear()
    oled_print("Quick Test Complete!")
    print("Quick test complete!")

if __name__ == "__main__":
    quick_test() 