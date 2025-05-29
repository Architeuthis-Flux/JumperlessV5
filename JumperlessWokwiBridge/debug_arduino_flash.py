#!/usr/bin/env python3
"""
Debug script to check Arduino flashing status and variables
"""

import sys
import os

# Add current directory to path to import the main module
sys.path.insert(0, '.')

# Import the main script as a module
try:
    import JumperlessWokwiBridge as bridge
    print("✅ Successfully imported JumperlessWokwiBridge")
except ImportError as e:
    print(f"❌ Failed to import JumperlessWokwiBridge: {e}")
    sys.exit(1)

def check_arduino_flash_status():
    """Check the current status of Arduino flashing variables"""
    print("\n=== Arduino Flash Debug Status ===")
    
    # Check pyduinocli availability
    print(f"PYDUINOCLI_AVAILABLE: {bridge.PYDUINOCLI_AVAILABLE}")
    
    # Check global variables
    print(f"noArduinocli: {bridge.noArduinocli}")
    print(f"disableArduinoFlashing: {bridge.disableArduinoFlashing}")
    print(f"arduino object: {bridge.arduino}")
    print(f"arduinoPort: {bridge.arduinoPort}")
    print(f"updateInProgress: {bridge.updateInProgress}")
    print(f"arduino_flash_active: {bridge.arduino_flash_active}")
    
    # Check slot assignments
    print(f"numAssignedSlots: {bridge.numAssignedSlots}")
    print(f"slotURLs: {bridge.slotURLs}")
    
    # Calculate the main condition for Arduino flashing
    main_condition = (not bridge.noArduinocli and 
                     not bridge.disableArduinoFlashing and 
                     bridge.updateInProgress == 0)
    print(f"\nMain Arduino flash condition: {main_condition}")
    
    if not main_condition:
        print("❌ Arduino flashing is disabled because:")
        if bridge.noArduinocli:
            print("  - noArduinocli is True")
        if bridge.disableArduinoFlashing:
            print("  - disableArduinoFlashing is True")
        if bridge.updateInProgress != 0:
            print("  - updateInProgress is not 0")
    else:
        print("✅ Arduino flashing should be enabled!")
    
    # Test Arduino CLI setup
    print("\n=== Testing Arduino CLI Setup ===")
    try:
        result = bridge.setup_arduino_cli()
        print(f"setup_arduino_cli() result: {result}")
        print(f"After setup - noArduinocli: {bridge.noArduinocli}")
        print(f"After setup - disableArduinoFlashing: {bridge.disableArduinoFlashing}")
        print(f"After setup - arduino object: {bridge.arduino}")
    except Exception as e:
        print(f"❌ Error calling setup_arduino_cli(): {e}")

def test_sketch_detection():
    """Test the sketch detection mechanism"""
    print("\n=== Testing Sketch Detection ===")
    
    # Sample Wokwi URL for testing
    test_url = "https://wokwi.com/projects/359651108432393217"
    print(f"Testing with URL: {test_url}")
    
    try:
        # Set up a test slot
        bridge.slotURLs[0] = test_url
        bridge.numAssignedSlots = 1
        bridge.arduinoPort = "/dev/cu.usbserial-test"  # Mock port for testing
        
        # Test the sketch processing
        result = bridge.process_wokwi_sketch_and_flash(test_url, 0)
        print(f"process_wokwi_sketch_and_flash result: {result}")
        
    except Exception as e:
        print(f"❌ Error testing sketch detection: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    print("Arduino Flash Debug Tool")
    print("=" * 50)
    
    check_arduino_flash_status()
    test_sketch_detection()
    
    print("\n=== Recommendations ===")
    if bridge.noArduinocli or bridge.disableArduinoFlashing:
        print("1. Run setup_arduino_cli() to enable Arduino flashing")
        print("2. Make sure Arduino CLI is working properly")
        print("3. Check that arduinoPort is set correctly")
    else:
        print("Arduino flashing should be working!")
        print("Check that:")
        print("- Wokwi slots are assigned")
        print("- Sketches contain valid Arduino code")
        print("- Serial connection is established") 