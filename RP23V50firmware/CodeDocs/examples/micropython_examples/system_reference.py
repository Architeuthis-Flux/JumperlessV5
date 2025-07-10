"""
System Functions Reference
=========================

Complete reference for all system functions in the Jumperless MicroPython module.
This file demonstrates every system-related function with practical examples.

Functions demonstrated:
- print_bridges() - Show current bridge connections
- print_nets() - Show network information
- print_chip_status() - Show hardware chip status
- arduino_reset() - Reset connected Arduino
- run_app(appName) - Run specific application
- help() - Show module help
- nodes_help() - Show node reference
- clickwheel_up(steps) - Scroll clickwheel up
- clickwheel_down(steps) - Scroll clickwheel down
- clickwheel_press() - Press clickwheel center

Display Functions:
- oled_print(text) - Display text on OLED
- oled_clear() - Clear OLED display
- oled_show() - Update OLED display
- oled_connect() - Connect OLED
- oled_disconnect() - Disconnect OLED

Usage:
  exec(open('micropython_examples/system_reference.py').read())
"""

import time

def system_status_functions():
    """Demonstrate system status functions"""
    
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           SYSTEM STATUS FUNCTIONS                           │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("System Status Demo")
    
    print("\n☺ System status information:")
    
    # Bridge status
    print("  Current bridge connections:")
    print_bridges()
    
    print("\n  Network information:")
    print_nets()
    
    print("\n  Hardware chip status:")
    print_chip_status()
    
    oled_clear()
    oled_print("Status Complete")
    time.sleep(2)
    
    print("✓ System status functions complete")

def oled_display_functions():
    """Demonstrate OLED display functions"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           OLED DISPLAY FUNCTIONS                            │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    print("\n☺ OLED display operations:")
    
    # Clear display
    oled_clear()
    print("  oled_clear() - Display cleared")
    time.sleep(1)
    
    # Basic text display
    test_messages = [
        "Hello Jumperless!",
        "System Test",
        "Line 1\nLine 2",
        "Numbers: 12345",
        "Symbols: !@#$%",
        "Mixed: ABC123"
    ]
    
    print("  Testing text display:")
    for i, message in enumerate(test_messages):
        oled_clear()
        oled_print(message)
        print("    Message " + str(i + 1) + ": \"" + message.replace('\n', '\\n') + "\"")
        time.sleep(1.5)
    
    # Test displaying different data types
    print("\n  Testing different data types:")
    test_data = [
        (42, "Integer"),
        (3.14159, "Float"),
        (True, "Boolean"),
        ([1, 2, 3], "List"),
        ({"key": "value"}, "Dictionary")
    ]
    
    for data, data_type in test_data:
        oled_clear()
        oled_print(str(data))
        print("    " + data_type + ": " + str(data))
        time.sleep(1.5)
    
    # Test OLED connection functions
    print("\n  Testing OLED connection:")
    print("    oled_disconnect() - Disconnecting OLED")
    oled_disconnect()
    time.sleep(1)
    
    print("    oled_connect() - Reconnecting OLED")
    oled_connect()
    time.sleep(1)
    
    oled_clear()
    oled_print("OLED Reconnected")
    print("    OLED reconnected successfully")
    
    # Test oled_show() function
    print("\n  Testing oled_show():")
    oled_clear()
    oled_print("Manual Update")
    oled_show()
    print("    oled_show() - Manual display update")
    
    time.sleep(2)
    
    print("✓ OLED display functions complete")

def clickwheel_functions():
    """Demonstrate clickwheel functions"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           CLICKWHEEL FUNCTIONS                              │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Clickwheel Demo")
    
    print("\n☺ Clickwheel control operations:")
    
    # Test clickwheel up
    print("  Testing clickwheel_up():")
    for steps in [1, 2, 3, 5]:
        clickwheel_up(steps)
        print("    clickwheel_up(" + str(steps) + ") - Scrolled up " + str(steps) + " steps")
        
        oled_clear()
        oled_print("Up: " + str(steps) + " steps")
        time.sleep(1)
    
    # Test clickwheel down
    print("\n  Testing clickwheel_down():")
    for steps in [1, 2, 3, 5]:
        clickwheel_down(steps)
        print("    clickwheel_down(" + str(steps) + ") - Scrolled down " + str(steps) + " steps")
        
        oled_clear()
        oled_print("Down: " + str(steps) + " steps")
        time.sleep(1)
    
    # Test clickwheel press
    print("\n  Testing clickwheel_press():")
    for press in range(3):
        clickwheel_press()
        print("    clickwheel_press() - Press " + str(press + 1))
        
        oled_clear()
        oled_print("Press " + str(press + 1))
        time.sleep(1)
    
    print("✓ Clickwheel functions complete")

def help_functions():
    """Demonstrate help functions"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                             HELP FUNCTIONS                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Help Demo")
    
    print("\n☺ Help system functions:")
    
    # General help
    print("  Displaying general help:")
    print("    help() - General module help")
    help()
    
    print("\n  Displaying node help:")
    print("    nodes_help() - Node reference guide")
    nodes_help()
    
    oled_clear()
    oled_print("Help Available")
    time.sleep(2)
    
    print("✓ Help functions complete")

def app_control_functions():
    """Demonstrate application control functions"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         APPLICATION CONTROL FUNCTIONS                       │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("App Control Demo")
    
    print("\n☺ Application control operations:")
    
    # Test different app launches
    test_apps = [
        "voltmeter",
        "oscilloscope", 
        "logic_analyzer",
        "function_generator",
        "network_analyzer"
    ]
    
    print("  Testing run_app() with different applications:")
    for app in test_apps:
        print("    run_app(\"" + app + "\") - Launching " + app)
        try:
            run_app(app)
            print("      App launched successfully")
        except Exception as e:
            print("      App launch result: " + str(e))
        
        oled_clear()
        oled_print("App: " + app[:10])
        time.sleep(1.5)
    
    # Arduino reset function
    print("\n  Testing Arduino control:")
    print("    arduino_reset() - Reset connected Arduino")
    print("      Note: This would reset any connected Arduino")
    # arduino_reset()  # Commented out to avoid actually resetting
    
    oled_clear()
    oled_print("Arduino Reset Ready")
    time.sleep(2)
    
    print("✓ Application control functions complete")

