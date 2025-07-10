# Test script for multiline editing in Jumperless REPL
# This script tests various multiline editing scenarios

print("Testing multiline editing functionality...")

# Test 1: Simple multiline function
def test_function():
    print("Line 1")
    print("Line 2")
    print("Line 3")
    return "done"

# Test 2: Complex multiline with editing
def complex_function(x, y):
    if x > 0:
        result = x * y
        print(f"Result: {result}")
    else:
        result = 0
        print("Zero result")
    return result

# Test 3: List comprehension multiline
numbers = [
    i * 2 
    for i in range(10) 
    if i % 2 == 0
]

# Test 4: Dictionary multiline
config = {
    "name": "test",
    "value": 42,
    "enabled": True
}

# Test 5: Try-except multiline
try:
    result = 10 / 2
    print(f"Division result: {result}")
except ZeroDivisionError:
    print("Division by zero error")

print("Multiline editing test completed!")
print("Try editing lines in the middle of multiline blocks")
print("Use arrow keys to navigate and edit") 