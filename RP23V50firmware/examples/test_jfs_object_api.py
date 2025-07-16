#!/usr/bin/env python3
"""
Test script for JFS object-oriented API

This script tests that file objects returned by jfs.open() have
working methods like .write(), .read(), .seek(), etc.
"""

import jfs

def test_jfs_object_api():
    """Test the object-oriented JFS API"""
    
    print("Testing JFS object-oriented API...")
    
    # Test 1: Basic write using object method
    print("\n=== Test 1: Object-oriented write ===")
    try:
        f = jfs.open('test_object_api.txt', 'w')
        print("File opened successfully in write mode")
        
        # This should now work with the object-oriented API
        bytes_written = f.write('Hello, Jumperless!')
        print('Wrote using f.write():', bytes_written, 'bytes')
        
        f.close()
        print("File closed successfully")
    except Exception as e:
        print("Error in write test:", e)
    
    # Test 2: Basic read using object method  
    print("\n=== Test 2: Object-oriented read ===")
    try:
        f = jfs.open('test_object_api.txt', 'r')
        print("File opened for reading")
        
        # This should now work with the object-oriented API
        content = f.read()
        print('Read using f.read():', repr(content))
        
        f.close()
        print("File closed successfully")
    except Exception as e:
        print("Error in read test:", e)
    
    # Test 2b: Read-write mode demonstration
    print("\n=== Test 2b: Read-write mode (w+) ===")
    try:
        f = jfs.open('test_rw_mode.txt', 'w+')
        print("File opened in w+ mode (read-write, truncates)")
        
        # Write some content
        f.write('Read-write mode test!')
        print("Wrote content")
        
        # Seek back to beginning to read what we wrote
        f.seek(0)
        print("Seeked to beginning")
        
        # Now read it back
        content = f.read()
        print('Read back content:', repr(content))
        
        f.close()
        print("File closed successfully")
        
        # Clean up this test file
        jfs.remove('test_rw_mode.txt')
    except Exception as e:
        print("Error in read-write test:", e)
    
    # Test 3: Seek and tell using object methods
    print("\n=== Test 3: Object-oriented seek/tell ===")
    try:
        f = jfs.open('test_object_api.txt', 'r')
        print("File opened for seek/tell test")
        
        # Test tell (get current position)
        pos = f.tell()
        print('Current position using f.tell():', pos)
        
        # Test seek
        result = f.seek(7)  # Seek to position 7
        print('Seek result using f.seek(7):', result)
        
        # Read from new position
        content = f.read(10)  # Read 10 characters
        print('Read after seek using f.read(10):', repr(content))
        
        f.close()
        print("File closed successfully")
    except Exception as e:
        print("Error in seek/tell test:", e)
    
    # Test 4: File size and available using object methods
    print("\n=== Test 4: Object-oriented size/available ===")
    try:
        f = jfs.open('test_object_api.txt', 'r')
        print("File opened for size test")
        
        # Test size
        size = f.size()
        print('File size using f.size():', size)
        
        # Test available
        available = f.available()
        print('Available bytes using f.available():', available)
        
        # Test name
        name = f.name()
        print('File name using f.name():', repr(name))
        
        f.close()
        print("File closed successfully")
    except Exception as e:
        print("Error in size/available test:", e)
    
    # Test 5: Context manager (with statement)
    print("\n=== Test 5: Context manager support ===")
    try:
        with jfs.open('test_context.txt', 'w+') as f:
            print("File opened using 'with' statement in w+ mode")
            f.write('Context manager test!')
            print("Wrote using f.write() in context manager")
            
            # Read back in the same context (since we used w+ mode)
            f.seek(0)
            content = f.read()
            print('Read back in same context:', repr(content))
        print("File automatically closed by context manager")
        
        # Verify the content was written by opening again
        with jfs.open('test_context.txt', 'r') as f:
            content = f.read()
            print('Content verified by reopening:', repr(content))
            
        # Clean up
        jfs.remove('test_context.txt')
    except Exception as e:
        print("Error in context manager test:", e)
    
    # Cleanup
    print("\n=== Cleanup ===")
    try:
        jfs.remove('test_object_api.txt')
        print("Test file removed successfully")
    except Exception as e:
        print("Error removing test file:", e)
    
    print("\nJFS object-oriented API test completed!")

if __name__ == '__main__':
    test_jfs_object_api() 