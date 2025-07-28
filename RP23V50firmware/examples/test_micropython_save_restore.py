# Test script to verify MicroPython save/restore behavior
# This tests that connections made in MicroPython persist after exiting

print("Testing MicroPython save/restore behavior...")

# Before testing, clear any existing connections
nodes_clear()
print("Cleared all existing connections")

# Make some connections that should persist
print("\nMaking test connections...")
connect(1, 30)
connect(2, 31) 
connect(3, 32)

print("✓ Connected nodes 1-30, 2-31, 3-32")

# Verify connections were made
print("\nVerifying connections...")
print(f"1 → 30: {is_connected(1, 30)}")
print(f"2 → 31: {is_connected(2, 31)}")
print(f"3 → 32: {is_connected(3, 32)}")

# Save the current state to ensure persistence
print("\nSaving current state...")
nodes_save()
print("✓ Saved connections to persistent storage")

print("\n" + "="*50)
print("✓ TEST SETUP COMPLETE")
print("="*50)
print("Now exit MicroPython REPL (type 'exit()') and re-enter.")
print("The connections should still be there when you return!")
print("Use this script to verify after re-entering:")
print("")
print("# Verification script:")
print("print('Checking if connections survived REPL exit...')")
print("print(f'1 → 30: {is_connected(1, 30)}')")
print("print(f'2 → 31: {is_connected(2, 31)}')")  
print("print(f'3 → 32: {is_connected(3, 32)}')")
print("if is_connected(1, 30) and is_connected(2, 31) and is_connected(3, 32):")
print("    print('✓ SUCCESS: All connections survived!')")
print("else:")
print("    print('✗ FAILED: Some connections were lost')")
print("")
print("Type 'exit()' to test the save/restore behavior!") 