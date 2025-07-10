"""
Jumperless MicroPython Module - Complete Reference
=================================================

This file demonstrates every available function in the Jumperless MicroPython module.
It serves as a comprehensive reference and test suite for all functionality.

IMPORTANT: This file contains ALL functions for reference purposes.
For device-specific examples, see the individual category files:
- dac_reference.py - DAC (Digital-to-Analog) functions
- adc_reference.py - ADC (Analog-to-Digital) functions  
- gpio_reference.py - GPIO pin control functions
- ina_reference.py - Current/power monitoring functions
- connections_reference.py - Node connection functions
- oled_reference.py - Display functions
- probe_reference.py - Probe and button functions
- system_reference.py - System and status functions

Usage:
  exec(open('examples/jumperless_complete_reference.py').read())
"""

import time

def comprehensive_demo():
    """Run a comprehensive demonstration of all Jumperless functions"""
    
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                    JUMPERLESS COMPREHENSIVE REFERENCE                       │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Starting Demo...")
    time.sleep(2)
    
    # Run all function category demos
    demo_dac_functions()
    demo_adc_functions()
    demo_gpio_functions()
    demo_ina_functions()
    demo_connection_functions()
    demo_oled_functions()
    demo_probe_functions()
    demo_system_functions()
    demo_advanced_examples()
    
    print("\n🎉 Comprehensive demo complete!")
    oled_clear()
    oled_print("Demo Complete!")

def demo_dac_functions():
    """Demonstrate DAC (Digital-to-Analog Converter) functions"""
    
    print("\n" + "="*75)
    print("DAC (DIGITAL-TO-ANALOG CONVERTER) FUNCTIONS")
    print("="*75)
    print("DAC channels: 0-3, DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL")
    print("Voltage range: -8.0V to +8.0V")
    
    oled_clear()
    oled_print("DAC Demo")
    
    # Basic DAC operations
    print("\n📍 Basic DAC Operations:")
    
    # Set DAC outputs using different methods
    dac_set(DAC0, 1.5)          # Using node constant
    print("  dac_set(DAC0, 1.5) - Set DAC0 to 1.5V")
    
    set_dac(DAC1, 2.5)          # Using alias function
    print("  set_dac(DAC1, 2.5) - Set DAC1 to 2.5V (alias)")
    
    dac_set(TOP_RAIL, 3.3)      # Set power rail voltage
    print("  dac_set(TOP_RAIL, 3.3) - Set top rail to 3.3V")
    
    dac_set(BOTTOM_RAIL, 0.0)   # Set bottom rail to ground
    print("  dac_set(BOTTOM_RAIL, 0.0) - Set bottom rail to 0V")
    
    # Alternative syntax using channel numbers
    dac_set(0, 1.0)             # Channel 0 = DAC0
    print("  dac_set(0, 1.0) - Set channel 0 (DAC0) to 1.0V")
    
    time.sleep(1)
    
    # Read DAC values back
    print("\n📖 Reading DAC Values:")
    voltage0 = dac_get(DAC0)
    voltage1 = get_dac(DAC1)    # Using alias
    rail_top = dac_get(TOP_RAIL)
    rail_bot = dac_get(BOTTOM_RAIL)
    
    print(f"  DAC0 output: {voltage0:.3f}V")
    print(f"  DAC1 output: {voltage1:.3f}V") 
    print(f"  Top rail: {rail_top:.3f}V")
    print(f"  Bottom rail: {rail_bot:.3f}V")
    
    # Demonstrate voltage sweep
    print("\n🌊 DAC Voltage Sweep:")
    print("  Sweeping DAC0 from 0V to 3V...")
    for v in [0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0]:
        dac_set(DAC0, v)
        actual = dac_get(DAC0)
        print(f"    Set: {v:.1f}V, Read: {actual:.3f}V")
        time.sleep(0.3)
    
    # Reset DACs
    dac_set(DAC0, 0.0)
    dac_set(DAC1, 0.0)
    
    print("✓ DAC demo complete")

