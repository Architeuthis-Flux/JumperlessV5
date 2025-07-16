#!/usr/bin/env python3
"""
Quick fix example for JFS file mode issue

This shows the solution for the OSError when trying to read 
from a file opened in write-only mode.
"""

import jfs

print("=== PROBLEM: OSError when reading from write-only file ===")
print("This will cause an error:")
print("f = jfs.open('hello.txt', 'w')  # Write-only mode")
print("f.write('Hello, Jumperless!')")
print("f.seek(0)")
print("content = f.read()  # ERROR: Can't read from write-only file!")
print()

print("=== SOLUTION 1: Close and reopen ===")
try:
    # Write the file
    f = jfs.open('hello.txt', 'w')
    f.write('Hello, Jumperless!')
    f.close()
    print("✅ Wrote and closed file")
    
    # Open again for reading
    f = jfs.open('hello.txt', 'r')
    content = f.read()
    f.close()
    print("✅ Read content:", repr(content))
except Exception as e:
    print("❌ Error:", e)

print()
print("=== SOLUTION 2: Use w+ mode (read-write) ===")
try:
    # Open in read-write mode
    f = jfs.open('hello.txt', 'w+')
    f.write('Hello, Jumperless!')
    f.seek(0)  # Go back to beginning
    content = f.read()
    size = f.size()
    f.close()
    print("✅ Read content:", repr(content))
    print("✅ File size:", size)
except Exception as e:
    print("❌ Error:", e)

print()
print("=== SOLUTION 3: Context manager with w+ ===")
try:
    with jfs.open('hello.txt', 'w+') as f:
        f.write('Hello, Jumperless!')
        f.seek(0)
        content = f.read()
        print("✅ Content from context manager:", repr(content))
except Exception as e:
    print("❌ Error:", e)

# Cleanup
try:
    jfs.remove('hello.txt')
    print("✅ Cleaned up test file")
except:
    pass

print()
print("=== FILE MODE SUMMARY ===")
print("'r'   - Read only")
print("'w'   - Write only (truncates file)")
print("'w+'  - Read + Write (truncates file) ← USE THIS for read/write")
print("'r+'  - Read + Write (preserves existing content)")
print("'a+'  - Read + Append (preserves existing content)") 