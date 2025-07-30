#!/usr/bin/env python3
"""
Slow PWM Demo - Demonstrates hardware timer-based PWM for frequencies below 10Hz

This demo shows how the Jumperless can now generate very slow PWM signals
using hardware timers instead of the hardware PWM peripheral, which is
limited to ~10Hz minimum frequency.

The slow PWM implementation uses repeating timers to toggle GPIO pins
at precise intervals, enabling frequencies from 0.01Hz to 10Hz.
"""

import jumperless as jl
import time

def slow_pwm_basic_demo():
    """Basic slow PWM demonstration"""
    print("\n=== Slow PWM Basic Demo ===")
    
    # Test very slow frequency (0.1Hz = 10 second period)
    print("Setting up 0.1Hz PWM on GPIO_1 (10 second period)")
    jl.pwm(GPIO_1, 0.1, 0.5)  # 50% duty cycle
    
    print("LED should blink once every 10 seconds")
    print("Watch for 3 cycles (30 seconds)...")
    time.sleep(30)
    
    # Test ultra-slow frequency (0.05Hz = 20 second period)
    print("\nSetting up 0.05Hz PWM on GPIO_1 (20 second period)")
    jl.pwm_set_frequency(GPIO_1, 0.05)
    
    print("LED should now blink once every 20 seconds")
    print("Watch for 2 cycles (40 seconds)...")
    time.sleep(40)
    
    jl.pwm_stop(GPIO_1)
    print("Slow PWM stopped")

def slow_pwm_duty_cycle_demo():
    """Demonstrate duty cycle control with slow PWM"""
    print("\n=== Slow PWM Duty Cycle Demo ===")
    
    # Set up 1Hz PWM with 25% duty cycle
    print("Setting up 1Hz PWM on GPIO_2 with 25% duty cycle")
    jl.pwm(GPIO_2, 1.0, 0.25)
    
    print("LED should be ON for 0.25 seconds, OFF for 0.75 seconds")
    print("Watch for 5 cycles (5 seconds)...")
    time.sleep(5)
    
    # Change to 75% duty cycle
    print("\nChanging to 75% duty cycle")
    jl.pwm_set_duty_cycle(GPIO_2, 0.75)
    
    print("LED should now be ON for 0.75 seconds, OFF for 0.25 seconds")
    print("Watch for 5 cycles (5 seconds)...")
    time.sleep(5)
    
    jl.pwm_stop(GPIO_2)
    print("Duty cycle demo stopped")

def slow_pwm_frequency_sweep():
    """Sweep through different slow frequencies"""
    print("\n=== Slow PWM Frequency Sweep ===")
    
    frequencies = [0.1, 0.2, 0.5, 1.0, 2.0, 5.0]
    
    for freq in frequencies:
        print(f"Testing {freq}Hz PWM on GPIO_3")
        jl.pwm(GPIO_3, freq, 0.5)
        
        # Let it run for 3 periods
        period = 1.0 / freq
        duration = min(period * 3, 10)  # Cap at 10 seconds max
        print(f"Running for {duration:.1f} seconds...")
        time.sleep(duration)
    
    jl.pwm_stop(GPIO_3)
    print("Frequency sweep complete")

def slow_pwm_multiple_pins():
    """Run slow PWM on multiple pins simultaneously"""
    print("\n=== Multiple Pin Slow PWM Demo ===")
    
    # Set up different frequencies on different pins
    print("Setting up multiple slow PWM signals:")
    print("GPIO_1: 0.2Hz (5 second period)")
    print("GPIO_2: 0.5Hz (2 second period)")
    print("GPIO_3: 1.0Hz (1 second period)")
    
    jl.pwm(GPIO_1, 0.2, 0.5)
    jl.pwm(GPIO_2, 0.5, 0.5)
    jl.pwm(GPIO_3, 1.0, 0.5)
    
    print("All LEDs should blink at different rates")
    print("Running for 20 seconds...")
    time.sleep(20)
    
    # Stop all
    jl.pwm_stop(GPIO_1)
    jl.pwm_stop(GPIO_2)
    jl.pwm_stop(GPIO_3)
    print("Multiple pin demo stopped")

def slow_pwm_transition_demo():
    """Demonstrate smooth transition from slow to fast PWM"""
    print("\n=== Slow to Fast PWM Transition Demo ===")
    
    # Start with very slow PWM
    print("Starting with 0.1Hz PWM on GPIO_4")
    jl.pwm(GPIO_4, 0.1, 0.5)
    time.sleep(5)  # Let it run for 5 seconds
    
    # Gradually increase frequency
    frequencies = [0.2, 0.5, 1.0, 2.0, 5.0, 10.0, 50.0, 100.0]
    
    for freq in frequencies:
        print(f"Transitioning to {freq}Hz")
        jl.pwm_set_frequency(GPIO_4, freq)
        
        # Let it run for a few cycles
        period = 1.0 / freq
        duration = min(period * 3, 2)  # Cap at 2 seconds max
        time.sleep(duration)
    
    print("Transition complete - now using hardware PWM")
    time.sleep(3)
    
    jl.pwm_stop(GPIO_4)
    print("Transition demo stopped")

def main():
    """Run all slow PWM demos"""
    print("Slow PWM Demo - Hardware Timer-based PWM for Frequencies Below 10Hz")
    print("=" * 70)
    
    try:
        slow_pwm_basic_demo()
        time.sleep(2)
        
        slow_pwm_duty_cycle_demo()
        time.sleep(2)
        
        slow_pwm_frequency_sweep()
        time.sleep(2)
        
        slow_pwm_multiple_pins()
        time.sleep(2)
        
        slow_pwm_transition_demo()
        
    except KeyboardInterrupt:
        print("\nDemo interrupted by user")
    except Exception as e:
        print(f"Demo error: {e}")
    finally:
        # Clean up
        print("\nCleaning up...")
        for pin in [GPIO_1, GPIO_2, GPIO_3, GPIO_4]:
            try:
                jl.pwm_stop(pin)
            except:
                pass
        print("Demo complete!")

if __name__ == "__main__":
    main() 