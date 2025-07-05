"""
Probe and Button Reference
=========================

Complete reference for all probe and button functions in the Jumperless MicroPython module.
This file demonstrates every probe and button-related function with practical examples.

Functions demonstrated:
- probe_read([blocking=True]) - Read probe touch (blocking/non-blocking)
- probe_read_blocking() - Read probe touch (blocking)
- probe_read_nonblocking() - Read probe touch (non-blocking)
- wait_touch() - Wait for probe touch (alias)
- probe_touch() - Wait for probe touch (alias)
- probe_wait() - Wait for probe touch (alias)
- get_button([blocking=True]) - Read button press (blocking/non-blocking)
- probe_button() - Read button press (blocking)
- check_button() - Read button press (non-blocking)
- button_check() - Read button press (non-blocking, alias)
- probe_button_blocking() - Read button press (blocking)
- probe_button_nonblocking() - Read button press (non-blocking)

Probe Pads:
- 1-60: Breadboard holes
- Special pads: LOGO_PAD_TOP, D13_PAD, TOP_RAIL_PAD, BOTTOM_RAIL_PAD
- Returns: ProbePad objects with .value attribute

Button Types:
- BUTTON_NONE / NONE - No button pressed
- BUTTON_CONNECT / CONNECT / CONNECT_BUTTON - Connect button
- BUTTON_REMOVE / REMOVE / REMOVE_BUTTON - Remove button

Usage:
  exec(open('micropython_examples/probe_reference.py').read())
"""

import time

def probe_basic_operations():
    """Demonstrate basic probe operations"""
    
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                          PROBE BASIC OPERATIONS                             │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Probe Basic Demo")
    
    print("\n☺ Non-blocking probe reads:")
    print("  Testing probe_read(False) - returns immediately")
    
    for test in range(10):
        pad = probe_read(False)  # Non-blocking
        print("  Test " + str(test + 1) + ": probe_read(False) = " + str(pad))
        
        oled_clear()
        if pad and pad != -1:
            oled_print("Touched: " + str(pad))
        else:
            oled_print("No touch")
        
        time.sleep(0.5)
    
    print("\n☺ Non-blocking probe with alias functions:")
    
    # Test different non-blocking aliases
    aliases = [
        probe_read_nonblocking,
        lambda: probe_read(False)
    ]
    
    alias_names = [
        "probe_read_nonblocking()",
        "probe_read(False)"
    ]
    
    for alias, name in zip(aliases, alias_names):
        pad = alias()
        print("  " + name + " = " + str(pad))
    
    print("\n☺ Interactive demo:")
    print("  Touch a pad for interactive demo, or press Enter to skip...")
    
    try:
        # Wait briefly for user input
        import select
        import sys
        
        # Simple timeout mechanism
        start_time = time.time()
        touched = False
        
        while time.time() - start_time < 5:
            pad = probe_read(False)
            if pad and pad != -1:
                try:
                    if hasattr(pad, 'value'):
                        pad_num = pad.value
                    else:
                        pad_num = int(str(pad))
                    
                    print("  ✓ Touched pad: " + str(pad_num))
                    oled_clear()
                    oled_print("Touched: " + str(pad_num))
                    touched = True
                    break
                except (ValueError, AttributeError):
                    pass
            
            time.sleep(0.1)
        
        if not touched:
            print("  No touch detected in 5 seconds")
    
    except:
        print("  Interactive demo skipped")
    
    print("✓ Basic probe operations complete")

