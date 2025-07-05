"""
Jumperless Module Test
=====================

Quick test to verify that the jumperless module is working correctly.
This script performs basic tests of all major function categories:
- DAC operations
- ADC operations  
- GPIO operations
- Node connections
- OLED display

Run this first to make sure your Jumperless device is working properly
before trying the more comprehensive examples.

Usage:
  exec(open('micropython_examples/test_jumperless.py').read())
"""

import time

def test_jumperless_module():
    """Test basic jumperless module functionality"""
    
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                          JUMPERLESS MODULE TEST                             │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    test_results = []
    
    # Test OLED
    print("\n☺ Testing OLED display...")
    try:
        oled_clear()
        oled_print("Testing...")
        print("  ✓ OLED display working")
        test_results.append(("OLED", True, "Working"))
    except Exception as e:
        print("  ❌ OLED test failed: " + str(e))
        test_results.append(("OLED", False, str(e)))
    
    # Test DAC
    print("\n☺ Testing DAC functions...")
    try:
        dac_set(DAC0, 1.0)
        voltage = dac_get(DAC0)
        dac_set(DAC0, 0.0)  # Reset
        print("  ✓ DAC read/write working: " + str(voltage) + "V")
        test_results.append(("DAC", True, str(voltage) + "V"))
    except Exception as e:
        print("  ❌ DAC test failed: " + str(e))
        test_results.append(("DAC", False, str(e)))
    
    # Test ADC
    print("\n☺ Testing ADC functions...")
    try:
        voltage = adc_get(0)
        print("  ✓ ADC reading working: " + str(voltage) + "V")
        test_results.append(("ADC", True, str(voltage) + "V"))
    except Exception as e:
        print("  ❌ ADC test failed: " + str(e))
        test_results.append(("ADC", False, str(e)))
    
    # Test GPIO
    print("\n☺ Testing GPIO functions...")
    try:
        gpio_set_dir(1, True)  # Set as output
        gpio_set(1, True)      # Set high
        state = gpio_get(1)    # Read back
        gpio_set(1, False)     # Set low
        print("  ✓ GPIO control working: " + state)
        test_results.append(("GPIO", True, "State: " + state))
    except Exception as e:
        print("  ❌ GPIO test failed: " + str(e))
        test_results.append(("GPIO", False, str(e)))
    
    # Test Connections
    print("\n☺ Testing connection functions...")
    try:
        result = connect(1, 2)
        connected = is_connected(1, 2)
        disconnect(1, 2)
        print("  ✓ Connections working: " + connected)
        test_results.append(("Connections", True, "Result: " + connected))
    except Exception as e:
        print("  ❌ Connection test failed: " + str(e))
        test_results.append(("Connections", False, str(e)))
    
    # Test summary
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                              TEST SUMMARY                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    passed = 0
    failed = 0
    
    for test_name, success, details in test_results:
        if success:
            print("  ✓ " + test_name.ljust(12) + " - PASSED - " + details)
            passed += 1
        else:
            print("  ❌ " + test_name.ljust(12) + " - FAILED - " + details)
            failed += 1
    
    print("\n📊 Results: " + str(passed) + " passed, " + str(failed) + " failed")
    
    if failed == 0:
        print("🎉 All tests passed! Your Jumperless device is working correctly.")
        oled_clear()
        oled_print("All Tests Passed!")
    else:
        print("⚠️  Some tests failed. Check connections and try again.")
        oled_clear()
        oled_print("Some Tests Failed")
    
    return test_results

