"""
Node Connections Reference
=========================

Complete reference for all connection functions in the Jumperless MicroPython module.
This file demonstrates every connection-related function with practical examples.

Functions demonstrated:
- connect(node1, node2) - Connect two nodes
- disconnect(node1, node2) - Disconnect two nodes  
- is_connected(node1, node2) - Check if nodes are connected
- nodes_clear() - Clear all connections
- node(name_or_number) - Create node from string/number

Node Types:
- Numbers: 1-60 (breadboard holes)
- Arduino pins: D0-D13, A0-A7
- GPIO: GPIO_1-GPIO_8
- Power: TOP_RAIL, BOTTOM_RAIL, GND
- DAC: DAC0, DAC1
- ADC: ADC0-ADC4
- Special: UART_TX, UART_RX, BUFFER_IN, BUFFER_OUT

Usage:
  exec(open('micropython_examples/connections_reference.py').read())
"""

import time

def connections_basic_operations():
    """Demonstrate basic connection operations"""
    
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                        BASIC CONNECTION OPERATIONS                          │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Basic Connections")
    
    # Clear any existing connections
    nodes_clear()
    print("☺ Cleared all existing connections")
    
    print("\n☺ Basic breadboard connections:")
    
    # Simple breadboard hole connections
    basic_connections = [
        (1, 30, "holes 1-30"),
        (5, 25, "holes 5-25"),
        (10, 40, "holes 10-40"),
        (15, 35, "holes 15-35")
    ]
    
    for node1, node2, description in basic_connections:
        result = connect(node1, node2)
        print("  connect(" + str(node1) + ", " + str(node2) + ") - " + description + ": " + str(result))
        
        # Verify connection
        connected = is_connected(node1, node2)
        print("    Verification: " + str(connected))
        
        oled_clear()
        oled_print("Connect " + str(node1) + "-" + str(node2))
        time.sleep(1)
    
    print("\n☺ Checking all connections:")
    for node1, node2, description in basic_connections:
        connected = is_connected(node1, node2)
        print("  " + description + ": " + str(connected))
    
    print("✓ Basic connection operations complete")

def connections_different_node_types():
    """Demonstrate connections with different node types"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                       DIFFERENT NODE TYPE CONNECTIONS                       │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Node Types Demo")
    
    print("\n☺ Using different ways to specify nodes:")
    
    # Different node specification methods
    node_connections = [
        (1, 30, "Numbers: breadboard holes"),
        ("D13", "TOP_RAIL", "Strings: Arduino pin to power rail"),
        (D13, BOTTOM_RAIL, "Constants: Arduino pin to power rail"),
        (GPIO_1, A0, "Constants: GPIO to analog pin"),
        (DAC0, 15, "Mixed: DAC constant to number"),
        ("ADC0", 20, "Mixed: ADC string to number"),
        (TOP_RAIL, 25, "Constants: Power rail to breadboard"),
        ("BOTTOM_RAIL", 35, "String: Power rail to breadboard")
    ]
    
    for node1, node2, description in node_connections:
        result = connect(node1, node2)
        print("  " + description + ": " + str(result))
        
        # Show connection on OLED
        oled_clear()
        oled_print(str(node1) + " -> " + str(node2))
        time.sleep(1)
    
    print("\n☺ Verifying all connections:")
    for node1, node2, description in node_connections:
        connected = is_connected(node1, node2)
        print("  " + description + ": " + str(connected))
    
    print("✓ Different node type connections complete")

def connections_power_distribution():
    """Demonstrate power distribution connections"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                        POWER DISTRIBUTION SETUP                             │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Power Distribution")
    
    print("\n☺ Setting up power distribution network:")
    
    # Set up power rail voltages first
    power_configs = [
        (TOP_RAIL, 3.3, "3.3V Logic"),
        (BOTTOM_RAIL, 5.0, "5V Power"),
        (DAC0, 1.8, "1.8V Logic"),
        (DAC1, 2.5, "2.5V Reference")
    ]
    
    print("  Setting power rail voltages:")
    for rail, voltage, description in power_configs:
        dac_set(rail, voltage)
        actual = dac_get(rail)
        print("    " + description + ": " + str(actual) + "V")
    
    # Connect power rails to breadboard distribution points
    power_connections = [
        (TOP_RAIL, 10, "3.3V to hole 10"),
        (TOP_RAIL, 11, "3.3V to hole 11"), 
        (BOTTOM_RAIL, 20, "5V to hole 20"),
        (BOTTOM_RAIL, 21, "5V to hole 21"),
        (DAC0, 30, "1.8V to hole 30"),
        (DAC1, 40, "2.5V to hole 40"),
        (GND, 50, "GND to hole 50"),
        (GND, 51, "GND to hole 51"),
        (GND, 52, "GND to hole 52")
    ]
    
    print("\n  Connecting power rails to breadboard:")
    for rail, hole, description in power_connections:
        result = connect(rail, hole)
        print("    " + description + ": " + str(result))
        
        oled_clear()
        oled_print(description)
        time.sleep(0.8)
    
    # Create power distribution buses
    print("\n  Creating power buses on breadboard:")
    
    # 3.3V bus (connect multiple holes together)
    v33_bus = [10, 11, 12, 13]
    for i in range(len(v33_bus) - 1):
        connect(v33_bus[i], v33_bus[i + 1])
        print("    3.3V bus: hole " + str(v33_bus[i]) + " to hole " + str(v33_bus[i + 1]))
    
    # GND bus (connect multiple holes together) 
    gnd_bus = [50, 51, 52, 53, 54]
    for i in range(len(gnd_bus) - 1):
        connect(gnd_bus[i], gnd_bus[i + 1])
        print("    GND bus: hole " + str(gnd_bus[i]) + " to hole " + str(gnd_bus[i + 1]))
    
    print("\n☺ Power distribution network ready!")
    print("  Available voltages:")
    print("    Holes 10-13: 3.3V")
    print("    Holes 20-21: 5.0V")
    print("    Hole 30: 1.8V")
    print("    Hole 40: 2.5V")
    print("    Holes 50-54: GND")
    
    oled_clear()
    oled_print("Power Ready!")
    time.sleep(2)
    
    print("✓ Power distribution setup complete")