def probe_blocking_operations():
    """Demonstrate blocking probe operations"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         BLOCKING PROBE OPERATIONS                           │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Blocking Demo")
    
    print("\n☺ Blocking probe functions demonstration:")
    print("  These functions wait until a pad is touched")
    print("  Available blocking functions:")
    print("    - probe_read() or probe_read(True)")
    print("    - probe_read_blocking()")
    print("    - wait_touch()")
    print("    - probe_touch()")
    print("    - probe_wait()")
    
    print("\n☺ Simulated blocking demo (using timeouts):")
    print("  In real usage, these would wait indefinitely for touch")
    
    # Since we can't actually block in a demo, simulate it
    blocking_functions = [
        ("probe_read()", "probe_read()"),
        ("probe_read_blocking()", "probe_read_blocking()"),
        ("wait_touch()", "wait_touch()"),
        ("probe_touch()", "probe_touch()"),
        ("probe_wait()", "probe_wait()")
    ]
    
    for func_name, description in blocking_functions:
        print("\n  " + description + ":")
        print("    Would wait for touch...")
        
        oled_clear()
        oled_print("Wait: " + func_name)
        
        # Simulate waiting by checking for touch briefly
        found_touch = False
        for wait_time in range(20):  # 2 second timeout
            pad = probe_read(False)
            if pad and pad != -1:
                try:
                    if hasattr(pad, 'value'):
                        pad_num = pad.value
                    else:
                        pad_num = int(str(pad))
                    
                    print("    ✓ Would return: " + str(pad_num))
                    oled_clear()
                    oled_print("Got: " + str(pad_num))
                    found_touch = True
                    break
                except (ValueError, AttributeError):
                    pass
            
            time.sleep(0.1)
        
        if not found_touch:
            print("    Timeout (would continue waiting)")
        
        time.sleep(1)
    
    print("✓ Blocking probe operations complete")

def probe_special_pads():
    """Demonstrate special probe pad constants"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           SPECIAL PROBE PADS                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Special Pads Demo")
    
    print("\n☺ Special probe pad constants:")
    
    # List of special pad constants
    special_pads = [
        ("LOGO_PAD_TOP", LOGO_PAD_TOP, "Logo pad (top)"),
        ("D13_PAD", D13_PAD, "D13 Arduino pin pad"),
        ("TOP_RAIL_PAD", TOP_RAIL_PAD, "Top power rail pad"),
        ("BOTTOM_RAIL_PAD", BOTTOM_RAIL_PAD, "Bottom power rail pad")
    ]
    
    for name, constant, description in special_pads:
        print("  " + name + " = " + str(constant) + " (" + description + ")")
        
        oled_clear()
        oled_print(name + ": " + str(constant))
        time.sleep(1)
    
    print("\n☺ Testing for special pad touches:")
    print("  Checking for special pad touches (5 second test)")
    
    start_time = time.time()
    special_touches = []
    
    while time.time() - start_time < 5:
        pad = probe_read(False)
        if pad and pad != -1:
            try:
                # Check if it's a special pad
                pad_value = pad.value if hasattr(pad, 'value') else int(str(pad))
                
                for name, constant, description in special_pads:
                    if pad_value == constant or str(pad) == str(constant):
                        if pad_value not in [t[1] for t in special_touches]:
                            special_touches.append((name, pad_value, description))
                            print("  ✓ Special pad touched: " + name + " (" + description + ")")
                            
                            oled_clear()
                            oled_print("Special: " + name)
                            time.sleep(1)
                
            except (ValueError, AttributeError):
                pass
        
        time.sleep(0.1)
    
    if not special_touches:
        print("  No special pads touched during test")
    else:
        print("  Special pads touched: " + str(len(special_touches)))
    
    print("✓ Special probe pads complete")

def button_basic_operations():
    """Demonstrate basic button operations"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                          BUTTON BASIC OPERATIONS                            │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Button Basic Demo")
    
    print("\n☺ Button constants:")
    
    # List button constants
    button_constants = [
        ("BUTTON_NONE", BUTTON_NONE, "No button pressed"),
        ("BUTTON_CONNECT", BUTTON_CONNECT, "Connect button"),
        ("BUTTON_REMOVE", BUTTON_REMOVE, "Remove button"),
        ("CONNECT_BUTTON", CONNECT_BUTTON, "Connect button (alias)"),
        ("REMOVE_BUTTON", REMOVE_BUTTON, "Remove button (alias)"),
        ("CONNECT", CONNECT, "Connect button (short alias)"),
        ("REMOVE", REMOVE, "Remove button (short alias)"),
        ("NONE", NONE, "No button (short alias)")
    ]
    
    for name, constant, description in button_constants:
        print("  " + name + " = " + str(constant) + " (" + description + ")")
    
    print("\n☺ Non-blocking button reads:")
    print("  Testing button functions that return immediately")
    
    for test in range(10):
        button = check_button()
        print("  Test " + str(test + 1) + ": check_button() = " + str(button))
        
        oled_clear()
        oled_print("Button: " + str(button))
        time.sleep(0.5)
    
    print("✓ Basic button operations complete")

def button_blocking_operations():
    """Demonstrate blocking button operations"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         BLOCKING BUTTON OPERATIONS                          │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Button Blocking Demo")
    
    print("\n☺ Blocking button functions:")
    print("  These functions wait until a button is pressed")
    print("  Available blocking functions:")
    print("    - get_button() or get_button(True)")
    print("    - probe_button()")
    print("    - probe_button_blocking()")
    
    print("\n☺ Simulated blocking demo:")
    print("  In real usage, these would wait indefinitely for button press")
    
    blocking_functions = [
        ("get_button()", "get_button() or get_button(True)"),
        ("probe_button()", "probe_button()"),
        ("probe_button_blocking()", "probe_button_blocking()")
    ]
    
    for func_name, description in blocking_functions:
        print("\n  " + description + ":")
        print("    Would wait for button press...")
        
        oled_clear()
        oled_print("Wait: " + func_name)
        
        # Simulate waiting by checking for button briefly
        found_button = False
        for wait_time in range(30):  # 3 second timeout
            button = get_button(False)
            if button != BUTTON_NONE and button != NONE:
                button_name = "CONNECT" if button == BUTTON_CONNECT or button == CONNECT else "REMOVE"
                print("    ✓ Would return: " + button_name)
                oled_clear()
                oled_print("Got: " + button_name)
                found_button = True
                break
            
            time.sleep(0.1)
        
        if not found_button:
            print("    Timeout (would continue waiting)")
        
        time.sleep(1)
    
    print("✓ Blocking button operations complete")