def demo_adc_functions():
    """Demonstrate ADC (Analog-to-Digital Converter) functions"""
    
    print("\n" + "="*75)
    print("ADC (ANALOG-TO-DIGITAL CONVERTER) FUNCTIONS")
    print("="*75)
    print("ADC channels: 0-4")
    print("Voltage range: Depends on hardware configuration")
    
    oled_clear()
    oled_print("ADC Demo")
    
    print("\n📍 Basic ADC Operations:")
    
    # Read all ADC channels
    for channel in range(5):
        voltage = adc_get(channel)
        print(f"  adc_get({channel}) = {voltage:.3f}V")
    
    # Using alias function
    voltage = get_adc(0)
    print(f"  get_adc(0) = {voltage:.3f}V (alias function)")
    
    # Continuous monitoring example
    print("\n📊 ADC Monitoring (5 samples):")
    print("  Channel | Sample 1 | Sample 2 | Sample 3 | Sample 4 | Sample 5")
    print("  --------|----------|----------|----------|----------|----------")
    
    for channel in range(3):  # Monitor first 3 channels
        readings = []
        for i in range(5):
            readings.append(adc_get(channel))
            time.sleep(0.1)
        
        readings_str = " | ".join([f"{r:8.3f}" for r in readings])
        print(f"    ADC{channel}  | {readings_str}")
    
    print("✓ ADC demo complete")

def demo_gpio_functions():
    """Demonstrate GPIO (General Purpose Input/Output) functions"""
    
    print("\n" + "="*75)
    print("GPIO (GENERAL PURPOSE INPUT/OUTPUT) FUNCTIONS")
    print("="*75)
    print("GPIO pins: 1-8 (routable GPIO), 9-10 (UART)")
    print("Values: True/False for HIGH/LOW")
    print("Directions: True/False for OUTPUT/INPUT")
    print("Pull resistors: 1=PULLUP, 0=NONE, -1=PULLDOWN")
    
    oled_clear()
    oled_print("GPIO Demo")
    
    # Configure GPIO pins as outputs
    print("\n📍 GPIO Configuration:")
    for pin in range(1, 5):  # Configure pins 1-4
        gpio_set_dir(pin, True)  # Set as output
        direction = gpio_get_dir(pin)
        print(f"  GPIO{pin} direction: {direction}")
    
    # Set GPIO states
    print("\n📤 Setting GPIO States:")
    gpio_set(1, True)   # Set high
    gpio_set(2, False)  # Set low
    set_gpio(3, True)   # Using alias
    set_gpio(4, False)  # Using alias
    
    for pin in range(1, 5):
        state = gpio_get(pin)
        print(f"  GPIO{pin} state: {state}")
    
    # Demonstrate GPIO pull resistors
    print("\n🔗 GPIO Pull Resistors:")
    gpio_set_dir(5, False)  # Set as input for pull resistor demo
    
    # Set different pull configurations
    gpio_set_pull(5, 1)   # Pull-up
    pull = gpio_get_pull(5)
    print(f"  GPIO5 pull (set to 1): {pull}")
    
    gpio_set_pull(5, 0)   # No pull
    pull = gpio_get_pull(5)
    print(f"  GPIO5 pull (set to 0): {pull}")
    
    gpio_set_pull(5, -1)  # Pull-down
    pull = gpio_get_pull(5)
    print(f"  GPIO5 pull (set to -1): {pull}")
    
    # GPIO blinking pattern
    print("\n💫 GPIO Blink Pattern:")
    print("  Blinking GPIO1-4 in sequence...")
    for cycle in range(3):
        for pin in range(1, 5):
            gpio_set(pin, True)
            time.sleep(0.2)
            gpio_set(pin, False)
            time.sleep(0.1)
    
    # Reset all GPIOs to safe state
    for pin in range(1, 5):
        gpio_set(pin, False)
    
    print("✓ GPIO demo complete")

