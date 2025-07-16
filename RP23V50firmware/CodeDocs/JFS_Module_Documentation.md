













# JFS (Jumperless FileSystem) Module

The `jfs` module is basically like MicroPython's `vfs` and parts of `os`, but kinda written in a style that's probably more familiar to Arduino-style C++ people (me). It uses the *almost* standardized API shared by [`FatFS` (the one Jumperless actually uses), `LittleFS`, and `SDFS`](https://arduino-pico.readthedocs.io/en/latest/fs.html#file-system-object-littlefs-sd-sdfs-fatfs), but still has been *Pythonified* to use types that are easier to work with in MicroPython.

## Module Usage

To use the module, import it like any other Python module:

```python
import jfs  #you don't actually need this, jfs is imported globally by default

# List files in the root directory
files = jfs.listdir('/')   # returns a python list
print(files)
```
Output:
```python
['config.txt', 'nodeFileSlot0.txt', 'python_scripts/', 'nodeFileSlot1.txt', 'nodeFileSlot2.txt', 'nodeFileSlot3.txt', 'nodeFileSlot4.txt', 'nodeFileSlot5.txt', 'nodeFileSlot6.txt', 'nodeFileSlot7.txt', 'net_colors/']
```
<details>

<summary>If you want to make this print with subdirectories</summary>

```python
files = jfs.listdir('/')

for file in range(len(files)):

    print(files[file])

    if files[file].endswith("/"):
        subdir = jfs.listdir(files[file])

        for i in range(len(subdir)):
            print("     - " + subdir[i])
```

Output: 
```config.txt
nodeFileSlot0.txt
python_scripts/
     - history.txt
     - examples/
     - fake_gpio.py
     - _temp_repl_edit.py
     - Hey.txt
     - script_1.py
     - script_2.py
     - script_3.py
     - log.txt
     - lib/
     - script_4.py
     - pathtest.py
nodeFileSlot1.txt
nodeFileSlot2.txt
nodeFileSlot3.txt
nodeFileSlot4.txt
nodeFileSlot5.txt
nodeFileSlot6.txt
nodeFileSlot7.txt
net_colors/
    - netColorsSlot0.txt
```

</details>

## File API 

File objects returned by `jfs.open()` support method calls directly on the object:

```python
# Object-oriented file operations

# Write-only mode
f = jfs.open('hello.txt', 'w')
f.write('Hello, Jumperless!')
f.close()

# Read from the file (need to reopen or use w+/r+ mode)
f = jfs.open('hello.txt', 'r')
content = f.read()             # Read from file object
size = f.size()                # Get file size
f.close()

# Read-write mode (truncates file)
f = jfs.open('hello.txt', 'w+')
f.write('Hello, Jumperless!')
f.seek(0)                      # Seek to beginning to read what we wrote
content = f.read()             # Now this works!
f.close()

# Context manager support (automatically closes file)
with jfs.open('data.txt', 'w+') as f:
    f.write('This file will be automatically closed')
    f.seek(0)                  # Reset to beginning
    content = f.read()         # Read back what we wrote
    pos = f.tell()             # Get current position
    name = f.name()            # Get file name
```
<details>

<summary>Other ways to do the same thing</summary>

### 2. Module-Level Functions

You can also use module-level functions with file handles:

```python
f = jfs.open('hello.txt', 'w')
jfs.write(f, 'Hello, Jumperless!')  # Module-level function
jfs.seek(f, 0)                      # Module-level function  
content = jfs.read(f)               # Module-level function
jfs.close(f)                        # Module-level function
```

### 3. Direct String Operations (For simple cases)

```python
# Write/read entire files at once (no file handles needed)
jfs.write('config.txt', 'key=value\nother=setting')
content = jfs.read('config.txt')
```
</summary>
</details>

## File Modes

When using `jfs.open(path, mode)`, the following modes are supported:

| Mode | Description | Read | Write | Create |
|------|-------------|------|-------|---------|
| `'r'` | Read only | ✅ | ❌ | ❌ |
| `'w'` | Write only | ❌ | ✅ | ✅ |
| `'a'` | Append only | ❌ | ✅ | ✅ |
| `'r+'` | Read + Write | ✅ | ✅ | ❌ |
| `'w+'` | Read + Write | ✅ | ✅ | ✅ |
| `'a+'` | Read + Append | ✅ | ✅ | ✅ |

**Important:** You cannot read from a file opened in write-only mode (`'w'` or `'a'`). Use `'w+'`, `'r+'`, or `'a+'` if you need both read and write access.

## Open a file for writing

```python
f = jfs.open('hello.txt', 'w')
f.write('Hello, Jumperless!')  # Now works with object-oriented API!
f.close()
```

## Directory Operations

### `jfs.listdir(path)`
Returns a list containing the names of the entries in the directory given by `path`.

*   `path` (str): The path to the directory.

**Example:**
```python
# List contents of the root directory
print(jfs.listdir('/'))

# List contents of a subdirectory
jfs.mkdir('/my_dir')
print(jfs.listdir('/my_dir'))
```

### `jfs.mkdir(path)`
Create a new directory.

*   `path` (str): The path of the new directory.

### `jfs.rmdir(path)`
Remove an empty directory.

*   `path` (str): The path of the directory to remove.

### `jfs.remove(path)`
Remove a file.

*   `path` (str): The path of the file to remove.

### `jfs.rename(old_path, new_path)`
Rename a file or directory.

*   `old_path` (str): The current path.
*   `new_path` (str): The new path.

### `jfs.exists(path)`
Check if a file or directory exists.

*   `path` (str): The path to check.
*   Returns `True` if it exists, `False` otherwise.

### `jfs.stat(path)`
Get status of a file or directory.

*   `path` (str): The path of the file or directory.
*   Returns a tuple with file information (mode, size, etc.), similar to `os.stat()`.

## Filesystem Information

### `jfs.info()`
Get information about the filesystem.

*   Returns a tuple `(total_bytes, used_bytes, free_bytes)`.

**Example:**
```python
total, used, free = jfs.info()
print("Filesystem Size: " + str(total / 1024) + " KB")
print("Used: " + str(used / 1024) + " KB")
print("Free: " + str(free / 1024) + " KB")
```

## File I/O

The `jfs` module supports standard file opening and handling using `jfs.open()` and file objects, including support for the `with` statement for automatic resource management.

### `jfs.open(path, mode='r')`
Open a file and return a corresponding file object.

*   `path` (str): The path to the file.
*   `mode` (str, optional): The mode in which the file is opened. Defaults to `'r'`.
    *   `'r'`: Read (default).
    *   `'w'`: Write (creates a new file or truncates an existing one).
    *   `'a'`: Append.
    *   `'r+'`: Read and write.
    *   `'w+'`: Write and read (creates/truncates).
    *   `'a+'`: Append and read.

**Example:**
```python
# Open a file for reading
f = jfs.open('config.txt', 'r')
content = f.read()
f.close()

# Use 'with' for automatic closing
with jfs.open('data.log', 'a') as log_file:
    log_file.write('New log entry.\\n')
```

## File Object Methods

The file object returned by `jfs.open()` has the following methods:

### `file.read([size])`
Read `size` bytes from the file. If `size` is omitted or negative, the entire file is read.

### `file.write(data)`
Write the given string or bytes `data` to the file. Returns the number of bytes written.

### `file.close()`
Close the file. A closed file cannot be read or written to.

### `file.seek(offset, [whence])`
Change the stream position.
*   `offset`: The byte offset.
*   `whence` (optional):
    *   `0`: Seek from the start of the stream (default). Use `jfs.SEEK_SET`.
    *   `1`: Seek from the current position. Use `jfs.SEEK_CUR`.
    *   `2`: Seek from the end of the stream. Use `jfs.SEEK_END`.

### `file.tell()`
Return the current stream position.
*   **Aliases**: `file.position()`

### `file.size()`
Return the total size of the file in bytes.

### `file.available()`
Return the number of bytes available to be read from the current position to the end of the file.

### `file.name`
Returns the name of the file.

---

## Module-Level File Operations

For convenience, the `jfs` module also provides functions that operate directly on file handles returned by `jfs.open()`. This can be useful in some scripting scenarios but using file object methods is generally preferred for clarity.

*   `jfs.read(file_handle, [size])`
*   `jfs.write(file_handle, data)`
*   `jfs.close(file_handle)`
*   `jfs.seek(file_handle, offset, [whence])`
*   `jfs.tell(file_handle)`
*   `jfs.size(file_handle)`
*   `jfs.available(file_handle)`

**Example:**
```python
file_handle = jfs.open('temp.txt', 'w')
jfs.write(file_handle, 'some data')
jfs.close(file_handle)
```