def probe_button_combined():
    """Demonstrate combined probe and button monitoring"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                      COMBINED PROBE & BUTTON MONITORING                     │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Combined Demo")
    
    print("\n☺ Simultaneous probe and button monitoring:")
    print("  Monitoring both probe touches and button presses")
    
    print("\n  Time | Probe Pad | Button | Action")
    print("  -----|-----------|--------|--------")
    
    start_time = time.time()
    last_probe = None
    last_button = None
    events = 0
    
    for cycle in range(50):  # 5 second monitoring
        current_time = time.time() - start_time
        
        # Read probe and button
        probe_pad = probe_read(False)
        button = check_button()
        
        # Format probe reading
        probe_str = "None"
        if probe_pad and probe_pad != -1:
            try:
                if hasattr(probe_pad, 'value'):
                    probe_str = str(probe_pad.value)
                else:
                    probe_str = str(probe_pad)
            except (ValueError, AttributeError):
                probe_str = "Error"
        
        # Format button reading
        button_str = "NONE"
        if button == BUTTON_CONNECT or button == CONNECT:
            button_str = "CONNECT"
        elif button == BUTTON_REMOVE or button == REMOVE:
            button_str = "REMOVE"
        
        # Detect changes
        action = ""
        if probe_str != "None" and probe_str != last_probe:
            action = "PROBE_TOUCH"
            events += 1
        elif button_str != "NONE" and button_str != last_button:
            action = "BUTTON_PRESS"
            events += 1
        elif probe_str == "None" and last_probe != "None":
            action = "PROBE_RELEASE"
        elif button_str == "NONE" and last_button != "NONE":
            action = "BUTTON_RELEASE"
        
        # Print only if there's activity or every 10th cycle
        if action or cycle % 10 == 0:
            time_str = str(round(current_time, 1)) + "s"
            print("  " + time_str.rjust(4) + " | " + probe_str.rjust(9) + " | " + 
                  button_str.rjust(6) + " | " + action.ljust(12))
        
        # Update OLED
        if action:
            oled_clear()
            if "PROBE" in action:
                oled_print("Probe: " + probe_str)
            elif "BUTTON" in action:
                oled_print("Button: " + button_str)
        elif cycle % 10 == 0:
            oled_clear()
            oled_print("Monitoring...")
        
        # Store last values
        last_probe = probe_str if probe_str != "None" else None
        last_button = button_str if button_str != "NONE" else None
        
        time.sleep(0.1)
    
    print("\n  Monitoring summary:")
    print("    Total events detected: " + str(events))
    print("    Monitoring duration: " + str(round(time.time() - start_time, 1)) + "s")
    
    oled_clear()
    oled_print("Events: " + str(events))
    time.sleep(2)
    
    print("✓ Combined monitoring complete")

def probe_interactive_demo():
    """Interactive demonstration with user feedback"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           INTERACTIVE DEMO                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Interactive Demo")
    
    print("\n☺ Interactive probe and button demo:")
    print("  This demo responds to your touches and button presses")
    print("  Touch different pads and press buttons to see responses")
    print("  Demo will run for 15 seconds...")
    
    start_time = time.time()
    touch_count = 0
    button_count = 0
    unique_pads = set()
    
    print("\n  Activity Log:")
    print("  Time | Event")
    print("  -----|--------")
    
    last_display_time = 0
    
    while time.time() - start_time < 15:
        current_time = time.time() - start_time
        
        # Check for probe touch
        probe_pad = probe_read(False)
        if probe_pad and probe_pad != -1:
            try:
                if hasattr(probe_pad, 'value'):
                    pad_num = probe_pad.value
                else:
                    pad_num = int(str(probe_pad))
                
                if pad_num not in unique_pads:
                    unique_pads.add(pad_num)
                    touch_count += 1
                    
                    time_str = str(round(current_time, 1)) + "s"
                    print("  " + time_str.rjust(4) + " | Touched pad " + str(pad_num))
                    
                    oled_clear()
                    oled_print("Touch: " + str(pad_num))
                    last_display_time = current_time
                
            except (ValueError, AttributeError):
                pass
        
        # Check for button press
        button = check_button()
        if button != BUTTON_NONE and button != NONE:
            button_name = "CONNECT" if button == BUTTON_CONNECT or button == CONNECT else "REMOVE"
            button_count += 1
            
            time_str = str(round(current_time, 1)) + "s"
            print("  " + time_str.rjust(4) + " | Pressed " + button_name + " button")
            
            oled_clear()
            oled_print("Button: " + button_name)
            last_display_time = current_time
            
            time.sleep(0.5)  # Debounce
        
        # Update display periodically
        if current_time - last_display_time > 2:
            oled_clear()
            oled_print("Try touching...")
            last_display_time = current_time
        
        time.sleep(0.1)
    
    print("\n  Demo Results:")
    print("    Touch events: " + str(touch_count))
    print("    Button events: " + str(button_count))
    print("    Unique pads touched: " + str(len(unique_pads)))
    print("    Pads: " + str(sorted(list(unique_pads))))
    
    oled_clear()
    oled_print("Demo: " + str(touch_count) + " touches")
    time.sleep(2)
    
    print("✓ Interactive demo complete")

