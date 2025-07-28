# Test script for the new raw hardware API functions
# Demonstrates send_raw() and switch_slot() functionality

print("Testing new raw hardware API functions...")

# Test switch_slot function
print("\n--- Slot Management Tests ---")

# Check current slot
current = CURRENT_SLOT()
print(f"Starting in slot: {current}")

# Test switching to different slots
print("Switching to slot 1...")
previous = switch_slot(1)
print(f"Switched from slot {previous} to slot 1")

print("Switching to slot 2...")
previous = switch_slot(2)
print(f"Switched from slot {previous} to slot 2")

# Switch back to original slot
print(f"Switching back to original slot {current}...")
switch_slot(current)
print(f"Back to original slot {current}")

print("Slot switching: SUCCESS")

# Test send_raw function with different chip formats
print("\n--- Raw Hardware Control Tests ---")

# Test with integer chip numbers (0-11) - default setOrClear=1 (set)
print("Testing with integer chip numbers (set paths):")
send_raw(0, 5, 10)       # Chip 0, x=5, y=10, set path
send_raw(1, 3, 7, 1)     # Chip 1, x=3, y=7, explicitly set path
send_raw(11, 15, 0)      # Chip 11, x=15, y=0, set path

# Test clearing paths (setOrClear=0)
print("Testing clearing paths:")
send_raw(0, 5, 10, 0)    # Clear the path we just set

# Test with letter chip identifiers (A-L)
print("Testing with letter chip identifiers:")
send_raw("A", 2, 8)      # Chip A (same as 0), x=2, y=8, set path
send_raw("B", 12, 4, 1)  # Chip B (same as 1), x=12, y=4, explicitly set 
send_raw("L", 6, 14, 0)  # Chip L (same as 11), x=6, y=14, clear path

# Test with lowercase letters
print("Testing with lowercase letter chip identifiers:")
send_raw("a", 1, 1)      # Chip a (same as A/0)
send_raw("c", 9, 11, 1)  # Chip c (same as C/2), set path
send_raw("l", 0, 15, 0)  # Chip l (same as L/11), clear path

print("Raw hardware control: SUCCESS")

# Test error handling
print("\n--- Error Handling Tests ---")

try:
    send_raw(12, 5, 5)    # Invalid chip number (>11)
    print("ERROR: Should have failed with invalid chip")
except:
    print("✓ Correctly rejected invalid chip number 12")

try:
    send_raw("M", 5, 5)   # Invalid chip letter (>L)
    print("ERROR: Should have failed with invalid chip letter")
except:
    print("✓ Correctly rejected invalid chip letter 'M'")

try:
    switch_slot(-1)       # Invalid slot number
    print("ERROR: Should have failed with invalid slot")
except:
    print("✓ Correctly rejected invalid slot number -1")

print("\n--- Usage Examples ---")
print("send_raw() accepts multiple chip formats and optional set/clear:")
print("  send_raw(0, x, y)        # Integer 0-11, default set path")
print("  send_raw('A', x, y, 1)   # Uppercase A-L, set path") 
print("  send_raw('a', x, y, 0)   # Lowercase a-l, clear path")
print("  send_raw('5', x, y)      # String numbers '0'-'11'")
print("  # setOrClear: 1=set path (default), 0=clear path")
print("")
print("switch_slot() changes active slot:")
print("  old_slot = switch_slot(3)  # Returns previous slot")
print("  current = CURRENT_SLOT()   # Check current slot")

print("\n--- All raw hardware API tests completed! ---") 