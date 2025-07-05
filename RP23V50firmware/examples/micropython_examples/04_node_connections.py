"""
Node Connections Basics
=======================

This example demonstrates node connection operations using the jumperless module:
- Connecting breadboard holes to special nodes
- Using different node specification methods
- Power rail connections
- Testing connections with DAC/ADC
- Connection verification and cleanup

Functions demonstrated:
- connect(node1, node2) - Connect two nodes
- disconnect(node1, node2) - Disconnect two nodes
- is_connected(node1, node2) - Check if nodes are connected
- nodes_clear() - Clear all connections
- oled_print(text) - Display text on OLED

Node Types:
- Numbers: 1-60 (breadboard holes)
- Arduino pins: D0-D13, A0-A7
- Special nodes: TOP_RAIL, BOTTOM_RAIL, GND, DAC0, DAC1, ADC0-ADC3
- GPIO: GPIO_1-GPIO_8
- Can be specified as numbers, strings, or constants
"""

import time

def basic_connections_demo():
    """Demonstrate basic node connections"""
    
    oled_clear()
    oled_print("Basic Connections")
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           BASIC CONNECTIONS                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Basic breadboard connections
    connections = [
        (1, 30, "holes 1-30"),
        (5, 25, "holes 5-25"),
        (10, 40, "holes 10-40"),
        (15, 45, "holes 15-45")
    ]
    
    print("\n☺ Making basic breadboard connections:")
    
    for node1, node2, description in connections:
        result = connect(node1, node2)
        
        oled_clear()
        oled_print("Connect " + str(node1) + "-" + str(node2))
        print("  Connecting " + description + ": " + result)
        
        # Verify connection
        connected = is_connected(node1, node2)
        print("    Verification: " + connected)
        
        time.sleep(2)
    
    print("\n✓ Basic connections complete")

def special_node_connections():
    """Demonstrate connections to special nodes"""
    
    oled_clear()
    oled_print("Special Nodes")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           SPECIAL NODES                                     │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Connections to special nodes
    special_connections = [
        (1, TOP_RAIL, "hole 1 to TOP_RAIL"),
        (2, BOTTOM_RAIL, "hole 2 to BOTTOM_RAIL"),
        (3, GND, "hole 3 to GND"),
        (4, DAC0, "hole 4 to DAC0"),
        (5, DAC1, "hole 5 to DAC1"),
        (6, ADC0, "hole 6 to ADC0")
    ]
    
    print("\n☺ Connecting to special nodes:")
    
    for node1, node2, description in special_connections:
        result = connect(node1, node2)
        
        oled_clear()
        oled_print("Connect " + description.split()[0] + " " + description.split()[2])
        print("  " + description + ": " + result)
        
        # Verify connection
        connected = is_connected(node1, node2)
        print("    Verification: " + connected)
        
        time.sleep(2)
    
    print("\n✓ Special node connections complete")

def string_node_connections():
    """Demonstrate using string node names"""
    
    oled_clear()
    oled_print("String Nodes")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            STRING NODES                                     │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Using string node names (case-insensitive)
    string_connections = [
        ("D13", "top_rail", "D13 to top rail"),
        ("A0", "adc0", "A0 to ADC0"),
        ("gpio_1", "15", "GPIO_1 to hole 15"),
        ("dac1", "20", "DAC1 to hole 20"),
        ("bottom_rail", "25", "bottom rail to hole 25")
    ]
    
    print("\n☺ Using string node names:")
    
    for node1, node2, description in string_connections:
        result = connect(node1, node2)
        
        oled_clear()
        oled_print("Connect " + str(node1) + "-" + str(node2))
        print("  " + description + ": " + result)
        
        # Verify connection
        connected = is_connected(node1, node2)
        print("    Verification: " + connected)
        
        time.sleep(2)
    
    print("\n✓ String node connections complete")