def demo_ina_functions():
    """Demonstrate INA (Current/Power Monitor) functions"""
    
    print("\n" + "="*75)
    print("INA (CURRENT/POWER MONITOR) FUNCTIONS")
    print("="*75)
    print("INA sensors: 0, 1")
    print("Measurements: current (A), voltage (V), bus voltage (V), power (W)")
    
    oled_clear()
    oled_print("INA Demo")
    
    print("\n📍 INA Sensor Readings:")
    
    # Read all measurements from both sensors
    for sensor in range(2):
        print(f"\n  INA Sensor {sensor}:")
        
        # Current measurement
        current = ina_get_current(sensor)
        current_alias = get_current(sensor)  # Using alias
        print(f"    Current: {current:.6f}A (ina_get_current)")
        print(f"    Current: {current_alias:.6f}A (get_current alias)")
        
        # Voltage measurements
        shunt_voltage = ina_get_voltage(sensor)
        bus_voltage = ina_get_bus_voltage(sensor)
        print(f"    Shunt voltage: {shunt_voltage:.6f}V")
        print(f"    Bus voltage: {bus_voltage:.3f}V")
        
        # Power measurement
        power = ina_get_power(sensor)
        power_alias = get_power(sensor)  # Using alias
        print(f"    Power: {power:.6f}W (ina_get_power)")
        print(f"    Power: {power_alias:.6f}W (get_power alias)")
    
    # Continuous monitoring
    print("\n📊 Power Monitoring (5 samples):")
    print("  Sample | INA0 Current | INA0 Power | INA1 Current | INA1 Power")
    print("  -------|--------------|------------|--------------|------------")
    
    for i in range(5):
        current0 = get_current(0)
        power0 = get_power(0)
        current1 = get_current(1)
        power1 = get_power(1)
        
        print(f"    {i+1:2d}   | {current0:10.6f}A | {power0:8.6f}W | {current1:10.6f}A | {power1:8.6f}W")
        time.sleep(0.5)
    
    print("✓ INA demo complete")

def demo_connection_functions():
    """Demonstrate node connection functions"""
    
    print("\n" + "="*75)
    print("NODE CONNECTION FUNCTIONS")
    print("="*75)
    print("Connect breadboard holes, Arduino pins, power rails, GPIO, DACs, ADCs, etc.")
    print("Node formats: numbers (1-60), strings (\"D13\"), constants (TOP_RAIL)")
    
    oled_clear()
    oled_print("Connections Demo")
    
    # Clear any existing connections
    nodes_clear()
    print("  nodes_clear() - Cleared all connections")
    
    print("\n📍 Basic Connections:")
    
    # Different ways to specify nodes
    connect(1, 30)                    # Numbers
    print("  connect(1, 30) - Connect breadboard holes 1 and 30")
    
    connect("D13", "TOP_RAIL")        # Strings
    print("  connect(\"D13\", \"TOP_RAIL\") - Connect D13 to top rail")
    
    connect(D13, BOTTOM_RAIL)         # Constants  
    print("  connect(D13, BOTTOM_RAIL) - Connect D13 to bottom rail")
    
    connect(GPIO_1, A0)               # GPIO to analog pin
    print("  connect(GPIO_1, A0) - Connect GPIO1 to A0")
    
    connect(DAC0, 15)                 # Mix constants and numbers
    print("  connect(DAC0, 15) - Connect DAC0 to hole 15")
    
    # Check connections
    print("\n🔍 Checking Connections:")
    connections = [
        (1, 30),
        ("D13", "TOP_RAIL"), 
        (GPIO_1, A0),
        (DAC0, 15)
    ]
    
    for node1, node2 in connections:
        connected = is_connected(node1, node2)
        print(f"  is_connected({node1}, {node2}): {connected}")
    
    # Create a more complex circuit
    print("\n🔌 Building Example Circuit:")
    print("  Creating LED circuit: GPIO1 -> LED -> Resistor -> GND")
    
    # Connect GPIO1 to breadboard for LED
    connect(GPIO_1, 5)
    print("    connect(GPIO_1, 5) - GPIO to LED positive")
    
    # Connect LED cathode through resistor to ground
    connect(6, 10)  # LED cathode to resistor
    connect(10, GND)  # Resistor to ground
    print("    connect(6, 10) and connect(10, GND) - LED to ground through resistor")
    
    # Test the LED
    gpio_set_dir(1, True)  # Set GPIO1 as output
    gpio_set(1, True)      # Turn on LED
    print("    GPIO1 set HIGH - LED should be on")
    time.sleep(1)
    gpio_set(1, False)     # Turn off LED
    print("    GPIO1 set LOW - LED should be off")
    
    # Disconnect some connections
    print("\n❌ Disconnecting:")
    disconnect(GPIO_1, 5)
    connected = is_connected(GPIO_1, 5)
    print(f"  disconnect(GPIO_1, 5) - Now connected: {connected}")
    
    # Show all connections
    print("\n📋 Current Bridge Status:")
    print_bridges()  # This will print to console
    
    print("✓ Connection demo complete")

