# Comprehensive test for full buffer multiline editing
# This test verifies that the entire input buffer is properly synchronized

print("Testing full buffer multiline editing...")

# Test 1: Simple multiline function with editing
def test_function():
    print("This is line 1")
    print("This is line 2")
    print("This is line 3")
    return "function completed"

# Test 2: Complex multiline structure
if True:
    x = 10
    y = 20
    result = x + y
    print(f"Result: {result}")
    if result > 25:
        print("Result is greater than 25")
    else:
        print("Result is 25 or less")

# Test 3: Nested structures
def calculate_values():
    values = []
    for i in range(5):
        if i % 2 == 0:
            values.append(i * 2)
        else:
            values.append(i * 3)
    return values

# Test 4: Dictionary with multiline structure
config = {
    "name": "test_config",
    "settings": {
        "debug": True,
        "verbose": False,
        "max_iterations": 100
    },
    "endpoints": [
        "https://api.example.com/v1",
        "https://api.example.com/v2"
    ]
}

# Test 5: Try-except with multiple clauses
try:
    result = 100 / 5
    print(f"Division successful: {result}")
    if result > 15:
        raise ValueError("Result too large")
except ValueError as e:
    print(f"Value error: {e}")
except ZeroDivisionError:
    print("Cannot divide by zero")
finally:
    print("Cleanup completed")

# Test 6: Class definition
class TestClass:
    def __init__(self, value):
        self.value = value
        self.multiplier = 2
    
    def calculate(self):
        return self.value * self.multiplier
    
    def display(self):
        result = self.calculate()
        print(f"Value: {self.value}, Result: {result}")

# Test 7: List comprehension multiline
numbers = [
    x * 2 
    for x in range(10) 
    if x % 2 == 0
]

# Test 8: String concatenation multiline
message = ("This is a very long message that spans " +
          "multiple lines to test how the buffer " +
          "handles line breaks and editing")

print("Full buffer multiline editing test completed!")
print("Try editing lines in the middle of any multiline block")
print("The entire buffer should stay synchronized")
print("Use arrow keys to navigate and edit any part") 