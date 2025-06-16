"""
Jumperless Custom Boolean Types Integration
==========================================

This shows how to integrate custom boolean types into the Jumperless
MicroPython module so that functions return readable values while
maintaining boolean behavior.
"""

# Custom Types for Jumperless
class GPIOState:
    """GPIO state that prints as HIGH/LOW but behaves as True/False"""
    def __init__(self, value):
        self._value = bool(value)
    
    def __bool__(self):
        return self._value
    
    def __str__(self):
        return "HIGH" if self._value else "LOW"
    
    def __repr__(self):
        return f"GPIOState({self._value})"
    
    def __eq__(self, other):
        if isinstance(other, GPIOState):
            return self._value == other._value
        return self._value == bool(other)

class GPIODirection:
    """GPIO direction that prints as INPUT/OUTPUT"""
    def __init__(self, value):
        self._value = bool(value)
    
    def __bool__(self):
        return self._value
    
    def __str__(self):
        return "OUTPUT" if self._value else "INPUT"
    
    def __repr__(self):
        return f"GPIODirection({self._value})"
    
    def __eq__(self, other):
        if isinstance(other, GPIODirection):
            return self._value == other._value
        return self._value == bool(other)

class GPIOPull:
    """GPIO pull resistor state"""
    def __init__(self, value):
        # 1 = PULLUP, -1 = PULLDOWN, 0 = NONE
        self._value = value
    
    def __bool__(self):
        return self._value == 1  # Only PULLUP is "truthy"
    
    def __str__(self):
        if self._value == 1:
            return "PULLUP"
        elif self._value == -1:
            return "PULLDOWN"
        else:
            return "NONE"
    
    def __repr__(self):
        return f"GPIOPull({self._value})"

class ConnectionState:
    """Connection state that prints as CONNECTED/DISCONNECTED"""
    def __init__(self, value):
        self._value = bool(value)
    
    def __bool__(self):
        return self._value
    
    def __str__(self):
        return "CONNECTED" if self._value else "DISCONNECTED"
    
    def __repr__(self):
        return f"ConnectionState({self._value})"

class VoltageValue:
    """Voltage value that includes units and formatting"""
    def __init__(self, value):
        self._value = float(value)
    
    def __float__(self):
        return self._value
    
    def __str__(self):
        if self._value >= 1.0:
            return f"{self._value:.3f}V"
        else:
            return f"{self._value * 1000:.1f}mV"
    
    def __repr__(self):
        return f"VoltageValue({self._value})"
    
    def __eq__(self, other):
        if isinstance(other, VoltageValue):
            return abs(self._value - other._value) < 0.001
        return abs(self._value - float(other)) < 0.001

class CurrentValue:
    """Current value that includes units and formatting"""
    def __init__(self, value):
        self._value = float(value)
    
    def __float__(self):
        return self._value
    
    def __str__(self):
        if abs(self._value) >= 1.0:
            return f"{self._value:.3f}A"
        else:
            return f"{self._value * 1000:.1f}mA"
    
    def __repr__(self):
        return f"CurrentValue({self._value})"

class PowerValue:
    """Power value that includes units and formatting"""
    def __init__(self, value):
        self._value = float(value)
    
    def __float__(self):
        return self._value
    
    def __str__(self):
        if abs(self._value) >= 1.0:
            return f"{self._value:.3f}W"
        else:
            return f"{self._value * 1000:.1f}mW"
    
    def __repr__(self):
        return f"PowerValue({self._value})"

# Constants
HIGH = GPIOState(True)
LOW = GPIOState(False)
OUTPUT = GPIODirection(True)
INPUT = GPIODirection(False)
PULLUP = GPIOPull(1)
PULLDOWN = GPIOPull(-1)
PULL_NONE = GPIOPull(0)
CONNECTED = ConnectionState(True)
DISCONNECTED = ConnectionState(False)

# Example modified Jumperless functions that return custom types
def gpio_get_enhanced(pin):
    """Example GPIO get function that returns formatted state"""
    # Simulate getting GPIO state (replace with actual implementation)
    raw_value = pin % 2  # Dummy logic for demo
    return GPIOState(raw_value)