def demo_oled_functions():
    """Demonstrate OLED display functions"""
    
    print("\n" + "="*75)
    print("OLED DISPLAY FUNCTIONS")
    print("="*75)
    print("Functions: oled_print(), oled_clear(), oled_show(), oled_connect(), oled_disconnect()")
    
    oled_clear()
    oled_print("OLED Demo Starting...")
    time.sleep(2)
    
    print("\n📺 OLED Display Tests:")
    
    # Text display
    oled_clear()
    oled_print("Hello Jumperless!")
    print("  oled_print(\"Hello Jumperless!\") - Display text")
    time.sleep(2)
    
    # Display different data types
    test_data = [
        ("Numbers", 42),
        ("Floats", 3.14159),
        ("Boolean", True),
        ("Node", D13),
        ("GPIO State", gpio_get(1))
    ]
    
    for label, value in test_data:
        oled_clear()
        oled_print(f"{label}: {value}")
        print(f"  oled_print(\"{label}: {value}\") - Display {label.lower()}")
        time.sleep(1.5)
    
    # Display custom types
    oled_clear()
    oled_print(TOP_RAIL)  # Display node constant
    print("  oled_print(TOP_RAIL) - Display node constant")
    time.sleep(1.5)
    
    # Display measurements
    voltage = adc_get(0)
    oled_clear()
    oled_print(f"ADC0: {voltage:.3f}V")
    print(f"  oled_print(\"ADC0: {voltage:.3f}V\") - Display measurement")
    time.sleep(1.5)
    
    # Connection status
    connection_status = is_connected(1, 30)
    oled_clear()
    oled_print(connection_status)  # Will show "CONNECTED" or "DISCONNECTED"
    print(f"  oled_print(connection_status) - Display: {connection_status}")
    time.sleep(1.5)
    
    # Show that the display works with the module's custom types
    oled_clear()
    oled_print("Demo Complete!")
    print("  oled_print(\"Demo Complete!\") - Final message")
    
    print("✓ OLED demo complete")