def power_rail_setup():
    """Set up power rails and connect devices"""
    
    oled_clear()
    oled_print("Power Rail Setup")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           POWER RAIL SETUP                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Set up power rails
    print("\n☺ Setting up power rails:")
    
    # Set TOP_RAIL to 5V
    dac_set(TOP_RAIL, 5.0)
    rail_voltage = dac_get(TOP_RAIL)
    print("  TOP_RAIL voltage: " + str(rail_voltage) + "V")
    
    # Set BOTTOM_RAIL to 0V (ground)
    dac_set(BOTTOM_RAIL, 0.0)
    rail_voltage = dac_get(BOTTOM_RAIL)
    print("  BOTTOM_RAIL voltage: " + str(rail_voltage) + "V")
    
    oled_clear()
    oled_print("Rails: 5V/0V")
    time.sleep(2)
    
    # Connect devices to power rails
    power_connections = [
        (TOP_RAIL, 1, "5V to hole 1"),
        (TOP_RAIL, 11, "5V to hole 11"),
        (TOP_RAIL, 21, "5V to hole 21"),
        (BOTTOM_RAIL, 2, "GND to hole 2"),
        (BOTTOM_RAIL, 12, "GND to hole 12"),
        (BOTTOM_RAIL, 22, "GND to hole 22")
    ]
    
    print("\n☺ Connecting devices to power rails:")
    
    for rail, hole, description in power_connections:
        result = connect(rail, hole)
        
        oled_clear()
        oled_print(description)
        print("  " + description + ": " + result)
        
        time.sleep(1.5)
    
    print("\n✓ Power rail setup complete")

def dac_adc_connection_test():
    """Test DAC to ADC connections through breadboard"""
    
    oled_clear()
    oled_print("DAC-ADC Test")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            DAC-ADC TEST                                     │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Connect DAC0 to breadboard hole 10
    result = connect(DAC0, 10)
    print("\n☺ Connected DAC0 to hole 10: " + result)
    
    # Connect breadboard hole 10 to ADC0
    result = connect(10, ADC0)
    print("☺ Connected hole 10 to ADC0: " + result)
    
    oled_clear()
    oled_print("DAC0-10-ADC0")
    time.sleep(2)
    
    # Test different DAC voltages and measure with ADC
    test_voltages = [0.0, 1.0, 2.0, 3.0, 2.5, 1.5, 0.5]
    
    print("\n☺ Testing DAC to ADC signal path:")
    
    for voltage in test_voltages:
        # Set DAC voltage
        dac_set(DAC0, voltage)
        dac_readback = dac_get(DAC0)
        
        # Read ADC voltage
        time.sleep(0.1)  # Allow signal to settle
        adc_reading = adc_get(0)
        
        oled_clear()
        oled_print("DAC:" + str(round(dac_readback, 2)) + "V ADC:" + str(round(adc_reading, 2)) + "V")
        print("  DAC0 set: " + str(voltage) + "V, reads: " + str(round(dac_readback, 3)) + "V")
        print("  ADC0 reads: " + str(round(adc_reading, 3)) + "V")
        
        time.sleep(2)
    
    print("\n✓ DAC-ADC connection test complete")

def gpio_breadboard_connections():
    """Connect GPIO pins to breadboard and control them"""
    
    oled_clear()
    oled_print("GPIO Connections")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           GPIO CONNECTIONS                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Connect GPIO pins to breadboard holes
    gpio_connections = [
        (GPIO_1, 31, "GPIO_1 to hole 31"),
        (GPIO_2, 32, "GPIO_2 to hole 32"),
        (GPIO_3, 33, "GPIO_3 to hole 33"),
        (GPIO_4, 34, "GPIO_4 to hole 34")
    ]
    
    print("\n☺ Connecting GPIO pins to breadboard:")
    
    for gpio, hole, description in gpio_connections:
        result = connect(gpio, hole)
        
        oled_clear()
        oled_print(description)
        print("  " + description + ": " + result)
        
        time.sleep(1.5)
    
    # Set up GPIO pins as outputs
    print("\n☺ Setting up GPIO pins as outputs:")
    
    for i in range(1, 5):
        gpio_set_dir(i, True)  # Set as output
        direction = gpio_get_dir(i)
        print("  GPIO" + str(i) + " direction: " + direction)
    
    # Toggle GPIO pins and show on breadboard
    print("\n☺ Toggling GPIO pins connected to breadboard:")
    
    for cycle in range(3):
        print("  Cycle " + str(cycle + 1) + "/3:")
        
        for i in range(1, 5):
            # Turn on current GPIO
            gpio_set(i, True)
            state = gpio_get(i)
            
            oled_clear()
            oled_print("GPIO" + str(i) + " ON (hole " + str(30 + i) + ")")
            print("    GPIO" + str(i) + " " + state + " -> hole " + str(30 + i))
            
            time.sleep(0.5)
            
            # Turn off current GPIO
            gpio_set(i, False)
            state = gpio_get(i)
            
            oled_clear()
            oled_print("GPIO" + str(i) + " OFF")
            
            time.sleep(0.5)
    
    print("\n✓ GPIO breadboard connections complete")

