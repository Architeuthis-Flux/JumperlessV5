"""
Custom Boolean-like Types for MicroPython
==========================================

These examples show how to create types that behave like True/False in 
boolean contexts but print with custom representations like HIGH/LOW.

This is perfect for GPIO states, directions, pull resistors, etc.
"""

# Approach 1: GPIO State Class
class GPIOState:
    """A boolean-like type that prints as HIGH/LOW"""
    
    def __init__(self, value):
        self._value = bool(value)
    
    def __bool__(self):
        """Return the boolean value - this makes it work in if statements"""
        return self._value
    
    def __str__(self):
        """Custom string representation"""
        return "HIGH" if self._value else "LOW"
    
    def __repr__(self):
        """Representation for debugging"""
        return f"GPIOState({self._value})"
    
    def __eq__(self, other):
        """Allow comparison with booleans and other GPIOStates"""
        if isinstance(other, GPIOState):
            return self._value == other._value
        return self._value == bool(other)

# Create constants
HIGH = GPIOState(True)
LOW = GPIOState(False)

# Approach 2: GPIO Direction Class
class GPIODirection:
    """A boolean-like type that prints as INPUT/OUTPUT"""
    
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

# Create constants
OUTPUT = GPIODirection(True)
INPUT = GPIODirection(False)

# Approach 3: GPIO Pull Resistor Class (with 3 states)
class GPIOPull:
    """A type for pull resistor states"""
    
    def __init__(self, value):
        # 0 = NONE, 1 = PULLUP, -1 = PULLDOWN
        self._value = value
    
    def __bool__(self):
        """PULLUP is True, everything else is False"""
        return self._value == 1
    
    def __str__(self):
        if self._value == 1:
            return "PULLUP"
        elif self._value == -1:
            return "PULLDOWN"
        else:
            return "NONE"
    
    def __repr__(self):
        return f"GPIOPull({self._value})"
    
    def __eq__(self, other):
        if isinstance(other, GPIOPull):
            return self._value == other._value
        return False

# Create constants
PULLUP = GPIOPull(1)
PULLDOWN = GPIOPull(-1)
PULL_NONE = GPIOPull(0)

# Approach 4: Connection State Class
class ConnectionState:
    """A boolean-like type that prints as CONNECTED/DISCONNECTED"""
    
    def __init__(self, value):
        self._value = bool(value)
    
    def __bool__(self):
        return self._value
    
    def __str__(self):
        return "CONNECTED" if self._value else "DISCONNECTED"
    
    def __repr__(self):
        return f"ConnectionState({self._value})"
    
    def __eq__(self, other):
        if isinstance(other, ConnectionState):
            return self._value == other._value
        return self._value == bool(other)

# Create constants
CONNECTED = ConnectionState(True)
DISCONNECTED = ConnectionState(False)

# Test the custom types
def test_custom_types():
    print("=== Testing Custom Boolean Types ===")
    
    # Test GPIO States
    print("\n1. GPIO State Testing:")
    print(f"HIGH = {HIGH}")
    print(f"LOW = {LOW}")
    print(f"HIGH is truthy: {bool(HIGH)}")
    print(f"LOW is falsy: {bool(LOW)}")
    
    # Test in conditionals
    if HIGH:
        print("HIGH evaluated as True in if statement")
    
    if not LOW:
        print("LOW evaluated as False in if statement")
    
    # Test GPIO Directions
    print("\n2. GPIO Direction Testing:")
    print(f"OUTPUT = {OUTPUT}")
    print(f"INPUT = {INPUT}")
    print(f"OUTPUT is truthy: {bool(OUTPUT)}")
    print(f"INPUT is falsy: {bool(INPUT)}")
    
    # Test GPIO Pull
    print("\n3. GPIO Pull Testing:")
    print(f"PULLUP = {PULLUP}")
    print(f"PULLDOWN = {PULLDOWN}")
    print(f"PULL_NONE = {PULL_NONE}")
    print(f"PULLUP is truthy: {bool(PULLUP)}")
    print(f"PULLDOWN is falsy: {bool(PULLDOWN)}")
    print(f"PULL_NONE is falsy: {bool(PULL_NONE)}")
    
    # Test Connection State
    print("\n4. Connection State Testing:")
    print(f"CONNECTED = {CONNECTED}")
    print(f"DISCONNECTED = {DISCONNECTED}")
    
    # Test comparisons
    print("\n5. Comparison Testing:")
    print(f"HIGH == True: {HIGH == True}")
    print(f"LOW == False: {LOW == False}")
    print(f"OUTPUT == True: {OUTPUT == True}")
    print(f"INPUT == False: {INPUT == False}")

# Advanced: Factory function approach
def create_boolean_type(true_name, false_name, type_name="CustomBool"):
    """Factory function to create custom boolean types"""
    
    class CustomBoolType:
        def __init__(self, value):
            self._value = bool(value)
        
        def __bool__(self):
            return self._value
        
        def __str__(self):
            return true_name if self._value else false_name
        
        def __repr__(self):
            return f"{type_name}({self._value})"
        
        def __eq__(self, other):
            if isinstance(other, CustomBoolType):
                return self._value == other._value
            return self._value == bool(other)
    
    # Return the true and false instances
    return CustomBoolType(True), CustomBoolType(False)

# Example usage of factory function
def test_factory():
    print("\n=== Testing Factory Function ===")
    
    # Create voltage level types
    VOLTAGE_HIGH, VOLTAGE_LOW = create_boolean_type("3.3V", "0V", "VoltageLevel")
    print(f"Voltage high: {VOLTAGE_HIGH}")
    print(f"Voltage low: {VOLTAGE_LOW}")
    
    # Create enable/disable types
    ENABLED, DISABLED = create_boolean_type("ENABLED", "DISABLED", "EnableState")
    print(f"Feature enabled: {ENABLED}")
    print(f"Feature disabled: {DISABLED}")

if __name__ == "__main__":
    test_custom_types()
    test_factory() 