def probe_error_handling():
    """Demonstrate error handling and edge cases"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         ERROR HANDLING & EDGE CASES                         │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Error Handling")
    
    print("\n☺ Testing edge cases and error conditions:")
    
    # Test various error conditions
    error_tests = [
        ("Multiple rapid reads", lambda: [probe_read(False) for _ in range(5)]),
        ("Mixed blocking flags", lambda: probe_read(True) if probe_read(False) else None),
        ("Button state consistency", lambda: [check_button() for _ in range(3)]),
        ("Probe object handling", lambda: str(probe_read(False))),
    ]
    
    for test_name, test_func in error_tests:
        print("\n  " + test_name + ":")
        try:
            result = test_func()
            print("    Result: " + str(result))
            print("    Status: OK")
        except Exception as e:
            print("    Error: " + str(e))
            print("    Status: Error handled")
        
        oled_clear()
        oled_print("Test: " + test_name[:12])
        time.sleep(1)
    
    print("\n☺ Testing probe object attributes:")
    
    # Test probe object handling
    for attempt in range(5):
        pad = probe_read(False)
        print("  Attempt " + str(attempt + 1) + ":")
        
        if pad and pad != -1:
            print("    Raw value: " + str(pad))
            print("    Type: " + str(type(pad)))
            
            # Test different ways to access the value
            try:
                if hasattr(pad, 'value'):
                    print("    .value attribute: " + str(pad.value))
                else:
                    print("    No .value attribute")
            except:
                print("    Error accessing .value")
            
            try:
                str_val = str(pad)
                print("    str() conversion: " + str_val)
                int_val = int(str_val)
                print("    int(str()) conversion: " + str(int_val))
            except:
                print("    Error in string/int conversion")
            
            break
        else:
            print("    No touch detected")
        
        time.sleep(0.2)
    
    print("✓ Error handling complete")

def run_all_probe_demos():
    """Run all probe and button demonstration functions"""
    
    print("🚀 Starting Complete Probe & Button Reference Demonstration")
    print("═" * 75)
    
    demos = [
        ("Basic Probe Operations", probe_basic_operations),
        ("Blocking Probe Operations", probe_blocking_operations),
        ("Special Probe Pads", probe_special_pads),
        ("Basic Button Operations", button_basic_operations),
        ("Blocking Button Operations", button_blocking_operations),
        ("Combined Monitoring", probe_button_combined),
        ("Interactive Demo", probe_interactive_demo),
        ("Error Handling", probe_error_handling)
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
    oled_print("Probe Reference Complete!")
    print("\n🎉 All probe and button demonstrations complete!")
    print("═" * 75)

def probe_quick_test():
    """Quick test of probe and button functions"""
    
    print("⚡ Quick Probe & Button Test")
    print("─" * 30)
    
    # Quick non-blocking tests
    pad = probe_read(False)
    button = check_button()
    
    print("probe_read(False): " + str(pad))
    print("check_button(): " + str(button))
    
    # Test special constants
    print("LOGO_PAD_TOP: " + str(LOGO_PAD_TOP))
    print("BUTTON_CONNECT: " + str(BUTTON_CONNECT))
    
    print("✓ Quick probe test complete")

# Run the demonstration
if __name__ == "__main__":
    run_all_probe_demos() 