# Test script for MicroPython session management
# Demonstrates entry state backup and change tracking

print("Testing MicroPython session management...")

# Clear everything first
nodes_clear()
print("✓ Cleared all connections")

# Check if we have unsaved changes (should be False after clear)
print(f"Has unsaved changes: {nodes_has_changes()}")

# Make some initial connections that will be our "entry state"
print("\nMaking initial connections...")
connect(1, 30)
connect(2, 31)
print("✓ Made initial connections: 1-30, 2-31")

# Save these as our persistent state
nodes_save()
print("✓ Saved to persistent storage")

print("\n" + "="*50)
print("EXIT AND RE-ENTER MICROPYTHON NOW")
print("This will establish the 'entry state'")
print("="*50)
print("\nAfter re-entering, run this test:")
print("")
print("# Check entry state")
print("print(f'Has changes: {nodes_has_changes()}')  # Should be False")
print("")
print("# Make some changes")
print("connect(3, 32)")
print("connect(4, 33)")
print("print(f'Has changes: {nodes_has_changes()}')  # Should be True")
print("")
print("# Check current connections")
print("print(f'1-30: {is_connected(1, 30)}')  # Should be CONNECTED")
print("print(f'3-32: {is_connected(3, 32)}')  # Should be CONNECTED")
print("")
print("# Discard all changes made since entering REPL")
print("nodes_discard()")
print("print(f'Has changes: {nodes_has_changes()}')  # Should be False")
print("print(f'1-30: {is_connected(1, 30)}')  # Should be CONNECTED (preserved)")
print("print(f'3-32: {is_connected(3, 32)}')  # Should be DISCONNECTED (discarded)")
print("")
print("Type 'exit()' to test the entry state backup system!") 