def quick_function_test():
    """Quick test of core functions without detailed output"""
    
    print("\n🎯 Quick Function Test")
    print("─" * 30)
    
    # Test each major function quickly
    functions_to_test = [
        ("dac_set", lambda: dac_set(DAC0, 0.0)),
        ("dac_get", lambda: dac_get(DAC0)),
        ("adc_get", lambda: adc_get(0)),
        ("gpio_set_dir", lambda: gpio_set_dir(1, True)),
        ("gpio_set", lambda: gpio_set(1, False)),
        ("gpio_get", lambda: gpio_get(1)),
        ("connect", lambda: connect(1, 2)),
        ("is_connected", lambda: is_connected(1, 2)),
        ("disconnect", lambda: disconnect(1, 2)),
        ("oled_clear", lambda: oled_clear()),
        ("oled_print", lambda: oled_print("Test"))
    ]
    
    working_functions = []
    broken_functions = []
    
    for func_name, func in functions_to_test:
        try:
            result = func()
            working_functions.append(func_name)
            print("  ✓ " + func_name)
        except Exception as e:
            broken_functions.append((func_name, str(e)))
            print("  ❌ " + func_name + " - " + str(e))
    
    print("\n📊 Function Test Results:")
    print("  Working: " + str(len(working_functions)) + "/" + str(len(functions_to_test)))
    print("  Broken:  " + str(len(broken_functions)))
    
    if len(broken_functions) == 0:
        print("✓ All core functions are working!")
        oled_clear()
        oled_print("Functions OK!")
    else:
        print("❌ Some functions are not working:")
        for func_name, error in broken_functions:
            print("    " + func_name + ": " + error)
    
    return working_functions, broken_functions

def test_node_constants():
    """Test that node constants are available"""
    
    print("\n🔍 Testing Node Constants")
    print("─" * 30)
    
    constants_to_test = [
        "DAC0", "DAC1", "TOP_RAIL", "BOTTOM_RAIL",
        "ADC0", "ADC1", "ADC2", "ADC3",
        "GPIO_1", "GPIO_2", "GPIO_3", "GPIO_4",
        "D13", "A0"
    ]
    
    available_constants = []
    missing_constants = []
    
    for const_name in constants_to_test:
        try:
            # Try to access the constant
            value = eval(const_name)
            available_constants.append((const_name, value))
            print("  ✓ " + const_name + " = " + str(value))
        except NameError:
            missing_constants.append(const_name)
            print("  ❌ " + const_name + " - not defined")
        except Exception as e:
            missing_constants.append(const_name)
            print("  ❌ " + const_name + " - error: " + str(e))
    
    print("\n📊 Constants Test Results:")
    print("  Available: " + str(len(available_constants)) + "/" + str(len(constants_to_test)))
    print("  Missing:   " + str(len(missing_constants)))
    
    if len(missing_constants) == 0:
        print("✓ All node constants are available!")
    else:
        print("❌ Missing constants: " + ", ".join(missing_constants))
    
    return available_constants, missing_constants

def run_all_tests():
    """Run all tests"""
    
    print("🚀 Starting Jumperless Module Tests")
    print("═" * 50)
    
    oled_clear()
    oled_print("Running Tests...")
    time.sleep(1)
    
    # Run tests
    basic_results = test_jumperless_module()
    time.sleep(2)
    
    working_funcs, broken_funcs = quick_function_test()
    time.sleep(2)
    
    available_consts, missing_consts = test_node_constants()
    time.sleep(2)
    
    # Final summary
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                             FINAL SUMMARY                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    basic_passed = sum(1 for _, success, _ in basic_results if success)
    total_basic = len(basic_results)
    
    print("📋 Test Categories:")
    print("  Basic Tests:      " + str(basic_passed) + "/" + str(total_basic) + " passed")
    print("  Functions:        " + str(len(working_funcs)) + "/11 working")
    print("  Node Constants:   " + str(len(available_consts)) + "/14 available")
    
    overall_health = (basic_passed == total_basic and 
                     len(broken_funcs) == 0 and 
                     len(missing_consts) == 0)
    
    if overall_health:
        print("\n🎉 OVERALL STATUS: EXCELLENT")
        print("   Your Jumperless device is fully functional!")
        oled_clear()
        oled_print("Device Status: EXCELLENT")
    else:
        print("\n⚠️  OVERALL STATUS: NEEDS ATTENTION")
        print("   Some features may not work properly.")
        oled_clear()
        oled_print("Device Status: CHECK NEEDED")
    
    print("\n💡 Next Steps:")
    if overall_health:
        print("   ✓ Run the full examples: exec(open('micropython_examples/run_examples.py').read())")
        print("   ✓ Try individual examples: exec(open('micropython_examples/01_dac_basics.py').read())")
    else:
        print("   🔧 Check hardware connections")
        print("   🔧 Verify jumperless module installation")
        print("   🔧 Try restarting the device")
    
    print("\n" + "═" * 75)

# Run tests when executed
if __name__ == "__main__":
    run_all_tests()
else:
    print("Jumperless test module loaded.")
    print("Run run_all_tests() to test your device.") 