def demo_probe_functions():
    """Demonstrate probe and button functions"""
    
    print("\n" + "="*75)
    print("PROBE AND BUTTON FUNCTIONS")
    print("="*75)
    print("Probe functions: probe_read(), wait_touch(), check_button(), etc.")
    print("Returns: ProbePad objects (1-60, D13_PAD, LOGO_PAD_TOP, etc.)")
    print("Button returns: CONNECT, REMOVE, NONE")
    
    oled_clear()
    oled_print("Probe Demo")
    
    print("\n🔍 Probe Functions Available:")
    print("  Blocking touch: probe_read(), wait_touch(), probe_touch()")
    print("  Non-blocking: probe_read(False), probe_read_nonblocking()")
    print("  Button (blocking): get_button(), probe_button()")
    print("  Button (non-blocking): check_button(), get_button(False)")
    
    print("\n📱 Non-blocking Tests:")
    
    # Non-blocking probe test
    pad = probe_read(False)  # Non-blocking
    print(f"  probe_read(False) = {pad}")
    
    # Non-blocking button test
    button = check_button()
    print(f"  check_button() = {button}")
    
    # Test probe constants
    print("\n🎯 Probe Pad Constants:")
    probe_constants = [
        ("LOGO_PAD_TOP", LOGO_PAD_TOP),
        ("D13_PAD", D13_PAD),
        ("TOP_RAIL_PAD", TOP_RAIL_PAD),
        ("BOTTOM_RAIL_PAD", BOTTOM_RAIL_PAD)
    ]
    
    for name, constant in probe_constants:
        print(f"  {name} = {constant}")
    
    # Button constants
    print("\n🔘 Button Constants:")
    button_constants = [
        ("BUTTON_NONE", BUTTON_NONE),
        ("BUTTON_CONNECT", BUTTON_CONNECT), 
        ("BUTTON_REMOVE", BUTTON_REMOVE),
        ("CONNECT_BUTTON", CONNECT_BUTTON),
        ("REMOVE_BUTTON", REMOVE_BUTTON)
    ]
    
    for name, constant in button_constants:
        print(f"  {name} = {constant}")
    
    print("\n💡 Interactive Demo:")
    print("  For interactive probe demo, try:")
    print("    pad = probe_read()        # Wait for touch")
    print("    button = get_button()     # Wait for button")
    print("    print(f'Touched: {pad}')")
    print("    print(f'Button: {button}')")
    
    print("✓ Probe demo complete")

def demo_system_functions():
    """Demonstrate system and status functions"""
    
    print("\n" + "="*75)
    print("SYSTEM AND STATUS FUNCTIONS")
    print("="*75)
    print("Status: print_bridges(), print_nets(), print_chip_status()")
    print("System: arduino_reset(), run_app(), help()")
    
    oled_clear()
    oled_print("System Demo")
    
    print("\n🔧 System Status Functions:")
    
    # Bridge status
    print("  print_bridges() - Show current connections:")
    print_bridges()
    
    print("\n  print_nets() - Show network information:")
    print_nets()
    
    print("\n  print_chip_status() - Show hardware status:")
    print_chip_status()
    
    print("\n📟 Clickwheel Functions:")
    
    # Clickwheel controls
    print("  Testing clickwheel controls...")
    clickwheel_up(1)
    print("    clickwheel_up(1) - Scroll up 1 step")
    
    clickwheel_down(2)  
    print("    clickwheel_down(2) - Scroll down 2 steps")
    
    clickwheel_press()
    print("    clickwheel_press() - Press center button")
    
    print("\n🔄 System Functions:")
    
    # Note: arduino_reset() would actually reset the Arduino, so we just show the syntax
    print("  arduino_reset() - Resets connected Arduino")
    
    print("\n💡 Help Functions:")
    print("  help() - Show module help")
    print("  nodes_help() - Show node reference")
    
    print("✓ System demo complete")