def connections_circuit_examples():
    """Demonstrate building complete circuits"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           CIRCUIT BUILDING EXAMPLES                         │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Circuit Examples")
    
    print("\n☺ Example 1: LED Control Circuit")
    print("  Building: GPIO1 -> LED -> Resistor -> GND")
    
    # Clear previous connections for clean start
    nodes_clear()
    
    # LED circuit connections
    led_connections = [
        (GPIO_1, 15, "GPIO1 to LED positive (hole 15)"),
        (16, 17, "LED negative to resistor (holes 16-17)"),
        (17, GND, "Resistor to GND"),
    ]
    
    for node1, node2, description in led_connections:
        result = connect(node1, node2)
        print("    " + description + ": " + str(result))
    
    # Configure and test the LED
    print("  Configuring GPIO1 as output and testing LED:")
    gpio_set_dir(1, True)
    
    # Blink the LED
    for blink in range(5):
        gpio_set(1, True)
        oled_clear()
        oled_print("LED ON")
        time.sleep(0.5)
        
        gpio_set(1, False)
        oled_clear()
        oled_print("LED OFF")
        time.sleep(0.5)
    
    print("\n☺ Example 2: Voltage Divider Circuit")
    print("  Building: TOP_RAIL -> R1 -> ADC0 -> R2 -> GND")
    
    # Set up power rail
    dac_set(TOP_RAIL, 3.3)
    
    voltage_divider_connections = [
        (TOP_RAIL, 25, "3.3V to voltage divider top"),
        (26, ADC0, "Voltage divider middle to ADC0"),
        (27, GND, "Voltage divider bottom to GND")
    ]
    
    for node1, node2, description in voltage_divider_connections:
        result = connect(node1, node2)
        print("    " + description + ": " + str(result))
    
    print("  Note: Connect external resistors between holes 25-26 and 26-27")
    print("  Monitoring voltage divider output:")
    
    for reading in range(5):
        voltage = adc_get(0)
        print("    ADC0 reading " + str(reading + 1) + ": " + str(voltage) + "V")
        
        oled_clear()
        oled_print("ADC0: " + str(voltage) + "V")
        time.sleep(1)
    
    print("\n☺ Example 3: Current Monitoring Circuit")
    print("  Building: Power -> Load -> Current Sensor -> GND")
    
    current_monitor_connections = [
        (DAC0, 35, "DAC0 power to load input"),
        (36, ISENSE_PLUS, "Load output to current sensor +"),
        (ISENSE_MINUS, GND, "Current sensor - to GND")
    ]
    
    # Set up test voltage
    dac_set(DAC0, 2.5)
    
    for node1, node2, description in current_monitor_connections:
        result = connect(node1, node2)
        print("    " + description + ": " + str(result))
    
    print("  Note: Connect load between holes 35-36")
    print("  Monitoring current consumption:")
    
    for reading in range(5):
        current = get_current(0)
        voltage = get_bus_voltage(0)
        power = get_power(0)
        
        print("    Reading " + str(reading + 1) + ": " + str(current * 1000) + "mA, " + str(voltage) + "V, " + str(power * 1000) + "mW")
        
        oled_clear()
        oled_print(str(current * 1000) + "mA")
        time.sleep(1)
    
    print("✓ Circuit examples complete")

def connections_node_creation():
    """Demonstrate node creation and manipulation"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                          NODE CREATION & MANIPULATION                       │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Node Creation")
    
    print("\n☺ Creating nodes from different inputs:")
    
    # Test node creation function
    test_inputs = [
        ("D13", "Arduino pin from string"),
        (13, "Breadboard hole from number"),
        ("TOP_RAIL", "Power rail from string"),
        ("GPIO_1", "GPIO from string"),
        ("ADC0", "ADC from string")
    ]
    
    created_nodes = []
    
    for input_val, description in test_inputs:
        try:
            my_node = node(input_val)
            created_nodes.append((my_node, description))
            print("  node(" + str(input_val) + ") -> " + str(my_node) + " (" + description + ")")
        except Exception as e:
            print("  node(" + str(input_val) + ") -> Error: " + str(e))
    
    print("\n☺ Using created nodes in connections:")
    
    # Use created nodes in connections
    for i in range(len(created_nodes) - 1):
        node1, desc1 = created_nodes[i]
        node2, desc2 = created_nodes[i + 1]
        
        result = connect(node1, node2)
        print("  connect(" + str(node1) + ", " + str(node2) + "): " + str(result))
        
        # Verify the connection
        connected = is_connected(node1, node2)
        print("    Verified: " + str(connected))
        
        oled_clear()
        oled_print(str(node1) + " <-> " + str(node2))
        time.sleep(1)
    
    print("\n☺ Node comparison and equality:")
    
    # Test node equality
    d13_string = node("D13")
    d13_constant = D13
    
    print("  node(\"D13\") == D13: " + str(d13_string == d13_constant))
    print("  node(\"D13\"): " + str(d13_string))
    print("  D13 constant: " + str(d13_constant))
    
    # Test with numbers
    hole_13 = node(13)
    print("  node(13): " + str(hole_13))
    print("  node(13) == 13: " + str(hole_13 == 13))
    
    print("✓ Node creation complete")

