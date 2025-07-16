# File System Guide

Comprehensive guide to file system operations on Jumperless, covering both basic functions and the advanced JFS module.

## Overview

Jumperless provides multiple interfaces for file system operations:

1. **Basic FS Functions** - Simple file operations via `jumperless.fs_*` functions
2. **JFS Module** - Complete filesystem API with advanced features
3. **File Manager** - Interactive file browser via terminal command `/`
4. **USB Mass Storage** - Direct computer access to files

## Quick Start

```python
import jumperless

# Basic operations
jumperless.fs_write("hello.txt", "Hello Jumperless!")
content = jumperless.fs_read("hello.txt")
print(content)  # Outputs: Hello Jumperless!

# Check if file exists
if jumperless.fs_exists("config.json"):
    config = jumperless.fs_read("config.json")

# List directory contents
files = jumperless.fs_listdir("/")
print("Root files:", files)
```

---

## Basic File System Functions

Simple filesystem operations available through the main `jumperless` module.

### File Operations

#### `fs_exists(path)`
Check if file or directory exists.

```python
# Check files
exists = jumperless.fs_exists("script.py")
print(f"Script exists: {exists}")

# Check directories
exists = jumperless.fs_exists("/python_scripts")
print(f"Scripts directory exists: {exists}")
```

#### `fs_read(path)`
Read entire file content as string.

```python
# Read text file
content = jumperless.fs_read("config.txt")
print("Config contents:", content)

# Read Python script
script = jumperless.fs_read("/python_scripts/test.py")
```

#### `fs_write(path, content)`
Write string content to file (overwrites existing).

```python
# Write simple text
jumperless.fs_write("output.txt", "Test data")

# Write Python script
script_code = '''
import jumperless
jumperless.oled_print("Script running!")
'''
jumperless.fs_write("/python_scripts/auto_run.py", script_code)

# Write configuration
config = "debug=true\nverbose=false\n"
jumperless.fs_write("settings.conf", config)
```

#### `fs_listdir(path)`
List directory contents as comma-separated string.

```python
# List root directory
files = jumperless.fs_listdir("/")
file_list = files.split(",") if files else []
print("Root files:", file_list)

# List python scripts
scripts = jumperless.fs_listdir("/python_scripts")
if scripts:
    script_list = scripts.split(",")
    python_files = [f for f in script_list if f.endswith('.py')]
    print("Python scripts:", python_files)
```

#### `fs_cwd()`
Get current working directory.

```python
current_dir = jumperless.fs_cwd()
print(f"Current directory: {current_dir}")
```

### Practical Examples

#### Configuration Manager
```python
def load_config(filename="config.txt"):
    """Load configuration from file"""
    if not jumperless.fs_exists(filename):
        # Create default config
        default_config = """# Jumperless Configuration
debug=false
led_brightness=50
oled_timeout=30
"""
        jumperless.fs_write(filename, default_config)
        print(f"Created default config: {filename}")
    
    config = {}
    content = jumperless.fs_read(filename)
    
    for line in content.split('\n'):
        line = line.strip()
        if line and not line.startswith('#'):
            if '=' in line:
                key, value = line.split('=', 1)
                config[key.strip()] = value.strip()
    
    return config

def save_config(config, filename="config.txt"):
    """Save configuration to file"""
    lines = ["# Jumperless Configuration"]
    for key, value in config.items():
        lines.append(f"{key}={value}")
    
    content = '\n'.join(lines)
    jumperless.fs_write(filename, content)
    print(f"Config saved to {filename}")

# Usage
config = load_config()
print("Loaded config:", config)

config['debug'] = 'true'
config['led_brightness'] = '75'
save_config(config)
```