def demo_advanced_examples():
    """Demonstrate advanced usage combining multiple functions"""
    
    print("\n" + "="*75)
    print("ADVANCED EXAMPLES - COMBINING FUNCTIONS")
    print("="*75)
    
    oled_clear()
    oled_print("Advanced Demo")
    
    # Example 1: Voltage monitoring with display
    print("\n📊 Example 1: Voltage Monitor with OLED Display")
    
    # Set up a test voltage on DAC0
    dac_set(DAC0, 2.5)
    connect(DAC0, ADC0)  # Connect DAC output to ADC input
    
    print("  Setup: DAC0 -> ADC0 connection for monitoring")
    
    for i in range(5):
        voltage = adc_get(0)
        oled_clear()
        oled_print(f"V: {voltage:.3f}V")
        print(f"    Reading {i+1}: {voltage:.3f}V")
        time.sleep(1)
    
    disconnect(DAC0, ADC0)
    dac_set(DAC0, 0.0)
    
    # Example 2: GPIO control with current monitoring
    print("\n💡 Example 2: LED Control with Current Monitoring")
    
    # Set up LED circuit
    connect(GPIO_1, 25)  # GPIO to LED
    connect(26, ISENSE_PLUS)  # LED return through current sensor
    connect(ISENSE_MINUS, GND)  # Current sensor to ground
    
    gpio_set_dir(1, True)  # Set GPIO as output
    
    print("  Circuit: GPIO1 -> LED -> Current Sensor -> GND")
    
    # Monitor current while controlling LED
    for state in [True, False, True, False]:
        gpio_set(1, state)
        time.sleep(0.2)  # Allow current to stabilize
        current = get_current(0)
        gpio_state = gpio_get(1)
        
        oled_clear()
        oled_print(f"LED: {gpio_state}")
        print(f"    LED {gpio_state}, Current: {current:.6f}A")
        time.sleep(1)
    
    # Clean up
    gpio_set(1, False)
    disconnect(GPIO_1, 25)
    disconnect(26, ISENSE_PLUS)
    disconnect(ISENSE_MINUS, GND)
    
    # Example 3: Multi-rail power supply
    print("\n⚡ Example 3: Configurable Power Supply")
    
    # Set different rail voltages
    voltages = [3.3, 5.0, 1.8, 2.5]
    rails = [TOP_RAIL, BOTTOM_RAIL, DAC0, DAC1]
    rail_names = ["TOP_RAIL", "BOTTOM_RAIL", "DAC0", "DAC1"]
    
    print("  Setting up multi-rail power supply:")
    for voltage, rail, name in zip(voltages, rails, rail_names):
        dac_set(rail, voltage)
        actual = dac_get(rail)
        print(f"    {name}: {voltage}V (actual: {actual:.3f}V)")
        
        oled_clear()
        oled_print(f"{name}: {actual:.1f}V")
        time.sleep(1)
    
    # Reset all to safe values
    for rail in rails:
        dac_set(rail, 0.0)
    
    print("\n🎯 Example 4: Node Type Demonstrations")
    
    # Show different ways to work with nodes
    my_node = node("D13")  # Create node from string
    connect(my_node, TOP_RAIL)
    
    # Check connection using node object
    connected = is_connected(my_node, TOP_RAIL)
    print(f"  Node object connection: {connected}")
    
    # Display node on OLED
    oled_clear()
    oled_print(my_node)  # Will show "D13"
    print(f"  Node object displays as: {my_node}")
    
    # Use node in comparisons
    if my_node == D13:
        print("  Node object equals D13 constant: True")
    
    if my_node == 83:  # D13's internal value
        print("  Node object equals numeric value: True")
    
    disconnect(my_node, TOP_RAIL)
    
    print("✓ Advanced examples complete")

def basic_demo():
    """Quick demonstration of basic functions"""
    
    print("\n" + "="*40)
    print("BASIC JUMPERLESS DEMO")
    print("="*40)
    
    oled_clear()
    oled_print("Basic Demo")
    
    # Basic operations
    dac_set(DAC0, 3.3)
    voltage = adc_get(0)
    
    connect(D13, TOP_RAIL)
    connected = is_connected(D13, TOP_RAIL)
    
    gpio_set_dir(1, True)
    gpio_set(1, True)
    state = gpio_get(1)
    
    current = get_current(0)
    
    print(f"DAC0: 3.3V, ADC0: {voltage:.3f}V")
    print(f"D13 to TOP_RAIL: {connected}")
    print(f"GPIO1: {state}")
    print(f"Current: {current:.6f}A")
    
    # Cleanup
    dac_set(DAC0, 0.0)
    gpio_set(1, False)
    disconnect(D13, TOP_RAIL)
    
    oled_clear()
    oled_print("Basic Demo Done")