def advanced_system_functions():
    """Demonstrate advanced system functions"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         ADVANCED SYSTEM FUNCTIONS                           │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Advanced Demo")
    
    print("\n☺ Advanced system operations:")
    
    # System state monitoring
    print("  System state monitoring:")
    
    # Create some connections for monitoring
    connect(1, 30)
    connect(5, 35)
    connect(GPIO_1, A0)
    
    print("    Created test connections")
    print("    Current system state:")
    print_bridges()
    
    # Monitor system with different operations
    operations = [
        ("DAC operation", lambda: dac_set(DAC0, 2.5)),
        ("ADC reading", lambda: adc_get(0)),
        ("GPIO control", lambda: gpio_set_dir(1, True)),
        ("Connection", lambda: connect(10, 40))
    ]
    
    print("\n  Monitoring system during operations:")
    for op_name, operation in operations:
        print("    Performing: " + op_name)
        
        try:
            result = operation()
            print("      Result: " + str(result))
        except Exception as e:
            print("      Error: " + str(e))
        
        # Show current status
        print("      System status after operation:")
        print_chip_status()
        
        oled_clear()
        oled_print("Op: " + op_name[:12])
        time.sleep(1.5)
    
    # Clean up
    nodes_clear()
    dac_set(DAC0, 0.0)
    
    print("\n  System cleanup completed")
    
    print("✓ Advanced system functions complete")

def system_diagnostics():
    """Run system diagnostics"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            SYSTEM DIAGNOSTICS                               │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Diagnostics")
    
    print("\n☺ System diagnostic tests:")
    
    # Test all major subsystems
    subsystems = [
        ("DAC System", lambda: [dac_set(i, 1.0) for i in range(4)]),
        ("ADC System", lambda: [adc_get(i) for i in range(5)]),
        ("GPIO System", lambda: [gpio_set_dir(i, True) for i in range(1, 5)]),
        ("Connection System", lambda: connect(1, 2)),
        ("INA System", lambda: [get_current(i) for i in range(2)]),
        ("Probe System", lambda: probe_read(False)),
        ("Button System", lambda: check_button()),
        ("OLED System", lambda: oled_print("Test"))
    ]
    
    results = []
    
    for subsystem, test_func in subsystems:
        print("  Testing " + subsystem + ":")
        
        try:
            result = test_func()
            print("    Status: PASS")
            print("    Result: " + str(result))
            results.append((subsystem, "PASS", None))
        except Exception as e:
            print("    Status: FAIL")
            print("    Error: " + str(e))
            results.append((subsystem, "FAIL", str(e)))
        
        oled_clear()
        oled_print("Test: " + subsystem[:10])
        time.sleep(1)
    
    # Summary
    print("\n  Diagnostic Summary:")
    print("  Subsystem        | Status | Notes")
    print("  -----------------|--------|--------")
    
    passed = 0
    for subsystem, status, error in results:
        notes = error[:20] + "..." if error and len(error) > 20 else (error or "OK")
        print("  " + subsystem.ljust(16) + " | " + status.ljust(6) + " | " + notes)
        if status == "PASS":
            passed += 1
    
    print("\n  Overall: " + str(passed) + "/" + str(len(results)) + " subsystems passing")
    
    # Clean up any test state
    nodes_clear()
    for i in range(4):
        dac_set(i, 0.0)
    
    oled_clear()
    oled_print("Diag: " + str(passed) + "/" + str(len(results)) + " OK")
    time.sleep(3)
    
    print("✓ System diagnostics complete")

def run_all_system_demos():
    """Run all system demonstration functions"""
    
    print("🚀 Starting Complete System Reference Demonstration")
    print("═" * 75)
    
    demos = [
        ("System Status Functions", system_status_functions),
        ("OLED Display Functions", oled_display_functions),
        ("Clickwheel Functions", clickwheel_functions),
        ("Help Functions", help_functions),
        ("App Control Functions", app_control_functions),
        ("Advanced System Functions", advanced_system_functions),
        ("System Diagnostics", system_diagnostics)
    ]
    
    for name, demo_func in demos:
        print("\n📍 Running: " + name)
        print("─" * 50)
        try:
            demo_func()
            print("✓ " + name + " completed successfully")
        except Exception as e:
            print("❌ " + name + " failed: " + str(e))
        
        time.sleep(2)
    
    oled_clear()
    oled_print("System Reference Complete!")
    print("\n🎉 All system demonstrations complete!")
    print("═" * 75)

def system_quick_test():
    """Quick test of system functions"""
    
    print("⚡ Quick System Test")
    print("─" * 25)
    
    # Test key system functions
    oled_clear()
    oled_print("Quick Test")
    
    print("OLED: Working")
    
    try:
        print_bridges()
        print("Bridges: Working")
    except:
        print("Bridges: Error")
    
    try:
        clickwheel_press()
        print("Clickwheel: Working")
    except:
        print("Clickwheel: Error")
    
    print("✓ Quick system test complete")

# Run the demonstration
if __name__ == "__main__":
    run_all_system_demos() 