#### Script Library Manager
```python
def list_python_scripts():
    """List all Python scripts"""
    if not jumperless.fs_exists("/python_scripts"):
        print("No python_scripts directory found")
        return []
    
    files = jumperless.fs_listdir("/python_scripts")
    if not files:
        return []
    
    scripts = [f for f in files.split(',') if f.endswith('.py')]
    return scripts

def run_script(script_name):
    """Run a Python script by name"""
    script_path = f"/python_scripts/{script_name}"
    
    if not jumperless.fs_exists(script_path):
        print(f"Script not found: {script_name}")
        return False
    
    try:
        script_content = jumperless.fs_read(script_path)
        exec(script_content)
        print(f"✓ Script {script_name} completed")
        return True
    except Exception as e:
        print(f"✗ Script {script_name} failed: {e}")
        return False

def create_script(script_name, script_content):
    """Create a new Python script"""
    script_path = f"/python_scripts/{script_name}"
    
    # Ensure directory exists (simplified check)
    if not jumperless.fs_exists("/python_scripts"):
        print("python_scripts directory not found")
        return False
    
    jumperless.fs_write(script_path, script_content)
    print(f"✓ Created script: {script_name}")
    return True

# Example usage
scripts = list_python_scripts()
print("Available scripts:", scripts)

# Create a simple test script
test_script = '''
import jumperless
import time

print("Test script starting...")
jumperless.oled_print("Test Running")
jumperless.gpio_set_dir(1, True)
jumperless.gpio_set(1, True)
time.sleep(1)
jumperless.gpio_set(1, False)
print("Test script complete!")
'''

create_script("test_gpio.py", test_script)
run_script("test_gpio.py")
```

---

## JFS Module - Advanced File System

The JFS (Jumperless FileSystem) module provides comprehensive file system operations with Python-like file handling.

### Module Import

```python
# Import JFS module
import jfs

# JFS is also available through jumperless module
import jumperless
jfs = jumperless.jfs
```

### File Handle Operations

JFS provides both simple functions and file handle operations for advanced use.

#### Opening Files

```python
# Open file for reading
file = jfs.open("data.txt", "r")
content = jfs.read(file)
jfs.close(file)

# Open file for writing
file = jfs.open("output.txt", "w")
jfs.write(file, "Hello World!")
jfs.close(file)

# Open file for appending
file = jfs.open("log.txt", "a")
jfs.write(file, "New log entry\n")
jfs.close(file)
```

#### File Operations

```python
# Read operations
file = jfs.open("large_file.dat", "r")

# Read specific number of bytes
data = jfs.read(file, 100)  # Read 100 bytes

# Check file position
pos = jfs.tell(file)
print(f"Current position: {pos}")

# Get file size
size = jfs.size(file)
print(f"File size: {size} bytes")

# Check bytes available
available = jfs.available(file)
print(f"Bytes available: {available}")

# Seek to position
jfs.seek(file, 50, 0)  # Seek to byte 50 from start
jfs.seek(file, 10, 1)  # Seek 10 bytes from current position
jfs.seek(file, -20, 2) # Seek 20 bytes before end

jfs.close(file)
```

#### Write Operations

```python
# Write operations
file = jfs.open("data.bin", "w")

# Write string data
bytes_written = jfs.write(file, "Binary data here")
print(f"Wrote {bytes_written} bytes")

# Get current position
pos = jfs.tell(file)

# Get file name
name = jfs.name(file)
print(f"Writing to: {name}")

jfs.close(file)
```

### Directory Operations

```python
# Create directory
success = jfs.mkdir("/new_directory")
if success:
    print("Directory created")

# Create nested directories (may need multiple calls)
jfs.mkdir("/data")
jfs.mkdir("/data/sensors")
jfs.mkdir("/data/logs")

# Remove empty directory
success = jfs.rmdir("/empty_directory")

# Remove file
success = jfs.remove("/old_file.txt")

# Rename/move file
success = jfs.rename("/old_name.txt", "/new_name.txt")
success = jfs.rename("/file.txt", "/backup/file.txt")  # Move to subdirectory
```

### Filesystem Information

```python
# Get filesystem statistics
total, used, free = jfs.info()
print(f"Total space: {total} bytes")
print(f"Used space: {used} bytes")
print(f"Free space: {free} bytes")
print(f"Usage: {used/total*100:.1f}%")
```

### Simple JFS Functions

For convenience, JFS also provides simple functions similar to the basic FS functions:

```python
# Simple read/write (like fs_ functions)
jfs.write("simple.txt", "Simple content")
content = jfs.read("simple.txt")
print(content)

# Check existence
exists = jfs.exists("simple.txt")

# List directory
files = jfs.listdir("/")
```

### Advanced File Operations

#### Binary File Handling