# Function reference organized by category
def show_function_reference():
    """Display organized function reference"""
    
    print("\n" + "="*75)
    print("JUMPERLESS MICROPYTHON FUNCTION REFERENCE")
    print("="*75)
    
    categories = {
        "DAC Functions": [
            "dac_set(channel, voltage) / set_dac()",
            "dac_get(channel) / get_dac()",
            "Channels: 0-3, DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL"
        ],
        
        "ADC Functions": [
            "adc_get(channel) / get_adc()",
            "Channels: 0-4"
        ],
        
        "GPIO Functions": [
            "gpio_set(pin, value) / set_gpio()",
            "gpio_get(pin) / get_gpio()", 
            "gpio_set_dir(pin, direction) / set_gpio_dir()",
            "gpio_get_dir(pin) / get_gpio_dir()",
            "gpio_set_pull(pin, pull) / set_gpio_pull()",
            "gpio_get_pull(pin) / get_gpio_pull()",
            "Pins: 1-8 (GPIO), 9-10 (UART)"
        ],
        
        "INA Functions": [
            "ina_get_current(sensor) / get_current()",
            "ina_get_voltage(sensor) / get_voltage()",
            "ina_get_bus_voltage(sensor) / get_bus_voltage()",
            "ina_get_power(sensor) / get_power()",
            "Sensors: 0, 1"
        ],
        
        "Connection Functions": [
            "connect(node1, node2)",
            "disconnect(node1, node2)",
            "is_connected(node1, node2)",
            "nodes_clear()"
        ],
        
        "OLED Functions": [
            "oled_print(text)",
            "oled_clear()",
            "oled_show()",
            "oled_connect() / oled_disconnect()"
        ],
        
        "Probe Functions": [
            "probe_read([blocking=True]) / read_probe()",
            "probe_read_blocking() / probe_read_nonblocking()",
            "wait_touch() / probe_touch() / probe_wait()",
            "get_button([blocking=True]) / probe_button()",
            "check_button() / button_check()",
            "probe_button_blocking() / probe_button_nonblocking()"
        ],
        
        "System Functions": [
            "print_bridges() / print_nets() / print_chip_status()",
            "arduino_reset()",
            "clickwheel_up() / clickwheel_down() / clickwheel_press()",
            "run_app(appName)",
            "help() / nodes_help()"
        ],
        
        "Node Creation": [
            "node(name_or_number)",
            "Constants: D0-D13, A0-A7, GPIO_1-GPIO_8",
            "Power: TOP_RAIL, BOTTOM_RAIL, GND",
            "DACs: DAC0, DAC1",
            "ADCs: ADC0-ADC4",
            "Special: UART_TX, UART_RX, BUFFER_IN, BUFFER_OUT"
        ]
    }
    
    for category, functions in categories.items():
        print(f"\n📚 {category}:")
        for func in functions:
            print(f"  • {func}")

# Entry point functions
def run_comprehensive_demo():
    """Run the complete comprehensive demo"""
    comprehensive_demo()

def run_basic_demo():
    """Run just the basic demo"""
    basic_demo()

def show_help():
    """Show function reference"""
    show_function_reference()

# Main execution
if __name__ == "__main__":
    print("Jumperless MicroPython Complete Reference loaded.")
    print("\nAvailable functions:")
    print("  run_comprehensive_demo() - Full demonstration of all functions")
    print("  run_basic_demo()         - Quick basic demo")
    print("  show_help()              - Show function reference")
    print("  help()                   - Show built-in help")
    print("\nFor individual categories, see separate files:")
    print("  exec(open('examples/dac_reference.py').read())")
    print("  exec(open('examples/gpio_reference.py').read())")
    print("  etc...")
else:
    print("Jumperless Complete Reference module loaded.")
    print("Call run_comprehensive_demo() or run_basic_demo() to start.") 