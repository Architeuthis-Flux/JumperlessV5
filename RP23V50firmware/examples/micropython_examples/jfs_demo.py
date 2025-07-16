"""
JFS Module Demo - Jumperless FileSystem Operations
Demonstrates comprehensive filesystem functionality with syntax highlighting

This script shows both basic and advanced filesystem operations using the JFS module.
Notice the cyan-blue syntax highlighting for all filesystem functions in the editor!
"""

import jfs
import time

def main():
    print("🚀 JFS Module Demonstration")
    print("=" * 40)
    
    # File operations demonstration
    demo_basic_operations()
    demo_advanced_operations()
    demo_directory_operations()
    demo_configuration_management()
    demo_data_logging()
    
    print("\n✅ JFS Demo completed successfully!")

def demo_basic_operations():
    print("\n📄 Basic File Operations")
    print("-" * 25)
    
    # Basic filesystem functions (highlighted in cyan-blue)
    test_file = "jfs_test.txt"
    
    # Write and read using basic functions
    fs_write(test_file, "Hello from basic JFS functions!")
    
    if fs_exists(test_file):
        content = fs_read(test_file)
        print(f"File content: {content}")
    
    # JFS module functions (also highlighted in cyan-blue)
    jfs.write("jfs_demo.txt", "Hello from JFS module!")
    
    if jfs.exists("jfs_demo.txt"):
        content = jfs.read("jfs_demo.txt")
        print(f"JFS content: {content}")

def demo_advanced_operations():
    print("\n🔧 Advanced File Operations")
    print("-" * 28)
    
    # File handle operations with context manager
    log_file = "advanced_demo.log"
    
    # Notice how all jfs functions are highlighted consistently
    with jfs.open(log_file, "w") as file:
        file.write("=== Advanced JFS Demo Log ===\n")
        file.write(f"Started at: {time.time()}\n")
        
        # File information functions
        pos = file.tell()
        print(f"Current position: {pos}")
    
    # Read file information
    with jfs.open(log_file, "r") as file:
        size = file.size()
        available = file.available()
        print(f"File size: {size} bytes, available: {available}")
        
        # Read in chunks
        file.seek(0)  # Go to beginning
        while file.available() > 0:
            chunk = file.read(20)  # Read 20 bytes
            if chunk:
                print(f"Chunk: {repr(chunk)}")

def demo_directory_operations():
    print("\n📁 Directory Operations")
    print("-" * 23)
    
    # Directory management (all highlighted in cyan-blue)
    demo_dir = "jfs_demo_dir"
    
    if not jfs.exists(demo_dir):
        jfs.mkdir(demo_dir)
        print(f"Created directory: {demo_dir}")
    
    # Create some files in the directory
    for i in range(3):
        filename = f"{demo_dir}/file_{i}.txt"
        jfs.write(filename, f"This is file number {i}")
    
    # List directory contents
    files = jfs.listdir(demo_dir)
    print(f"Directory contents: {files}")
    
    # File statistics
    test_file = f"{demo_dir}/file_0.txt"
    if jfs.exists(test_file):
        stat_info = jfs.stat(test_file)
        file_size = stat_info[6]  # Size is at index 6
        print(f"File size: {file_size} bytes")

def demo_configuration_management():
    print("\n⚙️ Configuration Management")
    print("-" * 29)
    
    config_file = "jfs_config.json"
    
    # Create configuration
    config_data = {
        "sensor_interval": 1000,
        "debug_mode": True,
        "device_name": "Jumperless_Dev"
    }
    
    # Note: In real usage, you'd use json module
    config_text = str(config_data).replace("'", '"').replace("True", "true")
    
    # Write configuration (highlighted function)
    jfs.write(config_file, config_text)
    print("Configuration saved")
    
    # Read and verify
    if jfs.exists(config_file):
        saved_config = jfs.read(config_file)
        print(f"Saved config: {saved_config}")

def demo_data_logging():
    print("\n📊 Data Logging Example")
    print("-" * 25)
    
    log_file = "sensor_data.csv"
    
    # Create CSV header if file doesn't exist
    if not jfs.exists(log_file):
        jfs.write(log_file, "timestamp,temperature,humidity\n")
    
    # Append sensor readings
    with jfs.open(log_file, "a") as file:
        for i in range(3):
            timestamp = int(time.time()) + i
            temp = 20.0 + i * 0.5
            humidity = 45.0 + i * 2.0
            
            file.write(f"{timestamp},{temp},{humidity}\n")
    
    # Read and display the log
    print("Sensor log contents:")
    content = jfs.read(log_file)
    print(content)

def demo_filesystem_info():
    print("\n💾 Filesystem Information")
    print("-" * 27)
    
    # Get filesystem statistics
    total, used, free = jfs.info()
    print(f"Storage: {used}KB used of {total}KB total")
    print(f"Free space: {free}KB")
    
    # List all files in root
    print("\nRoot directory contents:")
    root_files = jfs.listdir("/")
    for file in root_files:
        if jfs.exists(file):
            stat_info = jfs.stat(file)
            size = stat_info[6]
            print(f"  {file}: {size} bytes")

# Cleanup function
def cleanup_demo_files():
    """Clean up files created during demo"""
    demo_files = [
        "jfs_test.txt",
        "jfs_demo.txt", 
        "advanced_demo.log",
        "jfs_config.json",
        "sensor_data.csv"
    ]
    
    for file in demo_files:
        if jfs.exists(file):
            jfs.remove(file)
    
    # Remove demo directory and its contents
    demo_dir = "jfs_demo_dir"
    if jfs.exists(demo_dir):
        # Remove files first
        files = jfs.listdir(demo_dir)
        for file in files:
            jfs.remove(f"{demo_dir}/{file}")
        
        # Remove directory
        jfs.rmdir(demo_dir)

# Run the demo
if __name__ == "__main__":
    main()
    
    # Uncomment to clean up demo files
    # cleanup_demo_files()
    # print("Demo files cleaned up") 