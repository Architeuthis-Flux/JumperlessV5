# Test script specifically for line clearing fix
# This verifies that navigation doesn't cause unwanted screen scrolling

print("Testing line clearing fix...")

# Test Case 1: Simple multiline - navigate up and down
def simple_test():
    print("Line 1")
    print("Line 2")
    print("Line 3")

# Test Case 2: Medium complexity
if True:
    x = 10
    y = 20
    result = x + y
    print(f"Result: {result}")

# Test Case 3: More lines to test scrolling behavior
def longer_function():
    print("This is line 1")
    print("This is line 2")
    print("This is line 3")
    print("This is line 4")
    print("This is line 5")
    print("This is line 6")
    return "done"

print("Line clearing test complete!")
print("")
print("INSTRUCTIONS FOR TESTING:")
print("1. Type any of the above multiline blocks")
print("2. Use UP/DOWN arrow keys to navigate between lines")
print("3. The display should NOT scroll up the screen")
print("4. Each line should stay in its correct position")
print("5. No extra lines should be erased")
print("")
print("This test verifies the line clearing fix where")
print("last_displayed_lines now correctly tracks lines_printed")
print("instead of newline count to prevent over-clearing.") 