```python
def save_binary_data(filename, data_array):
    """Save array of numbers as binary file"""
    file = jfs.open(filename, "w")
    
    # Convert numbers to bytes (simplified)
    for value in data_array:
        # Convert to string and write (real binary would use struct)
        jfs.write(file, str(value) + "\n")
    
    jfs.close(file)
    print(f"Saved {len(data_array)} values to {filename}")

def load_binary_data(filename):
    """Load binary data as array"""
    if not jfs.exists(filename):
        return []
    
    file = jfs.open(filename, "r")
    data_array = []
    
    # Read entire file
    content = jfs.read(file)
    
    # Parse lines
    for line in content.split('\n'):
        line = line.strip()
        if line:
            try:
                value = float(line)
                data_array.append(value)
            except ValueError:
                pass
    
    jfs.close(file)
    print(f"Loaded {len(data_array)} values from {filename}")
    return data_array

# Example usage
test_data = [1.5, 2.3, 4.7, 8.2, 3.1]
save_binary_data("sensor_data.bin", test_data)
loaded_data = load_binary_data("sensor_data.bin")
print("Data matches:", test_data == loaded_data)
```

#### Log File Management

```python
def append_log(filename, message, timestamp=True):
    """Append message to log file"""
    if timestamp:
        import time
        # Simple timestamp (MicroPython doesn't have full datetime)
        ts = int(time.time())
        message = f"[{ts}] {message}"
    
    file = jfs.open(filename, "a")
    jfs.write(file, message + "\n")
    jfs.close(file)

def read_log(filename, lines=None):
    """Read log file, optionally last N lines"""
    if not jfs.exists(filename):
        return []
    
    content = jfs.read(filename)
    all_lines = content.strip().split('\n')
    
    if lines is None:
        return all_lines
    else:
        return all_lines[-lines:] if len(all_lines) > lines else all_lines

def clear_log(filename):
    """Clear log file"""
    jfs.write(filename, "")

# Example usage
append_log("system.log", "System started")
append_log("system.log", "Voltage set to 3.3V")
append_log("system.log", "Connection created: D2-A0")

# Read last 10 log entries
recent_logs = read_log("system.log", 10)
for log_entry in recent_logs:
    print(log_entry)
```

#### Configuration File Handler

```python
class ConfigManager:
    def __init__(self, filename="config.json"):
        self.filename = filename
        self.config = {}
        self.load()
    
    def load(self):
        """Load configuration from file"""
        if jfs.exists(self.filename):
            try:
                content = jfs.read(self.filename)
                # Simple JSON parser (MicroPython has limited JSON support)
                self.config = self._parse_simple_json(content)
            except Exception as e:
                print(f"Error loading config: {e}")
                self.config = {}
        else:
            self.config = {}
    
    def save(self):
        """Save configuration to file"""
        try:
            content = self._create_simple_json(self.config)
            jfs.write(self.filename, content)
            return True
        except Exception as e:
            print(f"Error saving config: {e}")
            return False
    
    def get(self, key, default=None):
        """Get configuration value"""
        return self.config.get(key, default)
    
    def set(self, key, value):
        """Set configuration value"""
        self.config[key] = value
    
    def _parse_simple_json(self, content):
        """Simple JSON parser for basic key-value pairs"""
        config = {}
        for line in content.split('\n'):
            line = line.strip()
            if line.startswith('"') and ':' in line:
                # Simple parsing: "key": "value"
                parts = line.split(':', 1)
                if len(parts) == 2:
                    key = parts[0].strip(' "')
                    value = parts[1].strip(' ",')
                    config[key] = value
        return config
    
    def _create_simple_json(self, config):
        """Create simple JSON from config dict"""
        lines = ["{"]
        items = list(config.items())
        for i, (key, value) in enumerate(items):
            comma = "," if i < len(items) - 1 else ""
            lines.append(f'  "{key}": "{value}"{comma}')
        lines.append("}")
        return '\n'.join(lines)

# Example usage
config = ConfigManager("settings.json")
config.set("debug", "true")
config.set("led_brightness", "75")
config.set("voltage_default", "3.3")
config.save()

# Later, load and use config
config = ConfigManager("settings.json")
debug_mode = config.get("debug", "false") == "true"
brightness = int(config.get("led_brightness", "50"))
print(f"Debug: {debug_mode}, Brightness: {brightness}")
```

---

## File Manager Interface

Interactive file management through the terminal.

### Opening File Manager

```bash
/          # Open file manager in current directory
```

### File Manager Commands

| Key | Action |
|-----|--------|
| ↑/↓ | Navigate files |
| Enter | Open file/directory |
| v | View file contents |
| e | Edit with eKilo editor |
| n | Create new file |
| d | Create new directory |
| x | Delete file/directory |
| r | Rename file/directory |
| i | Show file information |
| h | Show help |
| q | Quit file manager |

### File Type Recognition

The file manager uses visual indicators for different file types:

- ⌘ **Directories** (blue) - Folders and subdirectories
- 𓆚 **Python files** (green) - `.py` files for scripts
- ⍺ **Text files** (gray) - `.txt`, `.md`, general text
- ⚙ **Config files** (yellow) - `.conf`, `.cfg`, `.json`, `.ini`
- ☊ **Node files** (cyan) - Circuit configuration files
- ⎃ **Color files** (rainbow) - Display and LED configurations

### Creating Python Scripts

1. Open file manager with `/`
2. Navigate to `/python_scripts/` directory
3. Press `n` to create new file
4. Name it with `.py` extension (e.g., `my_script.py`)
5. Press `e` to edit with eKilo editor
6. Write your Python code
7. Save and exit

### Editing Files

The file manager integrates with the eKilo editor for text editing:

- **Syntax highlighting** for Python code
- **Auto-indentation** for code blocks
- **Line numbers** for easy navigation
- **Search and replace** functionality

---

## USB Mass Storage

Access files directly from your computer.

### Enabling USB Mass Storage

```bash
U          # Enable USB mass storage mode
```

Or in Python:
```python
# USB mass storage is controlled by system commands
# Access via terminal command 'U'
```

### File Access from Computer

When USB mass storage is enabled:

1. Jumperless appears as a removable drive
2. Access all files and directories
3. Copy files to/from the device
4. Edit files with your preferred editor
5. Transfer Python scripts and data files

### Disabling USB Mass Storage

```bash
u          # Disable USB mass storage mode
```

### Best Practices

1. **Always disable USB mode** before resuming normal operation
2. **Safely eject** the drive from your computer
3. **Keep backups** of important scripts and configurations
4. **Use meaningful filenames** for easy identification

---

## Directory Structure

Understanding the Jumperless file system structure:

```
/                           # Root directory
├── python_scripts/         # Python scripts and modules
│   ├── examples/          # Example scripts
│   ├── user_scripts/      # User-created scripts
│   └── *.py              # Python files
├── config/                # Configuration files
│   ├── settings.json     # System settings
│   ├── calibration.conf  # Hardware calibration
│   └── *.conf           # Various config files
├── data/                  # Data storage
│   ├── logs/             # Log files
│   ├── measurements/     # Sensor data
│   └── *.dat            # Data files
├── slots/                 # Circuit configurations
│   ├── slot1.txt         # Saved circuit 1
│   ├── slot2.txt         # Saved circuit 2
│   └── *.txt            # Circuit definition files
└── tmp/                   # Temporary files
    └── *.tmp             # Temporary data
```

---

## File Operations Examples

### Data Logging System

```python
import jfs
import time

class DataLogger:
    def __init__(self, filename=None):
        if filename is None:
            # Create timestamped filename
            timestamp = int(time.time())
            filename = f"data_{timestamp}.log"
        
        self.filename = f"/data/logs/{filename}"
        self.ensure_directory("/data/logs")
    
    def ensure_directory(self, path):
        """Ensure directory exists"""
        if not jfs.exists(path):
            # Create parent directories
            parts = path.strip('/').split('/')
            current = ""
            for part in parts:
                current += "/" + part
                if not jfs.exists(current):
                    jfs.mkdir(current)
    
    def log_data(self, sensor_id, value, units="V"):
        """Log sensor data"""
        timestamp = int(time.time())
        log_entry = f"{timestamp},{sensor_id},{value},{units}\n"
        
        file = jfs.open(self.filename, "a")
        jfs.write(file, log_entry)
        jfs.close(file)
    
    def read_data(self, limit=None):
        """Read logged data"""
        if not jfs.exists(self.filename):
            return []
        
        content = jfs.read(self.filename)
        lines = content.strip().split('\n')
        
        data = []
        for line in lines:
            if line.strip():
                parts = line.split(',')
                if len(parts) >= 4:
                    data.append({
                        'timestamp': int(parts[0]),
                        'sensor_id': parts[1],
                        'value': float(parts[2]),
                        'units': parts[3]
                    })
        
        if limit:
            return data[-limit:]
        return data

# Example usage
logger = DataLogger("voltage_test.log")

# Log some data
import jumperless
for i in range(10):
    voltage = jumperless.adc_get(0)
    logger.log_data("ADC0", voltage, "V")
    time.sleep(1)

# Read back the data
data = logger.read_data(5)  # Last 5 entries
for entry in data:
    print(f"Time: {entry['timestamp']}, "
          f"Sensor: {entry['sensor_id']}, "
          f"Value: {entry['value']}{entry['units']}")
```