def gpio_get_dir_enhanced(pin):
    """Example GPIO direction function that returns formatted direction"""
    # Simulate getting GPIO direction (replace with actual implementation)
    raw_value = pin % 2  # Dummy logic for demo
    return GPIODirection(raw_value)

def gpio_get_pull_enhanced(pin):
    """Example GPIO pull function that returns formatted pull state"""
    # Simulate getting GPIO pull state (replace with actual implementation)
    states = [PULL_NONE, PULLUP, PULLDOWN]
    return states[pin % 3]

def dac_get_enhanced(channel):
    """Example DAC get function that returns formatted voltage"""
    # Simulate getting DAC voltage (replace with actual implementation)
    raw_voltage = (channel + 1) * 1.65  # Dummy logic for demo
    return VoltageValue(raw_voltage)

def ina_get_voltage_enhanced():
    """Example INA voltage function that returns formatted voltage"""
    # Simulate getting INA voltage (replace with actual implementation)
    raw_voltage = 3.298  # Dummy logic for demo
    return VoltageValue(raw_voltage)

def ina_get_current_enhanced():
    """Example INA current function that returns formatted current"""
    # Simulate getting INA current (replace with actual implementation)
    raw_current = 0.0234  # Dummy logic for demo
    return CurrentValue(raw_current)

def ina_get_power_enhanced():
    """Example INA power function that returns formatted power"""
    # Simulate getting INA power (replace with actual implementation)
    raw_power = 0.0772  # Dummy logic for demo
    return PowerValue(raw_power)

def is_connected_enhanced(node1, node2):
    """Example connection check function that returns formatted state"""
    # Simulate checking connection (replace with actual implementation)
    connected = (node1 + node2) % 2  # Dummy logic for demo
    return ConnectionState(connected)

# Test the enhanced functions
def test_enhanced_functions():
    print("=== Testing Enhanced Jumperless Functions ===")
    
    # Test GPIO functions
    print("\n1. GPIO Functions:")
    gpio_state = gpio_get_enhanced(2)
    print(f"gpio_get(2) = {gpio_state}")
    print(f"  - Type: {type(gpio_state)}")
    print(f"  - Boolean value: {bool(gpio_state)}")
    print(f"  - Can use in if: {gpio_state and 'YES' or 'NO'}")
    
    gpio_dir = gpio_get_dir_enhanced(2)
    print(f"gpio_get_dir(2) = {gpio_dir}")
    print(f"  - Boolean value: {bool(gpio_dir)}")
    
    gpio_pull = gpio_get_pull_enhanced(2)
    print(f"gpio_get_pull(2) = {gpio_pull}")
    print(f"  - Boolean value: {bool(gpio_pull)}")
    
    # Test voltage/current/power functions
    print("\n2. Analog Functions:")
    dac_voltage = dac_get_enhanced(0)
    print(f"dac_get(0) = {dac_voltage}")
    print(f"  - Float value: {float(dac_voltage)}")
    
    ina_voltage = ina_get_voltage_enhanced()
    print(f"ina_get_voltage() = {ina_voltage}")
    
    ina_current = ina_get_current_enhanced()
    print(f"ina_get_current() = {ina_current}")
    
    ina_power = ina_get_power_enhanced()
    print(f"ina_get_power() = {ina_power}")
    
    # Test connection functions
    print("\n3. Connection Functions:")
    connection = is_connected_enhanced(1, 5)
    print(f"is_connected(1, 5) = {connection}")
    print(f"  - Boolean value: {bool(connection)}")
    
    # Test conditional usage
    print("\n4. Conditional Usage Examples:")
    if gpio_get_enhanced(2):
        print("GPIO 2 is HIGH")
    else:
        print("GPIO 2 is LOW")
    
    if gpio_get_dir_enhanced(3):
        print("GPIO 3 is set as OUTPUT")
    else:
        print("GPIO 3 is set as INPUT")
    
    if is_connected_enhanced(1, 5):
        print("Nodes 1 and 5 are CONNECTED")
    else:
        print("Nodes 1 and 5 are DISCONNECTED")

if __name__ == "__main__":
    test_enhanced_functions() 