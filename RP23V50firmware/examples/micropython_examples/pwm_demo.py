#!/usr/bin/env python3
"""
PWM Demo for Jumperless MicroPython API

This example demonstrates how to use the PWM functions with:
- Numeric pin numbers (1-8)
- GPIO constants (GPIO_1, GPIO_2, etc.)
- Low frequencies (down to 10Hz)
- Dynamic frequency and duty cycle changes

Requirements:
- Connect an LED to GPIO_1 (pin 1) for visual feedback
- Connect an oscilloscope to GPIO_2 (pin 2) for frequency measurement
"""

import time
import sys
import jumperless as jl

def pwm_basic_demo():
    """Basic PWM setup and control"""
    print("=== Basic PWM Demo ===")
    
    # Test with numeric pin (GPIO pin 1)
    print("Setting up PWM on pin 1 (1kHz, 50% duty cycle)")
    jl.pwm(1, 1000, 0.5)
    
    print("LED should be at medium brightness")
    time.sleep(2)
    
    # Change duty cycle
    print("Changing duty cycle to 10%")
    jl.pwm_set_duty_cycle(1, 0.1)
    time.sleep(2)
    
    print("Changing duty cycle to 90%")
    jl.pwm_set_duty_cycle(1, 0.9)
    time.sleep(2)
    
    # Stop PWM
    print("Stopping PWM")
    jl.pwm_stop(1)
    time.sleep(1)

def pwm_gpio_constants_demo():
    """PWM with GPIO constants"""
    print("\n=== GPIO Constants Demo ===")
    
    # Test with GPIO constant
    print("Setting up PWM on GPIO_2 (500Hz, 25% duty cycle)")
    jl.pwm(GPIO_2, 500, 0.25)
    
    print("PWM running on GPIO_2")
    time.sleep(2)
    
    # Change frequency
    print("Changing frequency to 2kHz")
    jl.pwm_set_frequency(GPIO_2, 2000)
    time.sleep(2)
    
    print("Stopping PWM on GPIO_2")
    jl.pwm_stop(GPIO_2)
    time.sleep(1)

def pwm_low_frequency_demo():
    """Very low frequency PWM demo"""
    print("\n=== Low Frequency PWM Demo ===")
    
    # Test very low frequency
    print("Setting up very low frequency PWM (0.5Hz) on GPIO_1")
    jl.pwm(GPIO_1, 0.5, 0.5)
    
    print("You should see the LED blink once per 2 seconds")
    time.sleep(8)  # Let it blink a few times
    
    # Test ultra-low frequency
    print("Setting ultra-low frequency PWM (0.1Hz) on GPIO_1")
    jl.pwm_set_frequency(GPIO_1, 0.1)
    
    print("LED should now blink once per 10 seconds")
    time.sleep(15)
    
    # Test minimum frequency
    print("Setting minimum frequency PWM (10Hz) on GPIO_1")
    jl.pwm_set_frequency(GPIO_1, 10)
    
    print("LED should now blink 10 times per second (fast!)")
    time.sleep(5)  # Let it blink for a few seconds
    
    jl.pwm_stop(GPIO_1)

def pwm_multiple_pins_demo():
    """Multiple PWM pins with different frequencies"""
    print("\n=== Multiple PWM Pins Demo ===")
    
    # Setup multiple PWM pins
    print("Setting up PWM on multiple pins:")
    print("- GPIO_1: 1Hz, 50% duty cycle")
    print("- GPIO_2: 2Hz, 25% duty cycle") 
    print("- GPIO_3: 4Hz, 75% duty cycle")
    
    jl.pwm(GPIO_1, 1, 0.5)
    jl.pwm(GPIO_2, 2, 0.25)
    jl.pwm(GPIO_3, 4, 0.75)
    
    print("All PWM signals running for 10 seconds...")
    time.sleep(10)
    
    # Stop all PWM
    print("Stopping all PWM signals")
    jl.pwm_stop(GPIO_1)
    jl.pwm_stop(GPIO_2)
    jl.pwm_stop(GPIO_3)

def pwm_error_handling_demo():
    """Demonstrate error handling"""
    print("\n=== Error Handling Demo ===")
    
    try:
        # Test invalid pin number
        print("Testing invalid pin number (should fail)")
        jl.pwm(99, 1000, 0.5)
    except ValueError as e:
        print(f"Expected error: {e}")
    
    try:
        # Test invalid frequency (too low)
        print("Testing frequency too low (should fail)")
        jl.pwm(1, 0.001, 0.5)
    except ValueError as e:
        print(f"Expected error: {e}")
    
    try:
        # Test invalid duty cycle
        print("Testing invalid duty cycle (should fail)")
        jl.pwm(1, 1000, 1.5)
    except ValueError as e:
        print(f"Expected error: {e}")
    
    print("Error handling tests completed")

def main():
    """Run all PWM demonstrations"""
    print("Jumperless PWM Demo")
    print("===================")
    print("This demo shows the enhanced PWM functionality")
    print("Press Ctrl+C to stop at any time\n")
    
    try:
        pwm_basic_demo()
        pwm_gpio_constants_demo()
        pwm_low_frequency_demo()
        pwm_multiple_pins_demo()
        pwm_error_handling_demo()
        
        print("\n=== Demo Complete ===")
        print("All PWM demonstrations completed successfully!")
        
    except KeyboardInterrupt:
        print("\nDemo interrupted by user")
        # Clean up any running PWM
        for pin in range(1, 9):
            try:
                jl.pwm_stop(pin)
            except:
                pass
        print("PWM cleanup completed")
    except Exception as e:
        print(f"Error during demo: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 