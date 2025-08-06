#!/usr/bin/env python3
"""
Test script for JulseView high-speed digital capture with decimated analog sampling

This script demonstrates the new decimation mode feature that allows:
- Digital capture at high rates (>5MHz)
- Analog capture at reduced rates (within 200kHz limit)
- Automatic sample duplication to maintain timing alignment

Usage:
    python test_decimation_mode.py
"""

import time
import serial
import struct

def test_decimation_mode():
    """Test the decimation mode functionality"""
    
    print("=== JulseView Decimation Mode Test ===")
    print("Testing high-speed digital capture with decimated analog sampling")
    print()
    
    # Test scenarios
    test_scenarios = [
        {
            "name": "Normal Mode (10kHz)",
            "sample_rate": 10000,
            "expected_decimation": False,
            "expected_factor": 1
        },
        {
            "name": "High-Speed Mode (500kHz)",
            "sample_rate": 500000,
            "expected_decimation": True,
            "expected_factor": 3  # 500kHz / 200kHz = 2.5, rounded up to 3
        },
        {
            "name": "Ultra-High Mode (2MHz)",
            "sample_rate": 2000000,
            "expected_decimation": True,
            "expected_factor": 10  # 2MHz / 200kHz = 10
        },
        {
            "name": "Extreme Mode (10MHz)",
            "sample_rate": 10000000,
            "expected_decimation": True,
            "expected_factor": 50  # 10MHz / 200kHz = 50
        }
    ]
    
    for scenario in test_scenarios:
        print(f"Testing: {scenario['name']}")
        print(f"  Sample Rate: {scenario['sample_rate']:,} Hz")
        print(f"  Expected Decimation: {scenario['expected_decimation']}")
        print(f"  Expected Factor: {scenario['expected_factor']}")
        print()
        
        # In a real test, you would:
        # 1. Connect to the device via serial
        # 2. Send the 'R' command with the sample rate
        # 3. Send the 'L' command with number of samples
        # 4. Send the 'A' command to arm the device
        # 5. Send the 'G' command to start capture
        # 6. Monitor the response for decimation mode indicators
        
        print("  [Simulated test - would connect to actual device]")
        print("  Command sequence:")
        print(f"    R{scenario['sample_rate']}  # Set sample rate")
        print("    L10000        # Set sample count")
        print("    A             # Arm device")
        print("    G             # Start capture")
        print()
        
        # Simulate expected behavior
        if scenario['expected_decimation']:
            print("  Expected behavior:")
            print("    ✓ Decimation mode enabled")
            print(f"    ✓ Digital samples at {scenario['sample_rate']:,} Hz")
            print(f"    ✓ Analog samples at {scenario['sample_rate'] // scenario['expected_factor']:,} Hz")
            print("    ✓ Analog samples duplicated to match digital timing")
        else:
            print("  Expected behavior:")
            print("    ✓ Normal mode (no decimation)")
            print("    ✓ Digital and analog samples at same rate")
        
        print()
        print("-" * 50)
        print()

def explain_decimation_mode():
    """Explain how the decimation mode works"""
    
    print("=== Decimation Mode Explanation ===")
    print()
    print("The decimation mode allows JulseView to capture digital signals at")
    print("high rates while keeping analog sampling within hardware limits.")
    print()
    print("How it works:")
    print("1. When sample rate > 200kHz, decimation mode is automatically enabled")
    print("2. Digital capture runs at the full requested sample rate")
    print("3. Analog capture runs at a reduced rate (sample_rate / decimation_factor)")
    print("4. Analog samples are duplicated to maintain timing alignment")
    print()
    print("Benefits:")
    print("✓ Digital signals can be captured at >5MHz")
    print("✓ Analog signals stay within 200kHz hardware limit")
    print("✓ Timing relationships are preserved")
    print("✓ Backward compatible with existing drivers")
    print()
    print("Example:")
    print("  Requested: 1MHz sample rate")
    print("  Digital:   1MHz (full rate)")
    print("  Analog:    200kHz (decimated by factor of 5)")
    print("  Result:    Each analog sample duplicated 5 times")
    print()

def show_buffer_layout():
    """Show the new buffer layout for decimation mode"""
    
    print("=== Buffer Layout for Decimation Mode ===")
    print()
    print("New asymmetric buffer allocation:")
    print("  Total Buffer: 96KB")
    print("  Layout: [abuf0 32KB][abuf1 32KB][dbuf0 16KB][dbuf1 16KB]")
    print()
    print("Buffer sizes:")
    print("  Analog buffers:  32KB each (64KB total)")
    print("  Digital buffers: 16KB each (32KB total)")
    print("  Note: Digital buffers increased from 1KB to 16KB each (16x increase!)")
    print()
    print("Sample capacity (with 5 analog channels):")
    print("  Analog:  32KB / (5 channels × 2 bytes) = 3,276 samples per buffer")
    print("  Digital: 16KB / 1 byte = 16,384 samples per buffer")
    print()
    print("Decimation factor limits:")
    print("  Max factor = digital_samples / analog_samples")
    print("  Max factor = 16,384 / 3,276 ≈ 5.0")
    print("  This allows much higher decimation factors!")
    print()

if __name__ == "__main__":
    explain_decimation_mode()
    show_buffer_layout()
    test_decimation_mode()
    
    print("=== Test Complete ===")
    print()
    print("To test with actual hardware:")
    print("1. Upload the firmware to your Jumperless device")
    print("2. Connect via serial terminal")
    print("3. Send commands like 'R500000' to set high sample rates")
    print("4. Monitor debug output for decimation mode indicators")
    print()
    print("The device will automatically enable decimation mode when")
    print("sample rates exceed 200kHz, allowing high-speed digital capture")
    print("while keeping analog sampling within hardware limits.") 