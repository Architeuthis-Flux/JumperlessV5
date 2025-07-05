#!/usr/bin/env python3
"""
Test script for the new GPIO functionality in routableBufferPower

This script demonstrates how to use the new useGPIO parameter to connect
the routable buffer to an unused GPIO instead of a DAC.

Usage:
    python3 routable_buffer_gpio_test.py
"""

import jumperless
import time

def test_routable_buffer_gpio():
    """Test the new GPIO functionality in routableBufferPower"""
    
    print("Testing routableBufferPower with GPIO option...")
    print("=" * 50)
    
    # First, let's see what GPIOs are available
    print("Available GPIOs: RP_GPIO_1 through RP_GPIO_8")
    print("Current GPIO states:")
    for i in range(1, 9):
        gpio_state = jumperless.get_gpio_state(i)
        print(f"  RP_GPIO_{i}: {'Connected' if gpio_state else 'Available'}")
    
    print("\n" + "=" * 50)
    
    # Test 1: Power on using DAC (default behavior)
    print("Test 1: Power on using DAC (default behavior)")
    jumperless.routable_buffer_power(1, 0, 0, 0)  # offOn=1, flash=0, force=0, useGPIO=0
    print("✓ Routable buffer powered on using DAC")
    time.sleep(1)
    
    # Test 2: Power off using DAC
    print("\nTest 2: Power off using DAC")
    jumperless.routable_buffer_power(0, 0, 0, 0)  # offOn=0, flash=0, force=0, useGPIO=0
    print("✓ Routable buffer powered off using DAC")
    time.sleep(1)
    
    # Test 3: Power on using GPIO (new functionality)
    print("\nTest 3: Power on using GPIO (new functionality)")
    jumperless.routable_buffer_power(1, 0, 0, 1)  # offOn=1, flash=0, force=0, useGPIO=1
    print("✓ Routable buffer powered on using GPIO")
    time.sleep(1)
    
    # Test 4: Power off using GPIO
    print("\nTest 4: Power off using GPIO")
    jumperless.routable_buffer_power(0, 0, 0, 1)  # offOn=0, flash=0, force=0, useGPIO=1
    print("✓ Routable buffer powered off using GPIO")
    time.sleep(1)
    
    print("\n" + "=" * 50)
    print("All tests completed!")
    print("\nNote: If no unused GPIOs are available, the function will")
    print("fall back to using DAC behavior automatically.")

def test_gpio_availability():
    """Test GPIO availability detection"""
    
    print("\nTesting GPIO availability detection...")
    print("=" * 50)
    
    # Check which GPIOs are currently connected
    connected_gpios = []
    available_gpios = []
    
    for i in range(1, 9):
        gpio_state = jumperless.get_gpio_state(i)
        if gpio_state:
            connected_gpios.append(i)
        else:
            available_gpios.append(i)
    
    print(f"Connected GPIOs: {connected_gpios}")
    print(f"Available GPIOs: {available_gpios}")
    
    if available_gpios:
        print(f"✓ Found {len(available_gpios)} available GPIO(s) for routable buffer")
    else:
        print("⚠ No GPIOs available - will fall back to DAC behavior")

if __name__ == "__main__":
    try:
        # Initialize jumperless module
        jumperless.init()
        
        # Run tests
        test_gpio_availability()
        test_routable_buffer_gpio()
        
    except Exception as e:
        print(f"Error during testing: {e}")
        print("Make sure the Jumperless device is connected and the jumperless module is available.") 