### Script Auto-Runner

```python
def create_startup_script():
    """Create script that runs on startup"""
    startup_code = '''
# Jumperless Startup Script
import jumperless
import time

print("Jumperless startup script running...")

# Initialize system
jumperless.oled_print("System Ready")

# Set default voltages
jumperless.dac_set(2, 3.3)  # TOP_RAIL to 3.3V
jumperless.dac_set(3, 0.0)  # BOTTOM_RAIL to GND

# Setup common connections
jumperless.connect("TOP_RAIL", "1")
jumperless.connect("BOTTOM_RAIL", "30")
jumperless.connect("GND", "31")

print("Startup complete!")
'''
    
    jfs.write("/python_scripts/startup.py", startup_code)
    print("Startup script created")

def run_startup_script():
    """Run startup script if it exists"""
    if jfs.exists("/python_scripts/startup.py"):
        try:
            exec(open("/python_scripts/startup.py").read())
        except Exception as e:
            print(f"Startup script error: {e}")

# Create and run startup script
create_startup_script()
run_startup_script()
```

---

## Error Handling and Best Practices

### File Operation Safety

```python
def safe_file_write(filename, content):
    """Safely write file with error handling"""
    try:
        # Write to temporary file first
        temp_filename = filename + ".tmp"
        
        file = jfs.open(temp_filename, "w")
        jfs.write(file, content)
        jfs.close(file)
        
        # If successful, rename to final name
        if jfs.exists(filename):
            jfs.remove(filename)  # Remove old file
        
        success = jfs.rename(temp_filename, filename)
        
        if success:
            print(f"✓ Successfully wrote {filename}")
            return True
        else:
            print(f"✗ Failed to finalize {filename}")
            return False
            
    except Exception as e:
        print(f"✗ Error writing {filename}: {e}")
        # Clean up temp file
        if jfs.exists(temp_filename):
            jfs.remove(temp_filename)
        return False

def safe_file_read(filename, default=""):
    """Safely read file with error handling"""
    try:
        if jfs.exists(filename):
            content = jfs.read(filename)
            print(f"✓ Successfully read {filename}")
            return content
        else:
            print(f"! File not found: {filename}, using default")
            return default
    except Exception as e:
        print(f"✗ Error reading {filename}: {e}")
        return default

# Example usage
config_data = "debug=true\nled_brightness=75\n"
safe_file_write("config.txt", config_data)

loaded_config = safe_file_read("config.txt", "debug=false\n")
print("Config:", loaded_config)
```

### Storage Management

```python
def check_storage_space():
    """Check available storage space"""
    total, used, free = jfs.info()
    
    usage_percent = (used / total) * 100
    
    print(f"Storage Status:")
    print(f"  Total: {total:,} bytes ({total/1024:.1f} KB)")
    print(f"  Used:  {used:,} bytes ({used/1024:.1f} KB)")
    print(f"  Free:  {free:,} bytes ({free/1024:.1f} KB)")
    print(f"  Usage: {usage_percent:.1f}%")
    
    if usage_percent > 90:
        print("⚠ Warning: Storage nearly full!")
    elif usage_percent > 75:
        print("! Notice: Storage over 75% full")
    else:
        print("✓ Storage space OK")
    
    return free

def cleanup_temp_files():
    """Clean up temporary files"""
    temp_dir = "/tmp"
    if jfs.exists(temp_dir):
        files = jfs.listdir(temp_dir)
        if files:
            file_list = files.split(',')
            for filename in file_list:
                if filename.endswith('.tmp'):
                    filepath = f"{temp_dir}/{filename}"
                    jfs.remove(filepath)
                    print(f"Removed temp file: {filename}")

# Regular maintenance
check_storage_space()
cleanup_temp_files()
```

### Best Practices Summary

1. **Always check file existence** before reading
2. **Use error handling** for all file operations
3. **Close files properly** after operations
4. **Use meaningful filenames** and directory structure
5. **Regular cleanup** of temporary and log files
6. **Monitor storage space** to prevent full disk issues
7. **Use temporary files** for safe writing operations
8. **Keep backups** of important configurations
9. **Test file operations** with small amounts of data first
10. **Use appropriate file extensions** for proper recognition

This comprehensive guide covers all aspects of file system operations on Jumperless, from basic functions to advanced JFS operations and best practices for reliable file management. 