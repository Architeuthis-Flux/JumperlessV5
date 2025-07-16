#!/usr/bin/env python3
"""
Test example for GPIO state string support in Jumperless MicroPython

This demonstrates the enhanced GPIO state type that now supports:
- Boolean values: True/False
- Integer values: 0 (LOW), 1 (HIGH), 2 (FLOATING)  
- String values: "HIGH", "LOW", "FLOATING" (case insensitive)
"""

import jumperless as jl

def test_gpio_state_strings():
    print("Testing GPIO State String Support")
    print("================================")
    
    # Test all string variations
    test_values = [
        # String values (various cases)
        "HIGH", "high", "High",
        "LOW", "low", "Low", 
        "FLOATING", "floating", "Floating",
        "FLOAT", "float", "Float",
        
        # String numbers
        "0", "1", "2",
        
        # Integer values
        0, 1, 2,
        
        # Boolean values
        True, False,
        
        # Module constants
        jl.HIGH, jl.LOW, jl.FLOATING
    ]
    
    for value in test_values:
        try:
            # Set GPIO pin using various input types (tests the enhanced make_new function)
            jl.gpio_set(1, value)  
            
            # Read back the state to see what was set
            state = jl.gpio_get(1)
            
            # Show what we get back
            print("Input: {:<12} -> Set successfully, Read: {}, int(): {}, bool(): {}".format(
                repr(value), state, int(state), bool(state)
            ))
            
        except Exception as e:
            print("Input: {:<12} -> ERROR: {}".format(repr(value), e))

def test_gpio_read_floating():
    print("\nTesting GPIO Read with FLOATING detection")
    print("========================================")
    
    # Test reading different GPIO pins to see if we can detect floating states
    for pin in range(1, 5):  # Test first 4 GPIO pins
        try:
            state = jl.gpio_get(pin)
            print("GPIO{}: {} (int: {}, bool: {})".format(
                pin, state, int(state), bool(state)
            ))
        except Exception as e:
            print("GPIO{}: ERROR - {}".format(pin, e))

def test_string_in_conditions():
    print("\nTesting GPIO states in conditional statements")
    print("===========================================")
    
    # Test how the states behave in conditionals
    states = [jl.HIGH, jl.LOW, jl.FLOATING]
    
    for state in states:
        print("State: {:<8} -> ".format(str(state)), end="")
        
        if state:
            print("Truthy (acts like HIGH)")
        else:
            print("Falsy (acts like LOW or FLOATING)")

if __name__ == "__main__":
    test_gpio_state_strings()
    test_gpio_read_floating()
    test_string_in_conditions()
    
    print("\n✅ GPIO state string support test complete!") 