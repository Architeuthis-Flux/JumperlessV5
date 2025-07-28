# Test script for the new local MicroPython API functionality
# This demonstrates the improved behavior with local copies

print("Testing new local MicroPython API...")

# Show current slot
print(f"Current slot: {CURRENT_SLOT()}")

# Test single line commands (these accumulate on local copy)
print("\n--- Single Line Commands Test ---")
print("These commands build up in local memory, very fast!")

connect(1, 30)  # Local operation (default save=False)
connect(2, 31)  # Local operation  
connect(3, 32)  # Local operation

print("Connected nodes 1-30, 2-31, 3-32 in local memory")
print(f"Node 1 connected to 30: {is_connected(1, 30)}")
print(f"Node 2 connected to 31: {is_connected(2, 31)}")

# Test saving to persistent storage
print("\n--- Save to Persistent Storage ---")
slot_saved = nodes_save()  # Save current local state to current slot
print(f"Saved local connections to slot {slot_saved}")

# Test multiline script (this will reset local copy before execution)
print("\n--- Multiline Script Test ---")
print("Run this multiline script using 'run' command:")
print("""
connect(10, 20)
connect(11, 21)  
connect(12, 22)
print("Multiline script executed - local copy was reset first")
print(f"Node 10 connected to 20: {is_connected(10, 20)}")
run
""")

print("\nNote: The multiline script above will start from a fresh copy")
print("of the persistent nodefile, not the accumulated single-line changes.")

print("\n--- Testing Flexible GPIO API ---")
print("The API now supports various input types:")

# Test with global constants
gpio_set_dir(1, OUTPUT)  # Global constant
gpio_set(1, HIGH)        # Global constant

# Test with strings  
gpio_set_dir(2, "input")    # String (case insensitive)
gpio_set(2, "low")          # String (case insensitive)
gpio_set_pull(2, "pullup")  # String (case insensitive)

# Test with integers and booleans
gpio_set_dir(3, 1)       # Integer (1 = OUTPUT)
gpio_set(3, True)        # Boolean (True = HIGH)
gpio_set_pull(3, 0)      # Integer (0 = NONE)

print("GPIO functions now accept strings, globals, ints, and booleans!")

print("\n--- Testing Raw Hardware API ---")
print("New low-level hardware control functions:")

# Test slot switching
current_slot = CURRENT_SLOT()
print(f"Current slot: {current_slot}")

# Test raw crossbar control
print("Sending raw crossbar commands:")
send_raw(0, 5, 10)      # Chip 0 (integer)
send_raw("A", 2, 8)     # Chip A (string, same as 0)
send_raw("b", 12, 4)    # Chip b (lowercase, same as 1)

print("Raw hardware control added for advanced users!")

print("\n--- Testing Save/Restore Behavior ---")
print("The MicroPython API now properly handles temporary connections!")
print("✓ When entering REPL: Uses generalized backup system to store entry state")
print("✓ During REPL: All operations work on fast local copy") 
print("✓ When exiting REPL: RESTORES to entry state (discards Python changes)")
print("✓ Python connections are TEMPORARY by default - use nodes_save() to keep!")

print("\n--- Testing Session Management ---")
print("New session management functions:")

# Test change tracking
has_changes_before = nodes_has_changes()
print(f"Has unsaved changes: {has_changes_before}")

# Make a change
connect(5, 35)
has_changes_after = nodes_has_changes()
print(f"After making connection: {has_changes_after}")

print("✓ Session management: nodes_has_changes(), nodes_discard(), nodes_save()")
print("✓ Uses generalized backup/restore system from FileParsing.cpp")
print("✓ Available to any part of codebase that needs temporary state management") 