#!/usr/bin/env python3
"""
Test script to verify Jumperless Mixed Signal driver is loaded in libsigrok
"""

import sys
import subprocess
import os

def test_sigrok_cli():
    """Test using sigrok-cli if available"""
    print("=== Testing with sigrok-cli ===")
    try:
        result = subprocess.run(['sigrok-cli', '--list-drivers'], 
                                capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            if 'jumperless-mixed-signal' in result.stdout:
                print("✅ SUCCESS: jumperless-mixed-signal driver found in sigrok-cli!")
                return True
            else:
                print("❌ ERROR: jumperless-mixed-signal driver NOT found in sigrok-cli")
                print("Available drivers:")
                for line in result.stdout.split('\n'):
                    if line.strip():
                        print(f"  - {line.strip()}")
                return False
        else:
            print(f"❌ ERROR: sigrok-cli failed with code {result.returncode}")
            print(f"Error: {result.stderr}")
            return False
    except FileNotFoundError:
        print("⚠️  WARNING: sigrok-cli not found in PATH")
        return None
    except subprocess.TimeoutExpired:
        print("❌ ERROR: sigrok-cli timed out")
        return False

def test_library_strings():
    """Test by checking library strings"""
    print("\n=== Testing library strings ===")
    
    # Find libsigrok library
    lib_paths = [
        '.libs/libsigrok.dylib',
        '.libs/libsigrok.so',
        '/usr/local/lib/libsigrok.dylib',
        '/usr/local/lib/libsigrok.so',
        '/opt/homebrew/lib/libsigrok.dylib'
    ]
    
    for lib_path in lib_paths:
        if os.path.exists(lib_path):
            print(f"Found library: {lib_path}")
            try:
                result = subprocess.run(['strings', lib_path], 
                                      capture_output=True, text=True, timeout=10)
                if 'jumperless-mixed-signal' in result.stdout:
                    print("✅ SUCCESS: jumperless-mixed-signal found in library strings!")
                    return True
                else:
                    print("❌ ERROR: jumperless-mixed-signal NOT found in library strings")
                    return False
            except Exception as e:
                print(f"❌ ERROR: Failed to run strings on {lib_path}: {e}")
                return False
    
    print("⚠️  WARNING: No libsigrok library found")
    return None

def test_pulseview_detection():
    """Guide user to test with PulseView"""
    print("\n=== PulseView Testing Guide ===")
    print("To test if the driver appears in PulseView:")
    print("1. Launch PulseView")
    print("2. Go to 'Connect to Device'")
    print("3. Look for 'Jumperless Mixed Signal Logic Analyzer' in the driver dropdown")
    print("4. If found, try connecting with your Jumperless device")
    print("")
    print("Expected connection settings:")
    print("  - Driver: Jumperless Mixed Signal Logic Analyzer")  
    print("  - Serial Port: /dev/cu.usbmodemJLV5port* (or similar)")
    print("  - Baud Rate: 115200 (default)")

def main():
    print("🔍 Testing Jumperless Mixed Signal Driver Integration")
    print("=" * 60)
    
    # Change to libsigrok directory if not already there
    if not os.path.exists('.libs') and os.path.exists('libsigrok-falaj'):
        os.chdir('libsigrok-falaj')
    
    results = []
    
    # Test with sigrok-cli
    cli_result = test_sigrok_cli()
    results.append(('sigrok-cli', cli_result))
    
    # Test library strings
    lib_result = test_library_strings()
    results.append(('library strings', lib_result))
    
    # Provide PulseView testing guide
    test_pulseview_detection()
    
    # Summary
    print("\n" + "=" * 60)
    print("📊 TEST SUMMARY")
    print("=" * 60)
    
    success_count = 0
    for test_name, result in results:
        if result is True:
            print(f"✅ {test_name}: PASSED")
            success_count += 1
        elif result is False:
            print(f"❌ {test_name}: FAILED")
        else:
            print(f"⚠️  {test_name}: SKIPPED")
    
    if success_count > 0:
        print(f"\n🎉 {success_count} test(s) passed! Driver appears to be properly integrated.")
        print("Next steps:")
        print("1. Flash the updated firmware to your Jumperless device")
        print("2. Test connection in PulseView using the guide above")
        print("3. Try both OLS compatibility mode and native Jumperless mode")
    else:
        print(f"\n😞 No tests passed. Driver may not be properly built or installed.")
        print("Troubleshooting:")
        print("1. Rebuild libsigrok: make clean && make")
        print("2. Install libsigrok: sudo make install")
        print("3. Check that PulseView is using the correct libsigrok version")

if __name__ == '__main__':
    main() 