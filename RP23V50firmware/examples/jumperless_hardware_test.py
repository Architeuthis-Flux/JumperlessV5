#!/usr/bin/env python3
"""
Jumperless Hardware Control Test
Demonstrates the consolidated hardware control functionality in the Jumperless module
"""

# Import the Jumperless module with all hardware control
import jumperless

def test_hardware_types():
    """Test the hardware control types"""
    print("=== Jumperless Hardware Control Test ===")
    print()
    
    # Test ADC functionality
    print("Testing ADC:")
    try:
        adc = jumperless.ADC(0)
        value = adc.read()
        value_u16 = adc.read_u16()
        print(f"  ADC raw value: {value}")
        print(f"  ADC 16-bit value: {value_u16}")
        
        # Test attenuation and width settings
        adc.atten(jumperless.ADC.ATTN_6DB)
        adc.width(jumperless.ADC.WIDTH_12BIT)
        print(f"  Attenuation: {adc.atten()}")
        print(f"  Width: {adc.width()}")
    except Exception as e:
        print(f"  ADC test failed: {e}")
    print()
    
    # Test Pin functionality
    print("Testing Pin:")
    try:
        pin = jumperless.Pin(1)
        pin.init(mode=jumperless.Pin.OUT, value=0)
        
        # Test pin control
        pin.value(1)
        print(f"  Pin value after setting high: {pin.value()}")
        pin(0)  # Test call syntax
        print(f"  Pin value after setting low: {pin()}")
        
        # Test mode setting
        pin.mode(jumperless.Pin.IN)
        print(f"  Pin mode: {pin.mode()}")
    except Exception as e:
        print(f"  Pin test failed: {e}")
    print()
    
    # Test PWM functionality
    print("Testing PWM:")
    try:
        pwm = jumperless.PWM(1)
        
        # Test frequency and duty cycle
        pwm.freq(1000)
        pwm.duty(512)
        print(f"  PWM frequency: {pwm.freq()} Hz")
        print(f"  PWM duty: {pwm.duty()}")
        
        # Test different duty formats
        pwm.duty_u16(32768)
        print(f"  PWM duty_u16: {pwm.duty_u16()}")
        
        duty_ns = pwm.duty_ns()
        print(f"  PWM duty_ns: {duty_ns} ns")
        
        pwm.deinit()
    except Exception as e:
        print(f"  PWM test failed: {e}")
    print()

def test_jumperless_functions():
    """Test the core Jumperless functions"""
    print("=== Core Jumperless Functions ===")
    print()
    
    # Test DAC functions
    print("Testing DAC:")
    try:
        jumperless.dac_set(0, 1.65)
        voltage = jumperless.dac_get(0)
        print(f"  DAC 0 voltage: {voltage}V")
        
        # Test with node constants
        jumperless.dac_set(jumperless.TOP_RAIL, 3.3)
        rail_voltage = jumperless.dac_get(jumperless.TOP_RAIL)
        print(f"  Top rail voltage: {rail_voltage}V")
    except Exception as e:
        print(f"  DAC test failed: {e}")
    print()
    
    # Test GPIO functions
    print("Testing GPIO:")
    try:
        jumperless.gpio_set_dir(1, 1)  # Set as output
        direction = jumperless.gpio_get_dir(1)
        print(f"  GPIO 1 direction: {direction}")
        
        jumperless.gpio_set(1, 1)  # Set high
        state = jumperless.gpio_get(1)
        print(f"  GPIO 1 state: {state}")
        
        jumperless.gpio_set_pull(1, 1)  # Set pull-up
        pull = jumperless.gpio_get_pull(1)
        print(f"  GPIO 1 pull: {pull}")
    except Exception as e:
        print(f"  GPIO test failed: {e}")
    print()
    
    # Test node connections
    print("Testing Node Connections:")
    try:
        # Connect some nodes
        jumperless.connect(1, 30)
        jumperless.connect(jumperless.D13, jumperless.TOP_RAIL)
        
        # Check connections
        connected = jumperless.is_connected(1, 30)
        print(f"  Nodes 1-30 connected: {connected}")
        
        connected = jumperless.is_connected(jumperless.D13, jumperless.TOP_RAIL)
        print(f"  D13-TOP_RAIL connected: {connected}")
        
        # Disconnect
        jumperless.disconnect(1, 30)
        connected = jumperless.is_connected(1, 30)
        print(f"  Nodes 1-30 after disconnect: {connected}")
    except Exception as e:
        print(f"  Node connection test failed: {e}")
    print()
    
    # Test OLED
    print("Testing OLED:")
    try:
        jumperless.oled_clear()
        jumperless.oled_print("Hardware Test")
        jumperless.oled_print("SUCCESS!", 1)
    except Exception as e:
        print(f"  OLED test failed: {e}")
    print()

def test_node_types():
    """Test the node type functionality"""
    print("=== Node Type Testing ===")
    print()
    
    try:
        # Test creating nodes from different inputs
        node1 = jumperless.node("D13")
        node2 = jumperless.node(30)
        node3 = jumperless.TOP_RAIL
        
        print(f"  Node from string 'D13': {node1}")
        print(f"  Node from integer 30: {node2}")
        print(f"  Predefined TOP_RAIL: {node3}")
        
        # Test using nodes in functions
        jumperless.connect(node1, node3)
        connected = jumperless.is_connected(node1, node3)
        print(f"  Connected D13 to TOP_RAIL: {connected}")
        
        # Test OLED with nodes
        jumperless.oled_print(node1)
        jumperless.oled_print(node3)
        
    except Exception as e:
        print(f"  Node type test failed: {e}")
    print()

if __name__ == "__main__":
    print("Jumperless Hardware Control Consolidated Test")
    print("=" * 50)
    
    try:
        test_hardware_types()
        test_jumperless_functions()
        test_node_types()
        
        print("=== Test Summary ===")
        print("All hardware control functionality is now consolidated")
        print("into the Jumperless module. No separate machine module needed!")
        print()
        print("Available hardware types:")
        print("  - jumperless.ADC(pin)")
        print("  - jumperless.Pin(pin)")
        print("  - jumperless.PWM(pin)")
        print()
        print("All functions and constants available directly from jumperless module")
        
    except Exception as e:
        print(f"Test failed with error: {e}")
        import traceback
        traceback.print_exc() 