def connections_disconnection():
    """Demonstrate disconnection operations"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            DISCONNECTION OPERATIONS                         │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Disconnection Demo")
    
    print("\n☺ Setting up connections to disconnect:")
    
    # Create several connections
    test_connections = [
        (1, 30),
        (5, 35),
        (GPIO_1, A0),
        (TOP_RAIL, 10),
        (DAC0, 15)
    ]
    
    for node1, node2 in test_connections:
        result = connect(node1, node2)
        print("  Connected " + str(node1) + " to " + str(node2) + ": " + str(result))
    
    print("\n☺ Verifying all connections exist:")
    for node1, node2 in test_connections:
        connected = is_connected(node1, node2)
        print("  " + str(node1) + " <-> " + str(node2) + ": " + str(connected))
    
    print("\n☺ Disconnecting specific connections:")
    
    # Disconnect some connections
    disconnections = [
        (1, 30),
        (GPIO_1, A0),
        (DAC0, 15)
    ]
    
    for node1, node2 in disconnections:
        result = disconnect(node1, node2)
        print("  disconnect(" + str(node1) + ", " + str(node2) + "): " + str(result))
        
        # Verify disconnection
        connected = is_connected(node1, node2)
        print("    Now connected: " + str(connected))
        
        oled_clear()
        oled_print("Disconnect " + str(node1) + "-" + str(node2))
        time.sleep(1)
    
    print("\n☺ Checking remaining connections:")
    for node1, node2 in test_connections:
        connected = is_connected(node1, node2)
        status = "CONNECTED" if connected == "CONNECTED" else "DISCONNECTED"
        print("  " + str(node1) + " <-> " + str(node2) + ": " + status)
    
    print("\n☺ Clearing all remaining connections:")
    result = nodes_clear()
    print("  nodes_clear(): " + str(result))
    
    print("\n☺ Verifying all connections cleared:")
    for node1, node2 in test_connections:
        connected = is_connected(node1, node2)
        print("  " + str(node1) + " <-> " + str(node2) + ": " + str(connected))
    
    oled_clear()
    oled_print("All Disconnected")
    time.sleep(1)
    
    print("✓ Disconnection operations complete")

def connections_complex_networks():
    """Demonstrate complex network topologies"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                          COMPLEX NETWORK TOPOLOGIES                         │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Complex Networks")
    
    print("\n☺ Building star network topology:")
    print("  Center: hole 25, connected to holes 1, 5, 10, 15, 20")
    
    # Star topology - one central node connected to many
    center = 25
    star_nodes = [1, 5, 10, 15, 20]
    
    for node in star_nodes:
        result = connect(center, node)
        print("    connect(" + str(center) + ", " + str(node) + "): " + str(result))
    
    # Verify star connections
    print("  Verifying star topology:")
    for node in star_nodes:
        connected = is_connected(center, node)
        print("    " + str(center) + " <-> " + str(node) + ": " + str(connected))
    
    oled_clear()
    oled_print("Star Network Built")
    time.sleep(2)
    
    print("\n☺ Building linear chain topology:")
    print("  Chain: 30 -> 31 -> 32 -> 33 -> 34 -> 35")
    
    # Linear chain topology
    chain_nodes = [30, 31, 32, 33, 34, 35]
    
    for i in range(len(chain_nodes) - 1):
        result = connect(chain_nodes[i], chain_nodes[i + 1])
        print("    connect(" + str(chain_nodes[i]) + ", " + str(chain_nodes[i + 1]) + "): " + str(result))
    
    # Verify chain connections
    print("  Verifying chain topology:")
    for i in range(len(chain_nodes) - 1):
        connected = is_connected(chain_nodes[i], chain_nodes[i + 1])
        print("    " + str(chain_nodes[i]) + " <-> " + str(chain_nodes[i + 1]) + ": " + str(connected))
    
    oled_clear()
    oled_print("Chain Network Built")
    time.sleep(2)
    
    print("\n☺ Building bus topology:")
    print("  Bus: All GPIO pins connected together")
    
    # Bus topology - all nodes connected to each other
    gpio_pins = [GPIO_1, GPIO_2, GPIO_3, GPIO_4]
    
    for i in range(len(gpio_pins)):
        for j in range(i + 1, len(gpio_pins)):
            result = connect(gpio_pins[i], gpio_pins[j])
            print("    connect(" + str(gpio_pins[i]) + ", " + str(gpio_pins[j]) + "): " + str(result))
    
    oled_clear()
    oled_print("Bus Network Built")
    time.sleep(2)
    
    print("\n☺ Building mixed signal/power network:")
    print("  Connecting signals with power distribution")
    
    # Mixed network - signals + power
    mixed_connections = [
        (TOP_RAIL, 40, "3.3V power"),
        (BOTTOM_RAIL, 41, "5V power"),
        (GND, 42, "Ground"),
        (DAC0, 43, "Analog out"),
        (ADC0, 44, "Analog in"),
        (GPIO_1, 45, "Digital I/O"),
        (D13, 46, "Arduino pin")
    ]
    
    # Set up power first
    dac_set(TOP_RAIL, 3.3)
    dac_set(BOTTOM_RAIL, 5.0)
    
    for node, hole, description in mixed_connections:
        result = connect(node, hole)
        print("    " + description + ": " + str(result))
    
    oled_clear()
    oled_print("Mixed Network Built")
    time.sleep(2)
    
    print("\n☺ Network analysis:")
    print("  Total connections created: " + str(len(star_nodes) + len(chain_nodes) - 1 + len(gpio_pins) * (len(gpio_pins) - 1) // 2 + len(mixed_connections)))
    
    # Show current bridge status
    print("  Current bridge status:")
    print_bridges()
    
    print("✓ Complex network topologies complete")

def run_all_connection_demos():
    """Run all connection demonstration functions"""
    
    print("🚀 Starting Complete Connections Reference Demonstration")
    print("═" * 75)
    
    demos = [
        ("Basic Operations", connections_basic_operations),
        ("Different Node Types", connections_different_node_types),
        ("Power Distribution", connections_power_distribution),
        ("Circuit Examples", connections_circuit_examples),
        ("Node Creation", connections_node_creation),
        ("Disconnection", connections_disconnection),
        ("Complex Networks", connections_complex_networks)
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
    
    # Final cleanup
    nodes_clear()
    
    oled_clear()
    oled_print("Connections Reference Complete!")
    print("\n🎉 All connection demonstrations complete!")
    print("═" * 75)

def connections_quick_test():
    """Quick test of connection functions"""
    
    print("⚡ Quick Connections Test")
    print("─" * 30)
    
    # Clear and test basic connection
    nodes_clear()
    
    result = connect(1, 30)
    print("connect(1, 30): " + str(result))
    
    connected = is_connected(1, 30)
    print("is_connected(1, 30): " + str(connected))
    
    result = disconnect(1, 30)
    print("disconnect(1, 30): " + str(result))
    
    connected = is_connected(1, 30)
    print("is_connected(1, 30): " + str(connected))
    
    print("✓ Quick connections test complete")

# Run the demonstration
if __name__ == "__main__":
    run_all_connection_demos() 