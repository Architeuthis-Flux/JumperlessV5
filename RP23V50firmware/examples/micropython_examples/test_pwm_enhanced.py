#!/usr/bin/env python3
"""
Test script for enhanced PWM functionality
Tests both numeric pins and GPIO constants, plus low frequency support
"""

import jumperless as jl
import time

def test_pwm_enhanced():
    """Test enhanced PWM functionality"""
    print("Testing enhanced PWM functionality...")
    
    # Test 1: Numeric pin with low frequency
    print("\n1. Testing numeric pin with low frequency (0.1Hz)")
    try:
        jl.pwm_set_frequency(1, 0.1)
        print("✓ Low frequency (0.1Hz) accepted")
    except Exception as e:
        print(f"✗ Low frequency failed: {e}")
    
    # Test 2: GPIO constant
    print("\n2. Testing GPIO constant (GPIO_1)")
    try:
        jl.pwm(GPIO_1, 1000, 0.5)
        print("✓ GPIO_1 constant accepted")
    except Exception as e:
        print(f"✗ GPIO_1 constant failed: {e}")
    
    # Test 3: GPIO constant with low frequency
    print("\n3. Testing GPIO constant with low frequency")
    try:
        jl.pwm(GPIO_2, 0.5, 0.25)
        print("✓ GPIO_2 with 0.5Hz accepted")
    except Exception as e:
        print(f"✗ GPIO_2 with low frequency failed: {e}")
    
    # Test 4: PWM modification functions with GPIO constants
    print("\n4. Testing PWM modification with GPIO constants")
    try:
        jl.pwm_set_duty_cycle(GPIO_1, 0.75)
        print("✓ pwm_set_duty_cycle with GPIO_1 accepted")
    except Exception as e:
        print(f"✗ pwm_set_duty_cycle with GPIO_1 failed: {e}")
    
    try:
        jl.pwm_set_frequency(GPIO_2, 0.25)
        print("✓ pwm_set_frequency with GPIO_2 and 0.25Hz accepted")
    except Exception as e:
        print(f"✗ pwm_set_frequency with GPIO_2 failed: {e}")
    
    # Test 5: Stop PWM with GPIO constants
    print("\n5. Testing PWM stop with GPIO constants")
    try:
        jl.pwm_stop(GPIO_1)
        jl.pwm_stop(GPIO_2)
        print("✓ PWM stop with GPIO constants accepted")
    except Exception as e:
        print(f"✗ PWM stop with GPIO constants failed: {e}")
    
    # Test 6: Error conditions should still work
    print("\n6. Testing error conditions")
    try:
        jl.pwm_set_frequency(1, 1.0)  # Too low
        print("✗ Should have rejected 1.0Hz")
    except ValueError:
        print("✓ Correctly rejected frequency too low")
    
    try:
        jl.pwm(999, 1000, 0.5)  # Invalid pin
        print("✗ Should have rejected invalid pin")
    except ValueError:
        print("✓ Correctly rejected invalid pin")
    
    print("\nPWM enhancement tests completed!")

if __name__ == "__main__":
    test_pwm_enhanced() 