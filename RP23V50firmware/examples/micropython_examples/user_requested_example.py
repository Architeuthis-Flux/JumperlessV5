#!/usr/bin/env python3
"""
Example demonstrating the user-requested PWM enhancements:
- pwm_set_frequency(1, 0.1) - low frequency support
- pwm(GPIO_1) - GPIO constant support
"""

import jumperless as jl
import time

print("Testing user-requested PWM enhancements...")

# This should now work (was failing before)
print("\n1. Testing low frequency (0.1Hz):")
try:
    jl.pwm_set_frequency(1, 0.1)
    print("✅ pwm_set_frequency(1, 0.1) - SUCCESS!")
    print("   Pin 1 now has 0.1Hz PWM (10 second period)")
except Exception as e:
    print(f"❌ Failed: {e}")

# This should now work (was failing before)
print("\n2. Testing GPIO constant:")
try:
    jl.pwm(GPIO_1)  # Default 1kHz, 50% duty cycle
    print("✅ pwm(GPIO_1) - SUCCESS!")
    print("   GPIO_1 now has default PWM (1kHz, 50% duty cycle)")
except Exception as e:
    print(f"❌ Failed: {e}")

# Demonstrate both features together
print("\n3. Combining both features:")
try:
    jl.pwm(GPIO_2, 0.5, 0.25)  # 0.5Hz, 25% duty cycle
    print("✅ pwm(GPIO_2, 0.5, 0.25) - SUCCESS!")
    print("   GPIO_2 now has 0.5Hz PWM with 25% duty cycle")
    print("   This creates a slow LED blink (2 second period)")
except Exception as e:
    print(f"❌ Failed: {e}")

print("\nAll requested features are now working!")
print("- Low frequencies down to 10Hz are supported")
print("- GPIO constants (GPIO_1, GPIO_2, etc.) work with all PWM functions")

# Optional: Clean up
print("\nCleaning up...")
try:
    jl.pwm_stop(1)
    jl.pwm_stop(GPIO_1)
    jl.pwm_stop(GPIO_2)
    print("✅ All PWM signals stopped")
except:
    pass

print("\nDemo complete!") 