def mixed_node_types_demo():
    """Demonstrate mixing different node specification methods"""
    
    oled_clear()
    oled_print("Mixed Node Types")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           MIXED NODE TYPES                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Mix numbers, strings, and constants
    mixed_connections = [
        (1, "D13", "hole 1 to D13 (number to string)"),
        (TOP_RAIL, "15", "TOP_RAIL to hole 15 (constant to string)"),
        (GPIO_1, 16, "GPIO_1 to hole 16 (constant to number)"),
        ("adc0", ADC1, "ADC0 to ADC1 (string to constant)"),
        ("25", DAC0, "hole 25 to DAC0 (string to constant)")
    ]
    
    print("\n☺ Mixing different node specification methods:")
    
    for node1, node2, description in mixed_connections:
        result = connect(node1, node2)
        
        oled_clear()
        oled_print("Mixed: " + str(node1) + "-" + str(node2))
        print("  " + description + ": " + result)
        
        # Verify connection
        connected = is_connected(node1, node2)
        print("    Verification: " + connected)
        
        time.sleep(2)
    
    print("\n✓ Mixed node types demo complete")

def connection_cleanup_demo():
    """Demonstrate disconnecting and clearing connections"""
    
    oled_clear()
    oled_print("Connection Cleanup")
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         CONNECTION CLEANUP                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    # Make some connections first
    print("\n☺ Making test connections:")
    
    test_connections = [
        (1, 10),
        (2, 20),
        (3, 30),
        (TOP_RAIL, 5),
        (DAC0, 15)
    ]
    
    for node1, node2 in test_connections:
        result = connect(node1, node2)
        print("  Connected " + str(node1) + " to " + str(node2) + ": " + result)
    
    oled_clear()
    oled_print("Test connections made")
    time.sleep(2)
    
    # Disconnect specific connections
    print("\n☺ Disconnecting specific connections:")
    
    for node1, node2 in test_connections[:3]:  # Disconnect first 3
        result = disconnect(node1, node2)
        
        oled_clear()
        oled_print("Disconnect " + str(node1) + "-" + str(node2))
        print("  Disconnected " + str(node1) + " from " + str(node2) + ": " + result)
        
        # Verify disconnection
        connected = is_connected(node1, node2)
        print("    Verification: " + connected)
        
        time.sleep(1.5)
    
    # Clear all remaining connections
    print("\n☺ Clearing all remaining connections:")
    
    result = nodes_clear()
    print("  Clear all connections: " + result)
    
    oled_clear()
    oled_print("All connections cleared")
    time.sleep(2)
    
    # Verify all connections are cleared
    print("\n☺ Verifying all connections cleared:")
    
    for node1, node2 in test_connections:
        connected = is_connected(node1, node2)
        print("  " + str(node1) + " to " + str(node2) + ": " + connected)
    
    print("\n✓ Connection cleanup complete")

def run_all_connection_demos():
    """Run all node connection demonstration functions"""
    
    print("🚀 Starting Complete Node Connection Demonstration")
    print("═" * 75)
    
    basic_connections_demo()
    special_node_connections()
    string_node_connections()
    power_rail_setup()
    dac_adc_connection_test()
    gpio_breadboard_connections()
    mixed_node_types_demo()
    connection_cleanup_demo()
    
    oled_clear()
    oled_print("Connection Demo Complete!")
    print("\n🎉 All node connection demonstrations complete!")
    print("═" * 75)

# Run the demonstration
if __name__ == "__main__":
    run_all_connection_demos() 