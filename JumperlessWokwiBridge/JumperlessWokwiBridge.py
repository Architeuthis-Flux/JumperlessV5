# SPDX-License-Identifier: MIT
# Kevin Santo Cappuccio
# Jumperless Bridge App
# KevinC@ppucc.io
#
# Enhanced Wokwi Integration:
# - Improved sketch.ino extraction from Wokwi projects using multiple extraction methods
# - Better error handling and fallback mechanisms for various Wokwi page formats
# - Support for direct code extraction from visible page elements

import pathlib
import requests
import json
import serial
import time
import sys
import codecs
import os
import shutil
from urllib.request import urlretrieve
import ssl
import psutil
import threading
import platform
from bs4 import BeautifulSoup
import re
import subprocess

# Command history support
try:
    import readline
    READLINE_AVAILABLE = True
    # Configure readline for better command history
    readline.set_history_length(100)  # Keep last 100 commands
    # Enable tab completion if available
    try:
        readline.parse_and_bind("tab: complete")
    except:
        pass
except ImportError:
    READLINE_AVAILABLE = False

# import colored

# from termcolor import colored, cprint


# Arduino CLI support with automatic installation
try:
    import pyduinocli
    PYDUINOCLI_AVAILABLE = True
except ImportError:
    PYDUINOCLI_AVAILABLE = False
    # safe_print will be called later after it's defined

# Cross-platform color handling
try:
    import colorama
    from colorama import Fore, Style
    colorama.init(autoreset=True)
    COLORS_AVAILABLE = True
except ImportError:
    # Fallback if colorama is not available
    class Fore:
        RED = '\033[31m' if platform.system() != 'Windows' else ''
        GREEN = '\033[32m' if platform.system() != 'Windows' else ''
        YELLOW = '\033[33m' if platform.system() != 'Windows' else ''
        BLUE = '\033[34m' if platform.system() != 'Windows' else ''
        MAGENTA = '\033[35m' if platform.system() != 'Windows' else ''
        CYAN = '\033[36m' if platform.system() != 'Windows' else ''
        WHITE = '\033[37m' if platform.system() != 'Windows' else ''
        RESET = '\033[0m' if platform.system() != 'Windows' else ''
    
    class Style:
        RESET_ALL = '\033[0m' if platform.system() != 'Windows' else ''
    
    COLORS_AVAILABLE = False

# Platform-specific imports
if sys.platform == "win32":
    try:
        import win32api
        WIN32_AVAILABLE = True
    except ImportError:
        WIN32_AVAILABLE = False
else:
    WIN32_AVAILABLE = False

# SSL context setup
try:
    ssl._create_default_https_context = ssl._create_unverified_context
except AttributeError:
    pass  # Older Python versions might not have this

# Enable ANSI colors on Windows
if sys.platform == "win32":
    os.system("")

import serial.tools.list_ports

# ============================================================================
# ARDUINO CLI SETUP AND AUTO-INSTALLATION
# ============================================================================
arduino_cli_version = "1.2.2"
def get_latest_arduino_cli_version():
    """Get the latest Arduino CLI version from GitHub releases API"""
    global arduino_cli_version
    try:
        response = requests.get(
            "https://api.github.com/repos/arduino/arduino-cli/releases/latest",
            timeout=10
        )
        if response.status_code == 200:
            release_data = response.json()
            version = release_data.get('tag_name', '').lstrip('v')  # Remove 'v' prefix if present
            if version:
                arduino_cli_version = version
                return version
    except Exception:
        # Don't print here since safe_print might not be defined yet
        pass
    
    # Fallback to known working version
    return "1.2.2"

def get_arduino_cli_url():
    """Get the appropriate Arduino CLI download URL for the current platform"""
    system = platform.system().lower()
    machine = platform.machine().lower()
    
    # Get latest version from GitHub
    version = get_latest_arduino_cli_version()
    
    if system == "windows":
        if machine in ["x86_64", "amd64"]:
            return f"https://github.com/arduino/arduino-cli/releases/download/v{version}/arduino-cli_{version}_Windows_64bit.zip"
        else:
            return f"https://github.com/arduino/arduino-cli/releases/download/v{version}/arduino-cli_{version}_Windows_32bit.zip"
    elif system == "darwin":  # macOS
        if machine in ["arm64", "aarch64"]:
            return f"https://github.com/arduino/arduino-cli/releases/download/v{version}/arduino-cli_{version}_macOS_ARM64.tar.gz"
        else:
            return f"https://github.com/arduino/arduino-cli/releases/download/v{version}/arduino-cli_{version}_macOS_64bit.tar.gz"
    elif system == "linux":
        if machine in ["x86_64", "amd64"]:
            return f"https://github.com/arduino/arduino-cli/releases/download/v{version}/arduino-cli_{version}_Linux_64bit.tar.gz"
        elif machine in ["armv7l", "armv6l"]:
            return f"https://github.com/arduino/arduino-cli/releases/download/v{version}/arduino-cli_{version}_Linux_ARMv7.tar.gz"
        elif machine in ["aarch64", "arm64"]:
            return f"https://github.com/arduino/arduino-cli/releases/download/v{version}/arduino-cli_{version}_Linux_ARM64.tar.gz"
        else:
            return f"https://github.com/arduino/arduino-cli/releases/download/v{version}/arduino-cli_{version}_Linux_32bit.tar.gz"
    
    return None

def download_and_extract_arduino_cli():
    """Download and extract Arduino CLI to the current directory"""
    try:
        import zipfile
        import tarfile
        
        cli_url = get_arduino_cli_url()
        if not cli_url:
            safe_print("Unsupported platform for Arduino CLI auto-download", Fore.RED)
            return False
        
        # Get and display the version being downloaded
        version = get_latest_arduino_cli_version()
        safe_print(f"Using Arduino CLI version: {version}", Fore.CYAN)
        
        # Check if we're using fallback version
        try:
            response = requests.get(
                "https://api.github.com/repos/arduino/arduino-cli/releases/latest",
                timeout=5
            )
            if response.status_code != 200:
                safe_print("Using fallback version (GitHub API unavailable)", Fore.YELLOW)
        except Exception:
            safe_print("Using fallback version (GitHub API unavailable)", Fore.YELLOW)
        
        safe_print("Downloading Arduino CLI...", Fore.CYAN)
        
        # Determine file extension
        if cli_url.endswith('.zip'):
            filename = "arduino-cli.zip"
        else:
            filename = "arduino-cli.tar.gz"
        
        # Download the file
        try:
            urlretrieve(cli_url, filename)
            safe_print("Arduino CLI downloaded successfully", Fore.GREEN)
        except Exception as e:
            safe_print(f"Failed to download Arduino CLI: {e}", Fore.RED)
            return False
        
        # Extract the file
        try:
            if filename.endswith('.zip'):
                with zipfile.ZipFile(filename, 'r') as zip_ref:
                    # Extract arduino-cli executable
                    for file_info in zip_ref.filelist:
                        if file_info.filename.endswith('arduino-cli.exe') or file_info.filename.endswith('arduino-cli'):
                            # Extract to current directory with proper name
                            with zip_ref.open(file_info) as source:
                                exe_name = "arduino-cli.exe" if sys.platform == "win32" else "arduino-cli"
                                with open(exe_name, 'wb') as target:
                                    target.write(source.read())
                                # Make executable on Unix-like systems
                                if sys.platform != "win32":
                                    os.chmod(exe_name, 0o755)
                                break
            else:  # tar.gz
                with tarfile.open(filename, 'r:gz') as tar_ref:
                    # Extract arduino-cli executable
                    for member in tar_ref.getmembers():
                        if member.name.endswith('arduino-cli'):
                            # Extract to current directory
                            member.name = "arduino-cli"
                            tar_ref.extract(member)
                            # Make executable
                            os.chmod("arduino-cli", 0o755)
                            break
            
            # Clean up downloaded archive
            os.remove(filename)
            safe_print("Arduino CLI extracted successfully", Fore.GREEN)
            return True
            
        except Exception as e:
            safe_print(f"Failed to extract Arduino CLI: {e}", Fore.RED)
            try:
                os.remove(filename)
            except:
                pass
            return False
            
    except Exception as e:
        safe_print(f"Error during Arduino CLI installation: {e}", Fore.RED)
        return False

def setup_arduino_cli():
    """Setup Arduino CLI with automatic installation if needed"""
    global arduino, noArduinocli, disableArduinoFlashing
    
    if not PYDUINOCLI_AVAILABLE:
        safe_print("pyduinocli module not available. Install with: pip install pyduinocli", Fore.YELLOW)
        noArduinocli = True
        disableArduinoFlashing = 1
        return False
    
    # Try different Arduino CLI locations in order of preference
    cli_paths = [
        resource_path("arduino-cli.exe" if sys.platform == "win32" else "arduino-cli"),
        "./arduino-cli.exe" if sys.platform == "win32" else "./arduino-cli",
        "arduino-cli"  # System PATH
    ]
    
    arduino = None
    for cli_path in cli_paths:
        try:
            arduino = pyduinocli.Arduino(cli_path)
            # Test if Arduino CLI is working
            arduino.version()
            safe_print(f"Arduino CLI found at: {cli_path}", Fore.GREEN)
            noArduinocli = False
            disableArduinoFlashing = 0
            return True
        except Exception:
            continue
    
    # If no Arduino CLI found, try to download and install it
    safe_print("Arduino CLI not found. Attempting automatic installation...", Fore.YELLOW)
    
    if download_and_extract_arduino_cli():
        # Try to initialize with the downloaded CLI
        cli_name = "arduino-cli.exe" if sys.platform == "win32" else "./arduino-cli"
        try:
            arduino = pyduinocli.Arduino(cli_name)
            arduino.version()
            safe_print("Arduino CLI automatically installed and ready!", Fore.GREEN)
            noArduinocli = False
            disableArduinoFlashing = 0
            return True
        except Exception as e:
            safe_print(f"Failed to initialize downloaded Arduino CLI: {e}", Fore.RED)
    
    # If all attempts fail
    safe_print("Could not setup Arduino CLI. Arduino flashing will be disabled.", Fore.YELLOW)
    safe_print("You can manually install Arduino CLI or install pyduinocli: pip install pyduinocli", Fore.CYAN)
    noArduinocli = True
    disableArduinoFlashing = 1
    arduino = None
    return False

def get_installed_arduino_cli_version():
    """Get the version of the currently installed Arduino CLI"""
    global arduino, arduino_cli_version
    
    if noArduinocli or not arduino:
        return "Not installed"
    
    if arduino_cli_version != "Unknown":
        return arduino_cli_version
    
    
    try:
        # Try to get version from arduino-cli
        version_output = arduino.version()
        if version_output and 'result' in version_output:
            arduino_cli_version = version_output['result'].get('Version', 'Unknown')
            return arduino_cli_version
    except Exception:
        pass
    
    return "Unknown"

# Initialize Arduino CLI
arduino = None
noArduinocli = True

# ============================================================================
# GLOBAL VARIABLES AND LOCKS
# ============================================================================

# Threading locks for thread-safe operations
serial_lock = threading.Lock()
wokwi_update_lock = threading.Lock()

# Debug and configuration flags
debug = False
jumperlessV5 = False
noWokwiStuff = False
disableArduinoFlashing = 1
noArduinocli = True
debugWokwi = False  # New debug flag for Wokwi updates

# Arduino CLI configuration
if noArduinocli == True:
    disableArduinoFlashing = 1

# Serial connection state
serialconnected = 0
portSelected = 0
portNotFound = 0
justreconnected = 0
menuEntered = 0
justChecked = 0
reading = 0
forceWokwiUpdate = 0

# Port and device information
portName = ''
arduinoPort = ''
ser = None
updateInProgress = 0

# Wokwi and project management
stringified = ' '
lastDiagram = [' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ']
diagram = [' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ']
sketch = [' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ']
lastsketch = ['  ', '  ', '  ', '  ', '  ', '  ', '  ', '  ']
lastlibraries = ['  '] * 8 # Changed from '  '
blankDiagrams = [' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ']

# Slot management
slotURLs = ['!', '!', '!', '!', '!', '!', '!', '!', '!']
slotAPIurls = ['!', '!', '!', '!', '!', '!', '!', '!', '!']
numAssignedSlots = 0
currentSlotUpdate = 0
wokwiUpdateRate = 3.0

# Local file management (new)
slotFilePaths = ['!', '!', '!', '!', '!', '!', '!', '!']  # Local .ino file paths
slotFileModTimes = [0, 0, 0, 0, 0, 0, 0, 0]  # File modification times for change detection
slotFileHashes = ['', '', '', '', '', '', '', '']  # Content hashes for change detection

# Firmware information
jumperlessFirmwareNumber = [0, 0, 0, 0, 0, 0]
jumperlessFirmwareString = ' '
currentString = 'unknown'

# File paths
slotAssignmentsFile = "JumperlessFiles/slotAssignments.txt"
savedProjectsFile = "JumperlessFiles/savedProjects.txt"

# Firmware URLs
latestFirmwareAddress = "https://github.com/Architeuthis-Flux/Jumperless/releases/latest/download/firmware.uf2"
latestFirmwareAddressV5 = "https://github.com/Architeuthis-Flux/JumperlessV5/releases/latest/download/firmware.uf2"

# Arduino sketch defaults
defaultWokwiSketchText = 'void setup() {'

# ============================================================================
# UTILITY FUNCTIONS
# ============================================================================

def resource_path(relative_path):
    """Get absolute path to resource, works for dev and for PyInstaller"""
    base_path = getattr(sys, '_MEIPASS', os.path.dirname(os.path.abspath(__file__)))
    return os.path.join(base_path, relative_path)

def safe_print(message, color=None, end='\n'):
    """Cross-platform safe printing with optional color"""
    if color and COLORS_AVAILABLE:
        print(f"{color}{message}{Style.RESET_ALL}", end=end)
    elif color and not COLORS_AVAILABLE and color != Fore.RESET:
        print(f"{color}{message}{Fore.RESET}", end=end)
    else:
        print(message, end=end)

def create_directories():
    """Create necessary directories"""
    try:
        pathlib.Path(slotAssignmentsFile).parent.mkdir(parents=True, exist_ok=True)
        pathlib.Path(savedProjectsFile).parent.mkdir(parents=True, exist_ok=True)
    except Exception as e:
        safe_print(f"Warning: Could not create directories: {e}", Fore.YELLOW)

def input_with_timeout(prompt, timeout=4, default="y"):
    """Get user input with timeout, returns default if timeout expires"""
    import select
    import sys
    
    def timeout_input():
        """Cross-platform input with timeout"""
        if sys.platform == "win32":
            # Windows implementation using threading
            import msvcrt
            import time
            
            print(prompt, end='', flush=True)
            start_time = time.time()
            input_chars = []
            
            while True:
                if msvcrt.kbhit():
                    char = msvcrt.getch().decode('utf-8')
                    if char == '\r':  # Enter key
                        print()  # New line
                        return ''.join(input_chars).strip()
                    elif char == '\b':  # Backspace
                        if input_chars:
                            input_chars.pop()
                            print('\b \b', end='', flush=True)
                    else:
                        input_chars.append(char)
                        print(char, end='', flush=True)
                
                if time.time() - start_time > timeout:
                    # print(f"\n(Timeout - defaulting to '{default}')")
                    return default
                
                time.sleep(0.1)
        else:
            # Unix-like systems (macOS, Linux)
            print(prompt, end='', flush=True)
            
            ready, _, _ = select.select([sys.stdin], [], [], timeout)
            if ready:
                return sys.stdin.readline().strip()
            else:
                # print(f"\n(Timeout - defaulting to '{default}')")
                return default
    
    try:
        return timeout_input()
    except Exception:
        # Fallback to regular input if timeout method fails
        print(prompt, end='', flush=True)
        try:
            return input().strip()
        except:
            return default

def print_format_table():
    """
    prints table of formatted text format options
    """
    for style in range(8):
        for fg in range(30,38):
            s1 = ''
            for bg in range(40,48):
                format = ';'.join([str(style), str(fg), str(bg)])
                s1 += '\x1b[%sm %s \x1b[0m' % (format, format)
            print(s1)
        print('\n')

# ============================================================================
# LOCAL FILE MONITORING FUNCTIONS
# ============================================================================

def is_valid_ino_file(file_path):
    """Check if the file path is a valid .ino file"""
    if not file_path or file_path == '!':
        return False
    return file_path.lower().endswith('.ino') and os.path.isfile(file_path)

def get_file_content_hash(file_path):
    """Get hash of file content for change detection"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        return str(hash(content))
    except Exception:
        return ''

def check_local_file_change(slot_number):
    """Check if local file has changed for given slot"""
    global slotFilePaths, slotFileModTimes, slotFileHashes
    
    if slot_number < 0 or slot_number >= len(slotFilePaths):
        return False
    
    file_path = slotFilePaths[slot_number]
    if not is_valid_ino_file(file_path):
        return False
    
    try:
        # Get current modification time and content hash
        current_mod_time = os.path.getmtime(file_path)
        current_hash = get_file_content_hash(file_path)
        
        # Check if file has changed
        if (current_mod_time != slotFileModTimes[slot_number] or 
            current_hash != slotFileHashes[slot_number]):
            
            # Update stored values
            slotFileModTimes[slot_number] = current_mod_time
            slotFileHashes[slot_number] = current_hash
            return True
            
    except Exception as e:
        if debugWokwi:
            safe_print(f"Error checking file change for slot {slot_number}: {e}", Fore.YELLOW)
    
    return False

def read_local_ino_file(file_path):
    """Read and return content of local .ino file"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read().strip()
        return content
    except Exception as e:
        safe_print(f"Error reading file {file_path}: {e}", Fore.RED)
        return None

def process_local_file_and_flash(slot_number):
    """Process local .ino file and flash if changed"""
    global slotFilePaths, lastsketch, sketch
    
    if slot_number < 0 or slot_number >= len(slotFilePaths):
        return False
    
    file_path = slotFilePaths[slot_number]
    if not is_valid_ino_file(file_path):
        return False
    
    try:
        if check_local_file_change(slot_number):
            content = read_local_ino_file(file_path)
            if content and len(content) > 10:
                # Store in sketch array
                sketch[slot_number] = content
                
                # Check if content has actually changed
                if lastsketch[slot_number] != content:
                    lastsketch[slot_number] = content
                    
                    safe_print(f"\nLocal file changed for slot {slot_number}: {os.path.basename(file_path)}", Fore.MAGENTA)
                    if debugWokwi:
                        safe_print(f"File path: {file_path}", Fore.CYAN)
                        safe_print(content[:200] + ('...' if len(content) > 200 else ''), Fore.CYAN)
                    
                    # Flash the Arduino
                    try:
                        flash_thread = flash_arduino_sketch_threaded(content, "", slot_number)
                        return flash_thread is not None
                    except Exception as e:
                        safe_print(f"Error flashing local file for slot {slot_number}: {e}", Fore.RED)
                        return False
                else:
                    if debugWokwi:
                        safe_print(f"Local file for slot {slot_number} changed but content same", Fore.BLUE)
            else:
                safe_print(f"Local file for slot {slot_number} is empty or too short", Fore.YELLOW)
                
    except Exception as e:
        safe_print(f"Error processing local file for slot {slot_number}: {e}", Fore.RED)
        return False
    
    return False

# ============================================================================
# SERIAL COMMUNICATION FUNCTIONS
# ============================================================================

def get_available_ports():
    """Get list of available serial ports with cross-platform handling"""
    try:
        ports = serial.tools.list_ports.comports()
        return [(port.device, port.description, port.hwid) for port in ports]
    except Exception as e:
        safe_print(f"Error getting serial ports: {e}", Fore.RED)
        return []

def parse_hardware_id(hwid):
    """Parse hardware ID to extract VID and PID"""
    try:
        if "VID:PID=" in hwid:
            split_at = "VID:PID="
            split_ind = hwid.find(split_at)
            hwid_string = hwid[split_ind + 8:split_ind + 17]
            vid = hwid_string[0:4]
            pid = hwid_string[5:9]
            return vid, pid
        elif "VID_" in hwid and "PID_" in hwid:
            # Windows format: USB VID_2E8A&PID_000A
            vid_start = hwid.find("VID_") + 4
            pid_start = hwid.find("PID_") + 4
            vid = hwid[vid_start:vid_start + 4]
            pid = hwid[pid_start:pid_start + 4]
            return vid, pid
    except Exception:
        pass
    return None, None

def is_jumperless_device(desc, pid):
    """Check if device is a Jumperless device"""
    return (desc == "Jumperless" or 
            pid in ["ACAB", "1312"] or 
            "jumperless" in desc.lower())

def choose_jumperless_port(sorted_ports):
    """Choose the correct Jumperless port by testing firmware response"""
    global jumperlessFirmwareString, jumperlessFirmwareNumber, jumperlessV5
    
    jumperlessFirmwareString = ' '
    
    for try_port_idx, port_name in enumerate(sorted_ports):
        try:
            with serial.Serial(port_name, 115200, timeout=1) as temp_ser:
                temp_ser.write(b'?')
                time.sleep(0.1)
                
                if temp_ser.in_waiting > 0:
                    input_buffer = b''
                    start_time = time.time()
                    
                    # Read with timeout
                    while time.time() - start_time < 0.5:
                        if temp_ser.in_waiting > 0:
                            input_buffer += temp_ser.read(temp_ser.in_waiting)
                        else:
                            time.sleep(0.01)
                        
                        if b'\n' in input_buffer or time.time() - start_time > 0.5:
                            break
                    
                    try:
                        input_str = input_buffer.decode('utf-8', errors='ignore').strip()
                        lines = input_str.split('\n')
                        
                        for line in lines:
                            if line.startswith("Jumperless firmware version:"):
                                jumperlessFirmwareString = line
                                
                                # Parse version number
                                version_part = line[29:].strip()
                                version_numbers = version_part.split('.')
                                
                                if len(version_numbers) >= 3:
                                    jumperlessFirmwareNumber = version_numbers[:3]
                                    
                                    if int(version_numbers[0]) >= 5:
                                        jumperlessV5 = True
                                
                                return try_port_idx
                    except Exception:
                        continue
        except Exception:
            continue
    
    return 0

def open_serial():
    """Open serial connection with cross-platform port detection"""
    global portName, ser, arduinoPort, serialconnected, portSelected, updateInProgress
    
    portSelected = False
    found_ports = []
    
    safe_print("\nScanning for serial ports...\n", Fore.CYAN)
    
    while not portSelected and updateInProgress == 0:
        ports = get_available_ports()
        found_ports = []
        
        if not ports:
            safe_print("No serial ports found. Please connect your Jumperless.", Fore.RED)
            time.sleep(2)
            continue
        
        # safe_print("\nAvailable ports:")
        for i, (port, desc, hwid) in enumerate(ports, 1):
            vid, pid = parse_hardware_id(hwid)
            
            
            if is_jumperless_device(desc, pid):
                found_ports.append(port)
                safe_print(f"{i}: {port} [{desc}]", Fore.MAGENTA)
            else:
                safe_print(f"{i}: {port} [{desc}]", Fore.BLUE)



        # safe_print(f"\nFound {len(found_ports)} Jumperless devices", Fore.CYAN)

        if found_ports:
            sorted_ports = sorted(found_ports, key=lambda x: x[-1] if x[-1].isdigit() else x)
            jumperless_index = choose_jumperless_port(sorted_ports)
            
            if len(sorted_ports) > jumperless_index:
                portName = sorted_ports[jumperless_index]
                
                # Try to find Arduino port (usually the next port)
                if len(sorted_ports) > jumperless_index + 1:
                    arduinoPort = sorted_ports[jumperless_index + 1]
                else:
                    arduinoPort = None
                
                portSelected = True
                safe_print(f"\nAutodetected Jumperless at {portName}", Fore.CYAN)
                if arduinoPort:
                    safe_print(f"Autodetected USB-Serial at {arduinoPort}", Fore.CYAN)
            else:
                safe_print("Could not identify Jumperless port automatically.", Fore.YELLOW)
                # Manual selection fallback
                try:
                    selection = input("\nSelect port number ('r' to rescan): ").strip()
                    if selection.lower() == 'r':
                        continue
                    
                    port_idx = int(selection) - 1
                    if 0 <= port_idx < len(ports):
                        portName = ports[port_idx][0]
                        portSelected = True
                        safe_print(f"Selected port: {portName}", Fore.GREEN)
                except (ValueError, IndexError):
                    safe_print("Invalid selection. Please try again.", Fore.RED)
                    continue
        else:
            safe_print("No Jumperless devices found.", Fore.YELLOW)
            selection = input("Enter port number manually or 'r' to rescan: ").strip()
            if selection.lower() == 'r':
                continue
            
            try:
                port_idx = int(selection) - 1
                if 0 <= port_idx < len(ports):
                    portName = ports[port_idx][0]
                    portSelected = True
            except (ValueError, IndexError):
                safe_print("Invalid selection.", Fore.RED)
                continue
    
    # Attempt to open the selected port
    try:
        if updateInProgress == 0:
            with serial_lock:
                ser = serial.Serial(portName, 115200, timeout=1)
                serialconnected = 1
            safe_print(f"\nConnected to Jumperless at {portName}", Fore.GREEN)
    except Exception as e:
        safe_print(f"Failed to open serial port {portName}: {e}", Fore.RED)
        with serial_lock:
            ser = None
            serialconnected = 0

# ============================================================================
# FIRMWARE MANAGEMENT
# ============================================================================
latestFirmware = "5.1.2.6"
def check_if_fw_is_old():
    """Check if firmware needs updating"""
    global currentString, jumperlessFirmwareString, jumperlessV5, noWokwiStuff, latestFirmware
    
    if len(jumperlessFirmwareString) < 2:
        safe_print('\nCould not read FW version from the Jumperless', Fore.YELLOW)
        safe_print('Make sure you don\'t have this app running in another window.', Fore.YELLOW)
        return False
    
    try:
        split_index = jumperlessFirmwareString.rfind(':')
        currentString = jumperlessFirmwareString[split_index + 2:].strip()
        
        current_list = currentString.split('.')
        if len(current_list) < 3:
            return False
        
        # Pad version numbers
        for i in range(len(current_list)):
            if len(current_list[i]) < 2:
                current_list[i] = '0' + current_list[i]
        
        if int(current_list[0]) >= 5:
            jumperlessV5 = True
        
        # Check latest version online
        repo_url = ("https://github.com/Architeuthis-Flux/JumperlessV5/releases/latest" 
                   if jumperlessV5 else 
                   "https://github.com/Architeuthis-Flux/Jumperless/releases/latest")
        
        response = requests.get(repo_url, timeout=10)
        version = response.url.split("/").pop()
        
        latest_list = version.split('.')
        if len(latest_list) < 3:
            return False
        
        # Pad latest version numbers
        for i in range(len(latest_list)):
            if len(latest_list[i]) < 2:
                latest_list[i] =  '0' + latest_list[i]
        
        latest_int = int("".join(latest_list))
        current_int = int("".join(current_list))
        # safe_print(f"\nLatest firmware: {version}", Fore.CYAN)
        # safe_print(f"Current version: {currentString}", Fore.CYAN)
        latestFirmware = version
        if latest_int > current_int:
            safe_print(f"\nLatest firmware: {version}", Fore.MAGENTA)
            safe_print(f"Current version: {currentString}", Fore.RED)

            update_jumperless_firmware(force=False)
            return True
        
        return False
        
    except Exception as e:
        safe_print(f"Could not check firmware version: {e}", Fore.YELLOW)
        noWokwiStuff = True
        return False

def update_jumperless_firmware(force=False):
    """Update Jumperless firmware"""
    global ser, menuEntered, serialconnected, updateInProgress, portName, latestFirmware
    
    # if not force and not check_if_fw_is_old():
    #     return
    # if (force == False):
    safe_print("\nUpdating your Jumperless to the latest firmware: " + latestFirmware + "\n", Fore.YELLOW)
    
    # Use timeout input - defaults to "y" after 4 seconds
    user_response = 'y' #input_with_timeout("", timeout=4, default="y")
    
    if force or user_response.lower() in ['', 'y', 'yes']:
        safe_print("Downloading latest firmware...", Fore.GREEN)
        
        with serial_lock:
            serialconnected = 0
            if ser:
                try:
                    ser.close()
                except:
                    
                    print("*", ser, "*")
                    time.sleep(0.1)
                    pass
                ser = None
        
        menuEntered = 1
        updateInProgress = 1
        
        try:
            firmware_url = latestFirmwareAddressV5 if jumperlessV5 else latestFirmwareAddress
            urlretrieve(firmware_url, "firmware.uf2")
            safe_print("Firmware downloaded successfully!", Fore.CYAN)
            
            # Attempt automatic firmware update
            automatic_update_success = False
            
            try:
                # safe_print("Attempting automatic firmware update...", Fore.CYAN)
                time.sleep(0.50)
                safe_print("Putting Jumperless in BOOTSEL mode...", Fore.BLUE)
                # ser.close()
                time.sleep(0.50)
                
                try:
                    ser.close()
                except:
                    pass
                
                # Create tickle serial connection to trigger bootloader
                try:
                    serTickle = serial.Serial(portName, 1200, timeout=None)
                    time.sleep(0.55)
                    serTickle.close()
                    time.sleep(0.55)
                except:
                    safe_print("Could not connect to Jumperless", Fore.RED)
                    return


                
                safe_print("Waiting for mounted drive...", Fore.MAGENTA)
                foundVolume = "none"
                timeStart = time.time()
                
                # Debug information
                if sys.platform == "win32":
                    safe_print(f"Windows platform detected. WIN32_AVAILABLE: {WIN32_AVAILABLE}", Fore.CYAN)
                
                while foundVolume == "none":
                    if time.time() - timeStart > 20:
                        safe_print("Timeout waiting for bootloader drive to appear", Fore.RED)
                        break
                    time.sleep(0.25)
                    
                    try:
                        partitions = psutil.disk_partitions()
                        
                        for p in partitions:
                            try:
                                if sys.platform == "win32" and WIN32_AVAILABLE:
                                    try:
                                        volume_info = win32api.GetVolumeInformation(p.mountpoint)
                                        volume_name = volume_info[0]
                                        if jumperlessV5:
                                            if volume_name == "RP2350":
                                                foundVolume = p.mountpoint
                                                safe_print(f"Found Jumperless V5 at {foundVolume}", Fore.CYAN)
                                                break
                                        else:
                                            if volume_name == "RPI-RP2":
                                                foundVolume = p.mountpoint
                                                safe_print(f"Found Jumperless at {foundVolume}", Fore.CYAN)
                                                break
                                    except Exception:
                                        continue
                                elif sys.platform == "win32" and not WIN32_AVAILABLE:
                                    # Fallback Windows method without win32api
                                    try:
                                        # Try to read a volume info file or check for characteristic files
                                        mountpoint = p.mountpoint
                                        safe_print(f"Checking Windows drive: {mountpoint}", Fore.YELLOW)
                                        
                                        # Check if this looks like a RP2040/RP2350 drive by looking for INFO_UF2.TXT
                                        info_file = os.path.join(mountpoint, "INFO_UF2.TXT")
                                        if os.path.exists(info_file):
                                            try:
                                                with open(info_file, 'r') as f:
                                                    content = f.read()
                                                    safe_print(f"Found INFO_UF2.TXT content: {content[:100]}...", Fore.YELLOW)
                                                    if jumperlessV5:
                                                        if "RP2350" in content:
                                                            foundVolume = mountpoint
                                                            safe_print(f"Found Jumperless V5 at {foundVolume} via INFO_UF2.TXT", Fore.CYAN)
                                                            break
                                                    else:
                                                        if "RP2040" in content:
                                                            foundVolume = mountpoint
                                                            safe_print(f"Found Jumperless at {foundVolume} via INFO_UF2.TXT", Fore.CYAN)
                                                            break
                                            except Exception as e:
                                                safe_print(f"Error reading INFO_UF2.TXT: {e}", Fore.RED)
                                        
                                        # Alternative: try using subprocess to get volume label
                                        try:
                                            drive_letter = mountpoint.rstrip('\\').rstrip('/')
                                            result = subprocess.run(['vol', drive_letter], 
                                                                  capture_output=True, text=True, timeout=2)
                                            if result.returncode == 0:
                                                output = result.stdout.strip()
                                                safe_print(f"Volume info for {drive_letter}: {output}", Fore.YELLOW)
                                                if jumperlessV5:
                                                    if "RP2350" in output:
                                                        foundVolume = mountpoint
                                                        safe_print(f"Found Jumperless V5 at {foundVolume} via vol command", Fore.CYAN)
                                                        break
                                                else:
                                                    if "RPI-RP2" in output:
                                                        foundVolume = mountpoint
                                                        safe_print(f"Found Jumperless at {foundVolume} via vol command", Fore.CYAN)
                                                        break
                                        except Exception as e:
                                            safe_print(f"Error checking volume with subprocess: {e}", Fore.RED)
                                    except Exception as e:
                                        safe_print(f"Error in fallback Windows detection: {e}", Fore.RED)
                                        continue
                                else:
                                    # Unix-like systems
                                    if jumperlessV5:
                                        if p.mountpoint.endswith("RP2350") or "RP2350" in p.mountpoint:
                                            foundVolume = p.mountpoint
                                            safe_print(f"Found Jumperless V5 at {foundVolume}", Fore.RED)
                                            break
                                    else:
                                        if p.mountpoint.endswith("RPI-RP2") or "RPI-RP2" in p.mountpoint:
                                            foundVolume = p.mountpoint
                                            safe_print(f"Found Jumperless at {foundVolume}", Fore.RED)
                                            break
                            except Exception:
                                continue
                    except Exception as partition_error:
                        safe_print(f"Error scanning partitions: {partition_error}", Fore.YELLOW)
                        break
                
                if foundVolume != "none":
                    try:
                        fullPathRP = os.path.join(foundVolume, "firmware.uf2")
                        time.sleep(0.2)
                        safe_print("Copying firmware.uf2 to Jumperless...", Fore.YELLOW)
                        shutil.copy("firmware.uf2", fullPathRP)
                        
                        time.sleep(0.75)
                        safe_print("Jumperless updated to latest firmware!", Fore.GREEN)
                        automatic_update_success = True
                        
                        # Wait for device to reboot and reconnect
                        time.sleep(1.5)
                        
                        # Attempt to reconnect
                        try:
                            with serial_lock:
                                ser = serial.Serial(portName, 115200, timeout=1)
                                ser.flush()
                                serialconnected = 1
                            safe_print("Reconnected to Jumperless", Fore.CYAN)
                        except Exception as reconnect_error:
                            safe_print(f"Could not automatically reconnect: {reconnect_error}", Fore.YELLOW)
                            # safe_print("Please restart the application to reconnect", Fore.YELLOW)
                        
                    except Exception as copy_error:
                        safe_print(f"Error copying firmware: {copy_error}", Fore.RED)
                        
            except Exception as auto_update_error:
                safe_print(f"Automatic update failed: {auto_update_error}", Fore.YELLOW)
            
            # Fallback to manual instructions if automatic update failed
            if not automatic_update_success:
                # safe_print("\n" + "="*60, Fore.YELLOW)
                # safe_print("MANUAL FIRMWARE UPDATE REQUIRED", Fore.YELLOW)
                # safe_print("="*60, Fore.YELLOW)
                safe_print("\nAutomatic update failed. Please follow these steps to update manually:\n\n", Fore.RED)
                safe_print("1. Disconnect your Jumperless from USB", Fore.CYAN)
                safe_print("2. Hold the BOOTSEL button while reconnecting USB", Fore.CYAN)
                safe_print("3. Your Jumperless should appear as a USB drive", Fore.CYAN)
                if jumperlessV5:
                    safe_print("4. Look for a drive named 'RP2350'", Fore.CYAN)
                else:
                    safe_print("4. Look for a drive named 'RPI-RP2'", Fore.CYAN)
                safe_print("5. Drag the 'firmware.uf2' file to that drive", Fore.CYAN)
                safe_print("6. The Jumperless will automatically reboot with new firmware", Fore.CYAN)
                safe_print("7. Restart this application to reconnect", Fore.CYAN)
                safe_print("\nThe firmware.uf2 file is in the current directory.", Fore.GREEN)
                
                # input("\nPress Enter when you have completed the manual update...")
                # safe_print("Please restart the application to reconnect to your Jumperless.", Fore.CYAN)
            
        except Exception as e:
            safe_print(f"Failed to download firmware: {e}", Fore.RED)
        updateInProgress = 0
        menuEntered = 0

# ============================================================================
# PROJECT AND SLOT MANAGEMENT
# ============================================================================

def count_assigned_slots():
    """Count how many slots have assigned URLs or local files"""
    global slotURLs, slotFilePaths, slotFileModTimes, slotFileHashes, numAssignedSlots, noWokwiStuff
    
    numAssignedSlots = 0
    
    try:
        with open(slotAssignmentsFile, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    split_line = line.split('\t')
                    if len(split_line) >= 2:
                        try:
                            idx = int(split_line[0])
                            if 0 <= idx < len(slotURLs):
                                path_or_url = split_line[1]
                                
                                # Check if it's a local file or Wokwi URL
                                if path_or_url.lower().endswith('.ino'):
                                    # Local .ino file
                                    slotFilePaths[idx] = path_or_url
                                    slotURLs[idx] = '!'  # Clear URL slot
                                    slotAPIurls[idx] = '!'  # Clear API URL slot
                                    if is_valid_ino_file(path_or_url):
                                        numAssignedSlots += 1
                                        # Initialize file monitoring for this slot
                                        if idx < len(slotFileModTimes):
                                            try:
                                                slotFileModTimes[idx] = os.path.getmtime(path_or_url)
                                                slotFileHashes[idx] = get_file_content_hash(path_or_url)
                                            except Exception:
                                                slotFileModTimes[idx] = 0
                                                slotFileHashes[idx] = ''
                                elif path_or_url.startswith('https://wokwi.com/projects/'):
                                    # Wokwi URL
                                    slotURLs[idx] = path_or_url
                                    slotAPIurls[idx] = path_or_url.replace(
                                        "https://wokwi.com/projects/", 
                                        "https://wokwi.com/api/projects/"
                                    ) + "/diagram.json"
                                    slotFilePaths[idx] = '!'  # Clear file path slot
                                    numAssignedSlots += 1
                        except (ValueError, IndexError):
                            continue
    except FileNotFoundError:
        pass
    except Exception as e:
        safe_print(f"Error reading slot assignments: {e}", Fore.RED)
    
    return numAssignedSlots

def print_saved_projects():
    """Print list of saved projects with proper alignment"""
    try:
        with open(savedProjectsFile, 'r') as f:
            lines = f.readlines()
            
        safe_print("\nSaved Projects:", Fore.CYAN)
        
        # Filter out empty lines and parse projects
        projects = []
        for line in lines:
            line = line.strip()
            if line and '\t\t' in line:
                parts = line.split('\t\t', 1)  # Split only on first occurrence of double tab
                if len(parts) == 2:
                    projects.append((parts[0].strip(), parts[1].strip()))
        
        if not projects:
            safe_print("No saved projects found.", Fore.YELLOW)
            return
        
        # Find the maximum name length for alignment
        max_name_length = max(len(project[0]) for project in projects)
        # Ensure minimum spacing
        max_name_length = max(max_name_length, 10)
        
        # Print projects with proper alignment
        for i, (name, url) in enumerate(projects, 1):
            # Pad the name to align URLs
            padded_name = name.ljust(max_name_length)
            safe_print(f"{i}: {padded_name} {url}")
            
    except FileNotFoundError:
        safe_print("No saved projects found.", Fore.YELLOW)
    except Exception as e:
        safe_print(f"Error reading saved projects: {e}", Fore.RED)

def search_saved_projects(input_to_search, return_name=False):
    """Search saved projects by index, name, or URL"""
    try:
        with open(savedProjectsFile, 'r') as f:
            lines = f.readlines()
    except FileNotFoundError:
        # Create file if it doesn't exist
        try:
            with open(savedProjectsFile, 'w') as f:
                pass
        except Exception:
            pass
        return None
    except Exception:
        return None
    
    # Parse projects
    projects = []
    for line in lines:
        line = line.strip()
        if line and '\t\t' in line:
            parts = line.split('\t\t', 1)
            if len(parts) == 2:
                projects.append((parts[0].strip(), parts[1].strip()))
    
    if not projects:
        return None
    
    input_str = str(input_to_search).strip()
    
    # Check if input is a URL
    if input_str.startswith("https://wokwi.com/projects/"):
        if return_name:
            # Check if this URL exists in saved projects to return its name
            for name, url in projects:
                if url == input_str:
                    return name
            # If not found, prompt for a name and save it
            project_name = input("\n\nEnter a name for this new project: ").strip()
            if project_name:
                try:
                    with open(savedProjectsFile, 'a') as f:
                        f.write(f"{project_name}\t\t{input_str}\n")
                    return project_name
                except Exception:
                    pass
            return "Unnamed Project"
        else:
            return input_str
    
    # Check if input is a number (index)
    elif input_str.isdigit():
        index = int(input_str)
        if 1 <= index <= len(projects):
            name, url = projects[index - 1]
            return name if return_name else url
        else:
            safe_print(f"\nNo project found at index {index}. Valid range: 1-{len(projects)}", Fore.RED)
            return None
    
    # Check if input is a project name
    else:
        for name, url in projects:
            if name.lower() == input_str.lower():
                return name if return_name else url
        
        safe_print(f"\nNo project found with name '{input_str}'", Fore.RED)
        return None

def assign_wokwi_slots():
    """Interactive slot assignment - returns (changes_made, return_to_menu)"""
    global slotURLs, slotAPIurls, slotFilePaths, slotFileModTimes, slotFileHashes, numAssignedSlots, noWokwiStuff, menuEntered, currentString
    
    changes_made = False
    original_num_slots = numAssignedSlots
    
    count_assigned_slots()  
    if numAssignedSlots > 0:
        safe_print(f"- {numAssignedSlots} assigned", Fore.YELLOW)
    else:
        safe_print("")

    # Print slot options
    safe_print("\n      'x' to clear all slots", Fore.RED)
    safe_print("      'c' to clear a single slot", Fore.RED)
    safe_print("\nEnter the slot number you'd like to assign a project to:\n", Fore.MAGENTA)
    
    # Show current slot assignments
    safe_print("Current slot assignments:\n", Fore.BLUE)
    for i, url in enumerate(slotURLs[:8]):
        # Check if this slot has a local file or Wokwi URL
        if slotFilePaths[i] != '!':
            file_path = slotFilePaths[i]
            file_name = os.path.basename(file_path) if file_path != '!' else file_path
            file_status = "(valid)" if is_valid_ino_file(file_path) else "(missing)"
            safe_print(f"slot {i}\t[LOCAL] {file_name} {file_status}", Fore.GREEN if is_valid_ino_file(file_path) else Fore.RED)
        elif url != '!':
            safe_print(f"slot {i}\t[WOKWI] {url}", Fore.CYAN)
        else:
            safe_print(f"slot {i}\tEmpty", Fore.GREEN)
    
    try:
        slot_input = input('\n\nslot > ').strip()
        
        # Handle special commands
        if slot_input == 'menu':
            return (changes_made, True)  # Return to bridge menu
        elif slot_input == 'slots':
            return assign_wokwi_slots()  # Recursive call
        elif slot_input == 'update':
            update_jumperless_firmware(force=True)
            return (changes_made, True)  # Return to bridge menu after update
        elif slot_input == '':
            # Empty input - return to bridge menu if no changes, main loop if changes made
            if numAssignedSlots == 0 and not changes_made:
                safe_print("\n\nNo Wokwi projects assigned. Returning to menu.", Fore.YELLOW)
                return (changes_made, True)  # Return to bridge menu
            else:
                return (changes_made, not changes_made)
        elif slot_input == 'x':
            safe_print("\n\nClearing all slots\n\n")
            slotURLs = ['!'] * 8
            slotAPIurls = ['!'] * 8
            slotFilePaths = ['!'] * 8  # Also clear local file paths
            # Save cleared slots to file
            try:
                with open(slotAssignmentsFile, 'w') as f:
                    pass  # Clear the file
                numAssignedSlots = 0
                changes_made = True
                return assign_wokwi_slots()  # Show menu again after clearing
            except Exception as e:
                safe_print(f"Error clearing slots: {e}", Fore.RED)
                return (changes_made, True)
        elif slot_input.startswith('c'):
            clear_slot = input("\n\nEnter the slot number you'd like to clear:\n")
            if clear_slot.isdigit() and 0 <= int(clear_slot) <= 7:
                slot_num = int(clear_slot)
                slotURLs[slot_num] = '!'
                slotAPIurls[slot_num] = '!'
                slotFilePaths[slot_num] = '!'  # Also clear local file path
                # Save to file
                try:
                    with open(slotAssignmentsFile, 'w') as f:
                        for i in range(8):
                            if slotURLs[i] != '!' or slotFilePaths[i] != '!':
                                path_or_url = slotURLs[i] if slotURLs[i] != '!' else slotFilePaths[i]
                                f.write(f"{i}\t{path_or_url}\n")
                    numAssignedSlots = count_assigned_slots()
                    changes_made = True
                    return assign_wokwi_slots()  # Show menu again after clearing
                except Exception as e:
                    safe_print(f"Error clearing slot: {e}", Fore.RED)
                    return (changes_made, True)
            else:
                safe_print("\nInvalid slot number. Try again")
                return assign_wokwi_slots()
        elif not slot_input.isdigit():
            safe_print("\nInvalid slot")
            return assign_wokwi_slots()
        elif int(slot_input) > 7:
            safe_print("\nInvalid slot")
            return assign_wokwi_slots()
        else:
            # Valid slot number
            slot_num = int(slot_input)
            safe_print(f"\nChoose from saved projects, paste a Wokwi URL, or enter path to local .ino file for Slot {slot_input}", Fore.YELLOW)
            safe_print("Examples:", Fore.CYAN)
            safe_print("  Wokwi URL: https://wokwi.com/projects/123456789", Fore.BLUE)
            safe_print("  Local file: /path/to/sketch.ino or ./sketch.ino", Fore.BLUE)
            print_saved_projects()
            
            url_input = input("\n\nlink/path > ").strip()
            
            # Handle empty input at link prompt
            if not url_input:
                safe_print("No project selected, returning to slot menu")
                return assign_wokwi_slots()
            
            # Check if input is a local .ino file path
            if url_input.lower().endswith('.ino'):
                # Handle local .ino file
                if os.path.isfile(url_input):
                    # Valid local file
                    slotFilePaths[slot_num] = url_input
                    slotURLs[slot_num] = '!'  # Clear URL slot
                    slotAPIurls[slot_num] = '!'  # Clear API URL slot
                    
                    # Initialize file monitoring
                    try:
                        slotFileModTimes[slot_num] = os.path.getmtime(url_input)
                        slotFileHashes[slot_num] = get_file_content_hash(url_input)
                    except Exception:
                        slotFileModTimes[slot_num] = 0
                        slotFileHashes[slot_num] = ''
                    
                    # Save to file
                    try:
                        with open(slotAssignmentsFile, 'w') as f:
                            for i in range(8):
                                if slotURLs[i] != '!' or slotFilePaths[i] != '!':
                                    path_or_url = slotURLs[i] if slotURLs[i] != '!' else slotFilePaths[i]
                                    f.write(f"{i}\t{path_or_url}\n")
                        
                        numAssignedSlots = count_assigned_slots()
                        changes_made = True
                        safe_print(f"\nLocal .ino file assigned to slot {slot_num}: {os.path.basename(url_input)}", Fore.GREEN)
                        safe_print("\n\nAssign another slot or ENTER to go to Jumperless")
                        return assign_wokwi_slots()
                        
                    except Exception as e:
                        safe_print(f"Error saving slot assignment: {e}", Fore.RED)
                        return (changes_made, True)
                else:
                    safe_print(f"\nFile not found: {url_input}", Fore.RED)
                    safe_print("Please check the file path and try again.")
                    return assign_wokwi_slots()
            
            # First try to search saved projects (handles index, name, or URL)
            found_url = search_saved_projects(url_input, return_name=False)
            
            if found_url:
                # Get the project name for display
                project_name = search_saved_projects(found_url, return_name=True)
                
                slotURLs[slot_num] = found_url
                slotFilePaths[slot_num] = '!'  # Clear file path slot
                slotAPIurls[slot_num] = found_url.replace(
                    "https://wokwi.com/projects/", 
                    "https://wokwi.com/api/projects/"
                ) + "/diagram.json"
                
                # Save to file
                try:
                    with open(slotAssignmentsFile, 'w') as f:
                        for i in range(8):
                            if slotURLs[i] != '!' or slotFilePaths[i] != '!':
                                path_or_url = slotURLs[i] if slotURLs[i] != '!' else slotFilePaths[i]
                                f.write(f"{i}\t{path_or_url}\n")
                    
                    numAssignedSlots = count_assigned_slots()
                    if numAssignedSlots > 0:
                        noWokwiStuff = False
                    
                    changes_made = True
                    safe_print(f"\n{project_name} assigned to slot {slot_num}", Fore.GREEN)
                    safe_print("\n\nAssign another slot or ENTER to go to Jumperless")
                    return assign_wokwi_slots()
                    
                except Exception as e:
                    safe_print(f"Error saving slot assignment: {e}", Fore.RED)
                    return (changes_made, True)
            elif url_input.startswith('https://wokwi.com/projects/'):
                # Handle direct URL input that's not in saved projects
                slotURLs[slot_num] = url_input
                slotFilePaths[slot_num] = '!'  # Clear file path slot
                slotAPIurls[slot_num] = url_input.replace(
                    "https://wokwi.com/projects/", 
                    "https://wokwi.com/api/projects/"
                ) + "/diagram.json"
                
                # Save to file
                try:
                    with open(slotAssignmentsFile, 'w') as f:
                        for i in range(8):
                            if slotURLs[i] != '!' or slotFilePaths[i] != '!':
                                path_or_url = slotURLs[i] if slotURLs[i] != '!' else slotFilePaths[i]
                                f.write(f"{i}\t{path_or_url}\n")
                    
                    numAssignedSlots = count_assigned_slots()
                    if numAssignedSlots > 0:
                        noWokwiStuff = False
                    
                    changes_made = True
                    safe_print(f"\nWokwi project assigned to slot {slot_num}")
                    safe_print("\n\nAssign another slot or ENTER to go to Jumperless")
                    return assign_wokwi_slots()
                    
                except Exception as e:
                    safe_print(f"Error saving slot assignment: {e}", Fore.RED)
                    return (changes_made, True)
            else:
                safe_print("\nInvalid input. Enter a project index, name, Wokwi URL, or local .ino file path", Fore.RED)
                return assign_wokwi_slots()
        
    except ValueError:
        safe_print("Invalid input. Please enter a number.", Fore.RED)
        return assign_wokwi_slots()
    except Exception as e:
        safe_print(f"Error updating slot: {e}", Fore.RED)
        return (changes_made, True)

# ============================================================================
# MENU SYSTEM
# ============================================================================

def bridge_menu():
    """Main bridge menu"""
    global menuEntered, wokwiUpdateRate, numAssignedSlots, currentString, noWokwiStuff, disableArduinoFlashing, noArduinocli, arduinoPort, debugWokwi

    safe_print("\n\n         Jumperless App Menu\n", Fore.MAGENTA)
    
    safe_print(" 'menu'    to open the app menu (this menu)", Fore.BLUE)  
    safe_print(" 'wokwi'   to " + ("enable" if noWokwiStuff else "disable") + " Wokwi updates " + ("and just use as a terminal" if not noWokwiStuff else ""), Fore.CYAN)
    safe_print(" 'rate'    to change the Wokwi update rate", Fore.GREEN)
    safe_print(" 'slots'   to assign Wokwi projects to slots - " + str(numAssignedSlots) + " assigned", Fore.YELLOW)
    safe_print(" 'arduino' to " + ("enable" if disableArduinoFlashing else "disable") + " Arduino flashing from wokwi", Fore.RED)
    safe_print(" 'debug'   to " + ("disable" if debugWokwi else "enable") + " Wokwi debug output - " + ("on" if debugWokwi else "off"), Fore.MAGENTA)
    safe_print(" 'update'  to force firmware update - yours is up to date (" + currentString + ")", Fore.BLUE)
    safe_print(" 'status'  to check the serial connection status", Fore.CYAN) 
    safe_print(" 'exit'    to exit the menu", Fore.GREEN)
    
    while menuEntered:
        try:
            choice = input("\n>> ").strip()
            
            if choice == 'menu':
                menuEntered = 0
                ser.write(b'm')
                return
            elif choice == 'wokwi':
                noWokwiStuff = not noWokwiStuff
                safe_print(f"Wokwi updates {'disabled' if noWokwiStuff else 'enabled'}", Fore.CYAN)
                continue
            elif choice == 'arduino':
                if noArduinocli:
                    safe_print("Arduino CLI not available. Cannot enable Arduino flashing.", Fore.RED)
                    safe_print("Install pyduinocli with: pip install pyduinocli", Fore.YELLOW)
                else:
                    disableArduinoFlashing = not disableArduinoFlashing
                    status = "enabled" if not disableArduinoFlashing else "disabled"
                    safe_print(f"Arduino flashing {status}", Fore.GREEN if not disableArduinoFlashing else Fore.YELLOW)
                    if not disableArduinoFlashing and not arduinoPort:
                        safe_print("Warning: No Arduino port configured. Use 'status' to check ports.", Fore.YELLOW)
                continue
            elif choice == 'debug':
                debugWokwi = not debugWokwi
                safe_print(f"Wokwi debug output {'enabled' if debugWokwi else 'disabled'}", Fore.CYAN)
                continue
            elif choice == 'update':
                update_jumperless_firmware(force=True)
                menuEntered = 0
                ser.write(b'm')
                return
            elif choice == 'slots':
                changes_made, return_to_menu = assign_wokwi_slots()
                if return_to_menu:
                    continue
                else:
                    menuEntered = 0
                    ser.write(b'm')
                    return
            elif choice == 'rate':
                try:
                    rate_input = input(f"Current update rate: {wokwiUpdateRate + 0.4}s\nEnter new rate (0.5-50.0): ")
                    new_rate = float(rate_input)
                    if 0.5 <= new_rate <= 50.0:
                        wokwiUpdateRate = new_rate - 0.4
                        safe_print(f"Update rate set to {new_rate}s", Fore.GREEN)
                    else:
                        safe_print("Rate must be between 0.5 and 50.0 seconds", Fore.RED)
                except ValueError:
                    safe_print("Invalid number format", Fore.RED)
                continue
            elif choice == 'status':
                # safe_print(f"\nConnection: {'Connected' if serialconnected else 'Disconnected'}", 
                        #   Fore.GREEN if serialconnected else Fore.RED)
                try:
                    arduinoPortStatus = serial.Serial(arduinoPort, 115200, timeout=0.1).is_open
                    arduinoPortStatus.close()
                    arduinoPortStatus = True
                    # arduinoPortStatus = serial.Serial(arduinoPort, 115200, timeout=0.1).is_open
                    # print(arduinoPortStatus)
                except:
                    arduinoPortStatus = False

                safe_print(f"Jumperless Port: {portName if portName else 'None'} " + ("(Connected)" if serialconnected else "(Disconnected)"), Fore.GREEN if serialconnected else Fore.RED)
                safe_print(f"Arduino Port: {arduinoPort if arduinoPort else 'None'} " + ("(Connectable)" if arduinoPortStatus else "(Busy)"), Fore.GREEN if arduinoPortStatus else Fore.RED)
                safe_print(f"Firmware: {currentString}", Fore.CYAN) 
                safe_print(f"Jumperless V5: {'Yes' if jumperlessV5 else 'No'}", Fore.MAGENTA if jumperlessV5 else Fore.BLUE)
                safe_print(f"Arduino CLI: {'Available' if not noArduinocli else 'Not Available'}" + (" - version: " + get_installed_arduino_cli_version() if not noArduinocli else ""), Fore.CYAN if not noArduinocli else Fore.YELLOW)
                safe_print(f"Arduino Flashing: {'Enabled' if not disableArduinoFlashing and not noArduinocli else 'Disabled'}", Fore.MAGENTA if not disableArduinoFlashing and not noArduinocli else Fore.BLUE)
                # safe_print(f"Arduino CLI Version: {get_installed_arduino_cli_version()}", Fore.CYAN)
                continue
            elif choice == 'exit':
                menuEntered = 0
                ser.write(b'm')
                return
            else:
                menuEntered = 0
                ser.write(b'm')
                safe_print("Exiting menu...", Fore.CYAN)
                return

        except KeyboardInterrupt:
            menuEntered = 0
        except Exception as e:
            safe_print(f"Menu error: {e}", Fore.RED)

# ============================================================================
# ARDUINO FLASHING FUNCTIONS
# ============================================================================

def removeLibraryLines(line):
    """Filter function for Arduino libraries"""
    if line.startswith('#'):
        return False
    if line.startswith('//'):
        return False
    if len(line.strip()) == 0:
        return False
    return True

def findsketchindex(decoded):
    """Find sketch file index in Wokwi project - SIMPLIFIED VERSION"""
    try:
        files = decoded['props']['pageProps']['p']['files']
        for i, file_info in enumerate(files):
            if file_info.get('name', '') == 'sketch.ino':
                return i
    except (KeyError, TypeError, IndexError):
        pass
    return 0

def findlibrariesindex(decoded):
    """Find libraries file index in Wokwi project - SIMPLIFIED VERSION"""
    try:
        files = decoded['props']['pageProps']['p']['files']
        for i, file_info in enumerate(files):
            if file_info.get('name', '') == 'libraries.txt':
                return i
    except (KeyError, TypeError, IndexError):
        pass
    return -1

def finddiagramindex(decoded):
    """Find diagram file index in Wokwi project - SIMPLIFIED VERSION"""
    try:
        files = decoded['props']['pageProps']['p']['files']
        for i, file_info in enumerate(files):
            if file_info.get('name', '') == 'diagram.json':
                return i
    except (KeyError, TypeError, IndexError):
        pass
    return -1

def flash_arduino_sketch_threaded(sketch_content, libraries_content="", slot_number=None):
    """Thread wrapper for Arduino sketch flashing"""
    def flash_worker():
        try:
            result = flash_arduino_sketch(sketch_content, libraries_content, slot_number)
            if result:
                safe_print(f"Arduino flash completed successfully for slot {slot_number}", Fore.GREEN)
            else:
                safe_print(f"Arduino flash failed for slot {slot_number}", Fore.RED)
        except Exception as e:
            safe_print(f"Arduino flash thread error for slot {slot_number}: {e}", Fore.RED)
    
    # Start the flash operation in its own thread
    flash_thread = threading.Thread(target=flash_worker, daemon=True)
    flash_thread.start()
    safe_print(f"Arduino flash started in background for slot {slot_number}...", Fore.CYAN)
    return flash_thread

def flash_arduino_sketch(sketch_content, libraries_content="", slot_number=None):
    """Flash Arduino sketch to connected Arduino"""
    global arduino, arduinoPort, ser, menuEntered, serialconnected
    
    if noArduinocli or disableArduinoFlashing or not arduino:
        safe_print("Arduino flashing is disabled or Arduino CLI not available", Fore.YELLOW)
        return False
    
    if not arduinoPort:
        safe_print("No Arduino port configured", Fore.RED)
        return False
    
    if not sketch_content or len(sketch_content.strip()) < 10:
        safe_print("Invalid or empty sketch content", Fore.RED)
        return False
    
    # Flag to track UART state
    uart_was_connected = False
    arduino_serial = None
    port_available = False

    ser.write(b"A")
    time.sleep(3.5)


    try:
        # Create sketch directory
        sketch_dir = './WokwiSketch'
        compile_dir = './WokwiSketch/compile'
        
        if not os.path.exists(sketch_dir):
            os.makedirs(sketch_dir)
        if not os.path.exists(compile_dir):
            os.makedirs(compile_dir)
        
        # Write sketch file
        sketch_file = os.path.join(sketch_dir, 'WokwiSketch.ino')
        with open(sketch_file, 'w', encoding='utf-8') as f:
            f.write(sketch_content)
        
        safe_print(f"Created sketch file: {sketch_file}", Fore.CYAN)
        if debugWokwi:
            print(sketch_content)
        

        # Handle libraries if provided
        if libraries_content and libraries_content.strip():
            lib_list = libraries_content.strip().split('\n')
            filtered_libs = list(filter(removeLibraryLines, lib_list))
            
            if filtered_libs:
                safe_print("Installing Arduino Libraries...", Fore.CYAN)
                try:
                    arduino.lib.install(filtered_libs)
                    safe_print(f"Installed libraries: {filtered_libs}", Fore.GREEN)
                except Exception as e:
                    safe_print(f"Warning: Could not install some libraries: {e}", Fore.YELLOW)
        
        # Install Arduino AVR core if not already installed
        try:
            cores = ['arduino:avr']
            arduino.core.install(cores, no_overwrite=True)
            safe_print("Arduino AVR core ready", Fore.GREEN)
        except Exception as e:
            safe_print(f"Warning: Arduino core installation issue: {e}", Fore.YELLOW)
        
        # Compile the sketch
        safe_print("Compiling Arduino sketch...", Fore.CYAN)
        # if ser and serialconnected:
        #     try:
        #         ser.write(b"_")  # Progress indicator
        #     except:
        #         pass
        
        try:
            compile_result = arduino.compile(
                sketch_dir,
                port=arduinoPort,
                fqbn="arduino:avr:nano",
                build_path=compile_dir,
                verify=False
            )
            safe_print("Sketch compiled successfully!", Fore.GREEN)
        except Exception as e:
            safe_print(f"Compilation failed: {e}", Fore.RED)
            # Restore original UART state if needed
            if ser and serialconnected and not uart_was_connected:
                try:
                    ser.write(b"a")  # Disconnect UART
                    time.sleep(0.1)
                except:
                    pass
            return False
        

        # Upload to Arduino
        safe_print("Uploading to Arduino...", Fore.CYAN)
        ser.write(b"A")
        time.sleep(1.1)
        
        try:
            # Perform the upload with minimal status updates and single attempt
            # Using custom upload function to ensure -x attempts=1 flag is used
            upload_result = upload_with_attempts_limit(
                sketch_dir,
                arduinoPort,
                "arduino:avr:nano",
                compile_dir,
                "2s"
            )
            
            safe_print("Arduino flashed successfully!", Fore.GREEN)
            
            # Restore original UART state if needed
            if ser and serialconnected and not uart_was_connected:
                try:
                    time.sleep(0.1)
                    ser.write(b"a")  # Disconnect UART
                    time.sleep(0.1)
                except:
                    pass
            
            # Send menu command to return to normal mode
            if ser and serialconnected:
                try:
                    time.sleep(0.1)
                    ser.write(b'm')
                except:
                    pass
            
            return True
            
        except Exception as e:
            safe_print(f"Upload failed: {e}", Fore.RED)
            
            # Restore original UART state if needed
            if ser and serialconnected and not uart_was_connected:
                try:
                    ser.write(b"a")  # Disconnect UART
                    time.sleep(0.1)
                except:
                    pass
            
            # Try to parse JSON error message if available
            try:
                if hasattr(e, 'args') and len(e.args) > 0:
                    error_data = e.args[0]
                    if isinstance(error_data, dict) and '__stdout' in error_data:
                        import json
                        error_json = json.loads(error_data['__stdout'])
                        if 'error' in error_json:
                            safe_print(f"Detailed error: {error_json['error']}", Fore.RED)
            except:
                pass
            return False
    
    except Exception as e:
        safe_print(f"Arduino flashing error: {e}", Fore.RED)
        
        # Restore original UART state if needed
        if ser and serialconnected and not uart_was_connected:
            try:
                ser.write(b"a")  # Disconnect UART
                time.sleep(0.1)
            except:
                pass
        
        return False

def process_wokwi_sketch_and_flash(wokwi_url, slot_number=None):
    """Download Wokwi project and flash Arduino sketch if present - SIMPLIFIED VERSION"""
    global lastsketch, sketch, lastlibraries
    
    if noArduinocli or disableArduinoFlashing:
        return False
    
    try:
        # Convert Wokwi project URL to full page URL
        if "/projects/" in wokwi_url:
            project_id = wokwi_url.split("/projects/")[1].split("/")[0]
            page_url = f"https://wokwi.com/projects/{project_id}"
            if debugWokwi:
                safe_print(f"Fetching project page: {page_url}", Fore.BLUE)
        else:
            safe_print("Invalid Wokwi URL format", Fore.RED)
            return False
        
        # Fetch the project page
        response = requests.get(page_url, timeout=10)
        if response.status_code != 200:
            safe_print(f"Failed to fetch Wokwi project page: {response.status_code}", Fore.RED)
            return False
        
        # Parse HTML
        soup = BeautifulSoup(response.text, 'html.parser')
        
        # Find the JSON data containing the project information
        decoded_data = None
        sketch_content = None
        libraries_content = None
        
        # Approach 0: Direct extraction of visible code if present
        try:
            # Look for the sketch code in the DOM - it might be in pre or code tags
            code_elements = soup.find_all(['pre', 'code'])
            for code_elem in code_elements:
                code_text = code_elem.get_text(strip=True)
                if code_text and 'void setup()' in code_text and 'void loop()' in code_text:
                    sketch_content = code_text
                    if debugWokwi:
                        safe_print("Found sketch.ino directly in page content", Fore.GREEN)
                    break
        except Exception as e:
            if debugWokwi:
                safe_print(f"Failed to extract code from page content: {e}", Fore.YELLOW)
        
        # Approach 1: Try to find embedded project data using regex
        if not sketch_content:
            try:
                # Look for a pattern like e:["$","$L16",null,{"project":...
                matches = re.findall(r'e:\[.*?"project":\{.*?"files":\[(.*?)\]', response.text, re.DOTALL)
                if matches:
                    for match in matches:
                        try:
                            # Extract files array and parse
                            files_json = '[' + match + ']'
                            files_data = json.loads(files_json)
                            
                            # Look for sketch.ino and libraries.txt
                            for file_data in files_data:
                                if isinstance(file_data, dict):
                                    if file_data.get('name') == 'sketch.ino':
                                        sketch_content = file_data.get('content', '')
                                    elif file_data.get('name') == 'libraries.txt':
                                        libraries_content = file_data.get('content', '')
                            
                            if sketch_content:
                                if debugWokwi:
                                    safe_print("Found sketch.ino using embedded JSON approach", Fore.GREEN)
                                break
                        except Exception as e:
                            if debugWokwi:
                                safe_print(f"Failed to parse embedded JSON: {e}", Fore.YELLOW)
                            continue
            except Exception as e:
                if debugWokwi:
                    safe_print(f"Failed in regex approach: {e}", Fore.YELLOW)
        
        # Additional extraction methods (if needed)
        if not sketch_content:
            # Try remaining approaches to find the sketch
            for approach_num in range(2, 6):
                if sketch_content:
                    break
                    
                if approach_num == 2:
                    # Look for raw code section in markdown-style code blocks
                    try:
                        code_blocks = re.findall(r'```\s*(?:arduino|cpp)?\s*(void\s+setup\(\)[\s\S]*?void\s+loop\(\)[\s\S]*?)```', response.text)
                        if code_blocks:
                            sketch_content = code_blocks[0].strip()
                            if debugWokwi:
                                safe_print("Found sketch.ino in code block", Fore.GREEN)
                    except Exception as e:
                        if debugWokwi:
                            safe_print(f"Failed to extract code block: {e}", Fore.YELLOW)
                
                elif approach_num == 3:
                    # Try script tags
                    script_tags = soup.find_all('script')
                    for script in script_tags:
                        if script.string and 'project' in script.string and 'files' in script.string:
                            try:
                                match = re.search(r'project":\s*{.*?"files":\s*\[(.*?)\]', script.string, re.DOTALL)
                                if match:
                                    files_json = '[' + match.group(1) + ']'
                                    files_data = json.loads(files_json)
                                    
                                    for file_data in files_data:
                                        if isinstance(file_data, dict):
                                            if file_data.get('name') == 'sketch.ino':
                                                sketch_content = file_data.get('content', '')
                                            elif file_data.get('name') == 'libraries.txt':
                                                libraries_content = file_data.get('content', '')
                                    
                                    if sketch_content:
                                        if debugWokwi:
                                            safe_print("Found sketch.ino using script tag approach", Fore.GREEN)
                                        break
                            except Exception as e:
                                if debugWokwi:
                                    safe_print(f"Failed to parse script tag: {e}", Fore.YELLOW)
                                continue
                
                elif approach_num == 4:
                    # Direct regex on HTML content
                    try:
                        sketch_pattern = r'"name"\s*:\s*"sketch.ino"\s*,\s*"content"\s*:\s*"(.*?)(?:"\s*[,}])'
                        sketch_matches = re.findall(sketch_pattern, response.text, re.DOTALL)
                        
                        if sketch_matches:
                            raw_content = sketch_matches[0]
                            sketch_content = raw_content.replace('\\n', '\n').replace('\\\"', '\"').replace('\\\\', '\\')
                            if debugWokwi:
                                safe_print(f"Found sketch.ino using direct regex", Fore.GREEN)
                        
                        if not libraries_content:
                            lib_pattern = r'"name"\s*:\s*"libraries.txt"\s*,\s*"content"\s*:\s*"(.*?)(?:"\s*[,}])'
                            lib_matches = re.findall(lib_pattern, response.text, re.DOTALL)
                            if lib_matches:
                                libraries_content = lib_matches[0].replace('\\n', '\n').replace('\\\"', '\"').replace('\\\\', '\\')
                    except Exception as e:
                        if debugWokwi:
                            safe_print(f"Failed to extract content using direct regex: {e}", Fore.YELLOW)
                
                elif approach_num == 5:
                    # Manual extraction
                    if "void setup()" in response.text and "void loop()" in response.text:
                        try:
                            start_idx = response.text.find("void setup()")
                            if start_idx > 0:
                                end_idx = response.text.find("}", response.text.find("void loop()", start_idx))
                                if end_idx > start_idx:
                                    raw_code = response.text[start_idx:end_idx+1]
                                    clean_code = re.sub(r'<[^>]+>', '', raw_code)
                                    sketch_content = clean_code.strip()
                                    if debugWokwi:
                                        safe_print("Extracted sketch directly from page content", Fore.GREEN)
                        except Exception as e:
                            if debugWokwi:
                                safe_print(f"Failed to extract sketch manually: {e}", Fore.YELLOW)
        
        # Check if we have found a valid sketch
        if sketch_content:
            # Clean up the content - remove excess whitespace and normalize line endings
            sketch_content = sketch_content.strip()
            sketch_content = re.sub(r'\r\n', '\n', sketch_content)
            sketch_content = re.sub(r'\n{3,}', '\n\n', sketch_content)
            
            # Store in the sketch array
            sketch[slot_number] = sketch_content
            
            # Check if sketch has changed since last time
            if lastsketch[slot_number] != sketch_content:
                lastsketch[slot_number] = sketch_content
                
                if debugWokwi:
                    safe_print(f"Found new sketch for slot {slot_number}:", Fore.GREEN)
                    safe_print(sketch_content[:200] + ('...' if len(sketch_content) > 200 else ''), Fore.CYAN)
                
                # Flash the Arduino if the sketch is valid
                if len(sketch_content) > 10:
                    safe_print(f"\nNew Arduino sketch for slot {slot_number} - flashing...", Fore.MAGENTA)
                    
                    # Only flash once
                    try:
                        # Start threaded flash and return the thread object
                        flash_thread = flash_arduino_sketch_threaded(sketch_content, libraries_content, slot_number)
                        return flash_thread is not None
                    except Exception as e:
                        safe_print(f"Error during Arduino flashing: {e}", Fore.RED)
                        return False
                else:
                    safe_print(f"Sketch too short. Not flashing.", Fore.YELLOW)
            else:
                if debugWokwi:
                    safe_print(f"Sketch for slot {slot_number} unchanged", Fore.BLUE)
                
        else:
            safe_print(f"No sketch found for slot {slot_number}", Fore.YELLOW)
        
        return False
        
    except Exception as e:
        safe_print(f"Error processing Wokwi sketch: {e}", Fore.RED)
        return False

# ============================================================================
# THREADING FUNCTIONS
# ============================================================================

def check_presence(correct_port, interval):
    """Monitor serial port presence and reconnect if needed"""
    global ser, portName, justreconnected, serialconnected, portNotFound, updateInProgress
    while True:
        if updateInProgress == 0:
            try:
                # Check if the port is actually available in the system
                port_found = False
                try:
                    ports = serial.tools.list_ports.comports()
                    for port in ports:
                        if correct_port in port.device:
                            port_found = True
                            break
                except Exception:
                    port_found = False
                
                with serial_lock:
                    # Determine if we need to reconnect based on actual port availability
                    currently_connected = (serialconnected and 
                                         ser is not None and 
                                         ser.is_open)
                    
                    needs_reconnection = (not currently_connected and port_found) or (currently_connected and not port_found)
                
                if port_found and not currently_connected:
                    # Port is available but we're not connected, attempt to reconnect immediately
                    try:
                        with serial_lock:
                            if ser:
                                try:
                                    ser.close()
                                except:
                                    pass
                            
                            ser = serial.Serial(correct_port, 115200, timeout=0.5)
                            serialconnected = 1
                            portNotFound = 0
                            justreconnected = 1
                        
                        safe_print(f"Reconnected to {correct_port}", Fore.GREEN)
                        
                    except Exception as e:
                        with serial_lock:
                            if ser:
                                try:
                                    ser.close()
                                except:
                                    pass
                            ser = None
                            serialconnected = 0
                            portNotFound = 1
                            
                elif not port_found and currently_connected:
                    # Port not found in system but we think we're connected, mark as disconnected
                    with serial_lock:
                        if ser:
                            try:
                                ser.close()
                            except:
                                pass
                        ser = None
                        serialconnected = 0
                        portNotFound = 1
                    safe_print(f"Port {correct_port} disconnected", Fore.YELLOW)
                
            except Exception as e:
                safe_print(f"Error in port monitoring: {e}", Fore.RED)
            
            time.sleep(interval)
        else:
            time.sleep(0.1)

def serial_term_in():
    """Handle incoming serial data with reliable byte-by-byte reading"""
    global serialconnected, ser, menuEntered, portNotFound, justreconnected, updateInProgress
    
    while True:
        if menuEntered == 0 and updateInProgress == 0:
            try:
                if ser and ser.is_open and ser.in_waiting > 0:
                    input_buffer = b''
                    
                    # Read byte-by-byte until buffer stabilizes
                    while serialconnected and ser and ser.is_open:
                        try:
                            if ser.in_waiting > 0:
                                in_byte = ser.read(1)
                                if in_byte:
                                    input_buffer += in_byte
                            
                            # Check if buffer has stabilized (no new data for a short time)
                            if ser.in_waiting == 0:
                                time.sleep(0.005)  # Wait a bit to see if more data arrives
                                if ser.in_waiting == 0:
                                    break  # Buffer is stable, process what we have
                        except (serial.SerialException, serial.SerialTimeoutException):
                            break
                    
                    if input_buffer:
                        try:
                            # Decode the complete buffer
                            decoded_string = input_buffer.decode('utf-8', errors='ignore')
                            print(decoded_string, end='')
                            
                            with serial_lock:
                                portNotFound = 0
                        except Exception:
                            pass
                else:
                    time.sleep(0.005)
                        
            except (serial.SerialException, serial.SerialTimeoutException):
                # Only handle serial exceptions if not updating firmware
                if updateInProgress == 0:
                    with serial_lock:
                        if ser: 
                            try: 
                                ser.close()
                            except:
                                pass
                        ser = None
                        portNotFound = 1
                        serialconnected = 0
                time.sleep(0.02)
            except Exception:
                pass
        else:
            time.sleep(0.01)
        
        time.sleep(0.001)  # Very short sleep to prevent excessive CPU usage

def serial_term_out():
    """Handle outgoing serial commands with command history support"""
    global serialconnected, ser, menuEntered, forceWokwiUpdate, noWokwiStuff, justreconnected, portNotFound, updateInProgress
    
    while True:
        if menuEntered == 0 and updateInProgress == 0:
            try:
                # Clear any pending input buffer before reading
                if hasattr(sys.stdin, 'flush'):
                    sys.stdin.flush()
                
                # Use readline for command history if available, otherwise fallback to input()
                if READLINE_AVAILABLE:
                    try:
                        output_buffer = input().strip()  # readline automatically handles history
                    except (EOFError, KeyboardInterrupt):
                        # Handle Ctrl+D or Ctrl+C gracefully
                        if menuEntered == 0:
                            continue
                        else:
                            break
                else:
                    output_buffer = input().strip()  # Strip whitespace
                
                # Skip empty commands
                if not output_buffer:
                    time.sleep(0.01)
                    continue
                
                # Add command to history if readline is available and it's not a duplicate
                if READLINE_AVAILABLE and output_buffer:
                    # Check if this command is different from the last one to avoid duplicates
                    history_length = readline.get_current_history_length()
                    if history_length == 0 or (history_length > 0 and 
                        readline.get_history_item(history_length) != output_buffer):
                        readline.add_history(output_buffer)
                
                # Handle special commands - check if it's a menu command
                menu_commands = ['menu', 'slots', 'wokwi', 'update', 'status', 'rate', 'exit', 'arduino', 'debug']
                
                if output_buffer.lower() in menu_commands:
                    # print("*", output_buffer, "*")
                    menuEntered = 1
                    bridge_menu()
                    continue
                
                # Send to serial port (only non-empty commands)
                with serial_lock:
                    if serialconnected and ser and ser.is_open:
                        try:
                            ser.write(output_buffer.encode('ascii'))
                            # Check for reset command
                            if output_buffer.lower() == 'r':
                                justreconnected = 1
                        except Exception:
                            # Only mark as disconnected if not updating firmware
                            if updateInProgress == 0:
                                portNotFound = 1
                                serialconnected = 0
                    else:
                        if updateInProgress == 0:
                            safe_print("Serial not connected", Fore.YELLOW)
                        
            except KeyboardInterrupt:
                break
            except EOFError:
                # Handle end of file (Ctrl+D on Unix, Ctrl+Z on Windows)
                time.sleep(0.1)
                continue
            except Exception as e:
                # Handle any other input exceptions
                safe_print(f"Input error: {e}", Fore.RED)
                time.sleep(0.1)
                continue
        else:
            time.sleep(0.1)

# ============================================================================
# WOKWI PROCESSING FUNCTIONS
# ============================================================================

def map_pin_name(pin_name_orig, is_jumperless_v5):
    """Maps a Wokwi pin name string to a Jumperless pin number string"""
    pin_name = pin_name_orig
    
    if pin_name_orig.startswith('pot1:SIG'): 
        pin_name = "106"
    elif pin_name_orig.startswith('pot2:SIG'): 
        pin_name = "107"
    elif pin_name_orig.startswith('logic1:'):
        details = pin_name_orig.split(':')[1]
        map_logic1 = {'0':"110", '1':"111", '2':"112", '3':"113", 
                     '4':"108", '5':"109", '6':"116", '7':"117", 'D':"114"}
        pin_name = map_logic1.get(details, pin_name_orig)
    elif pin_name_orig.startswith("bb1:"):
        part = pin_name_orig.split(':')[1].split('.')[0]
        if part.endswith('t'): 
            pin_name = part[:-1]
        elif part.endswith('b'): 
            pin_name = str(int(part[:-1]) + 30)
        elif part.endswith('n') or part == "GND": 
            pin_name = "100"
        elif part.endswith('p'): 
            pin_name = ("101" if part.startswith('t') else "102") if is_jumperless_v5 else ("105" if part.startswith('t') else "103")
    elif pin_name_orig.startswith("nano:"):
        part = pin_name_orig.split(':')[1]
        map_nano = {"GND":"100", "AREF":"85", "B0":"85", "RESET":"84", "RST":"84", 
                   "B1":"84", "5V":"105", "3.3V":"103", "TX":"71", "TX1":"71", "RX":"70", "RX0":"70"}
        if part in map_nano: 
            pin_name = map_nano[part]
        elif part.startswith("A") and part != "AREF" and len(part) > 1 and part[1:].isdigit(): 
            pin_name = str(int(part[1:]) + 86)
        elif part.isdigit(): 
            pin_name = str(int(part) + 70)
        elif part.startswith("D") and len(part) > 1 and part[1:].isdigit(): 
            pin_name = str(int(part[1:]) + 70)
    
    return pin_name

def construct_jumperless_command(connections, is_jumperless_v5):
    """Constructs the Jumperless command string from Wokwi connections"""
    if not connections:
        return "{ }"
    
    temp_p_list = []
    for conn_pair in connections:
        conn1_orig, conn2_orig = str(conn_pair[0]), str(conn_pair[1])
        
        conn1 = map_pin_name(conn1_orig, is_jumperless_v5)
        conn2 = map_pin_name(conn2_orig, is_jumperless_v5)
        
        if conn1.isdigit() and conn2.isdigit():
            temp_p_list.append(f"{conn1}-{conn2}")
    
    return "{ " + ",".join(temp_p_list) + ", }" if temp_p_list else "{ }"

# ============================================================================
# MAIN EXECUTION
# ============================================================================

def main():
    """Main application entry point"""
    global menuEntered, noWokwiStuff, currentSlotUpdate, forceWokwiUpdate
    global lastDiagram, diagram, serialconnected, ser, justreconnected
    
    # Initialize command history
    setup_command_history()
    
    # Initialize
    # safe_print("Jumperless Bridge App", Fore.MAGENTA)

    safe_print("""

                                            ▄▄███████████████████▄▄▄
                                      ▄ ████▀███████████████████████████▄▄
                                  ▄█████ ▀▀▀ ████████████████████████████████▄
                               ▄█████████    ███████████████████████████████████▄
                            ▄█████████████▄  █████████████████████████████████████▌▄
                          ██████████▀██████   █████████████████████████████████████▀
                       ╓██▀█▀██████╙ ╫╬█ ██   ╫██████████████████████████████████╙""", Fore.MAGENTA)
    safe_print("""                      ╣███ ║█─ ╙║▌   █ ▀ ╙ ▌   ███████████████████████████████▀└       ▄█▄
                    ▄████  ╙█             ▀▀    ▀████████████████████████████─      ╓██████
                   █████                   ╒    ╬█████████████████████████▀       ▄█████████▄
                  █████▌                        └─▀█████████████████████▀       ▄████████████▌
                 █████                          └▀████████████████████╙       ▄███████████████▌
                ██████─                          ▀▄█████████████████└       ███████▀▀▀▀████████▌""", Fore.BLUE)
                

    safe_print("""               █████████                         █████████████████─      ╓██████▀   ▄███████████▄
              ▐█████████─                          ████████████▌       ▄█████▀     ██████████████
              ██████████▌████                      ╙█████████▀       ▄██████▌     ███████████████▌
             ▐███████████▀▀╙└                        ██████╙       ▄██████████    ████████████████""", Fore.CYAN)
                               

    safe_print("""▀▀██▄▄▄   ▄▄▄▀▀▀▀▀▀▀▀▀                                ╜██└       ▄█████████████    ███████████████▄
   ▀█████████             ▄▄▄▄▄▌ ▄██▄                         ╓▌███████████████▒      ║████████████
             █████████████████████████▄                       ╙╙╙╙╙╙▀███████████       ████████████
             ╫█████████████████████████▌                               └╙███████       ████████████""", Fore.GREEN)
    safe_print("""             ▐██████████████████████████▄                                    └╙██      ╙██████████▀
              ███████████████████████████████▀                                          ║█████████
              ║████████████████████████████▀       ╓▄                                    ████████░
               ██████████████████████████▀       ╓██████▄▄                             ╓█████████""", Fore.YELLOW)
    safe_print("""               ║███████████████████████▀       ▄███████████▌                ╙███████████████████
                ╫████████████████████╙       ▄████████████████▄               └████████████████▀
                 ╫████████████████▀└       ▄████████████████████▄                 └▀██████████▀
                  ║█████████████▀       ╓▄█████████████████████████╗▄                      ╙╙╓▄▄
                   ╙██████████▀       ╓████████████████████████████████▌╗▄                  ╓█████████████▌
                     ▀██████▀       ▄████████████████████████████████████████▄▄▄▄▄         ▄███████▀▀██████""", Fore.RED)
    safe_print("""                      ╙███▀       ▄█████████████████████████████████████████████████████▀              ╙███▌
                        ▀       ▄█████████████████████████████████████████████████████▀                  ╙██▌
                              ▄█████████████████████████████████████████████████████▀                      ██
                             ▀███████████████████████████████████████████████████▀╙
                                ╙█████████████████████████████████████████████▀╙
                                   ╙▀█████████████████████████████████████▀▀─
                                        ╙▀▀██████████████████████████▀▀└
                                             └╙╜▀▀▀▀███████▀▀▀▀╙╙└


""", Fore.MAGENTA)
    create_directories()
    
    # Open serial connection
    open_serial()
    
    if not serialconnected:
        safe_print("Could not establish serial connection. Exiting.", Fore.RED)
        return
    
    # Check firmware
    check_if_fw_is_old()
    
    # Setup Arduino CLI
    setup_arduino_cli()
    
    # Check Windows volume detection capability
    if sys.platform == "win32" and not WIN32_AVAILABLE:
        safe_print("Windows volume detection: Using fallback method", Fore.YELLOW)
        safe_print("For better Windows drive detection, install pywin32: pip install pywin32", Fore.CYAN)
    
    # Count assigned slots
    count_assigned_slots()
    
    # Start monitoring threads
    port_monitor = threading.Thread(target=check_presence, args=(portName, 0.1), daemon=True)
    port_monitor.start()
    
    serial_in = threading.Thread(target=serial_term_in, daemon=True)
    serial_in.start()
    
    serial_out = threading.Thread(target=serial_term_out, daemon=True)
    serial_out.start()
    
    time.sleep(0.1)
    safe_print("Type 'menu' for App Menu", Fore.CYAN)
    if READLINE_AVAILABLE:
        safe_print("Use ↑/↓ arrow keys for command history, Tab for completion", Fore.BLUE)
    # Send initial menu command
    try:
        with serial_lock:
            if ser and ser.is_open:
                ser.write(b'm')
    except:
        pass
    
    # Main loop
    if noWokwiStuff:
        # safe_print("Wokwi functionality disabled. Use 'menu' command for options.", Fore.YELLOW)
        while noWokwiStuff:
            if menuEntered:
                time.sleep(0.1)
                # bridge_menu()
            time.sleep(0.1)
    else:
        # safe_print("Starting Wokwi monitoring...", Fore.GREEN)
        
        
        # Wokwi main loop
        while True:
            if noWokwiStuff: # if Wokwi is disabled, just chill
                time.sleep(0.1)
                continue
            if menuEntered:
                # bridge_menu()
                continue
            
            # Skip Wokwi processing during firmware updates
            if updateInProgress:
                time.sleep(0.1)
                continue
            
            # Handle reconnection
            with serial_lock:
                if justreconnected:
                    with wokwi_update_lock:
                        lastDiagram = blankDiagrams[:]
                        forceWokwiUpdate = 1
                        currentSlotUpdate = 0
                    justreconnected = 0
                    # safe_print("Reconnected - forcing Wokwi update", Fore.GREEN)
            
            # Process Wokwi slots and local files
            if numAssignedSlots > 0:
                with wokwi_update_lock:
                    # Find next valid slot (check both Wokwi URLs and local files)
                    found_slot = False
                    found_local_file = False
                    for i in range(8):
                        check_index = (currentSlotUpdate + i) % 8
                        if slotURLs[check_index] != '!' or slotFilePaths[check_index] != '!':
                            currentSlotUpdate = check_index
                            found_slot = slotURLs[check_index] != '!'
                            found_local_file = slotFilePaths[check_index] != '!'
                            break
                    
                    if found_slot:
                        # Process Wokwi URL
                        api_url = slotAPIurls[currentSlotUpdate]
                        current_wokwi_url = slotURLs[currentSlotUpdate]
                        
                        try:
                            # Fetch Wokwi data
                            response = requests.get(api_url, timeout=5)
                            if response.status_code == 200:
                                wokwi_data = response.json()
                                
                                # Process connections
                                connections = wokwi_data.get('connections', [])
                                command = construct_jumperless_command(connections, jumperlessV5)
                                
                                # Create a stable representation for comparison
                                # Normalize connections by converting to tuples and sorting consistently
                                normalized_connections = []
                                for conn in connections:
                                    if len(conn) >= 2:
                                        # Ensure consistent ordering within each connection pair
                                        conn_pair = tuple(sorted([str(conn[0]), str(conn[1])]))
                                        normalized_connections.append(conn_pair)
                                
                                # Sort all connections for consistent comparison
                                normalized_connections.sort()
                                current_diagram_hash = str(normalized_connections)
                                
                                # Check if update needed (only for this specific slot)
                                needs_update = (lastDiagram[currentSlotUpdate] != current_diagram_hash or 
                                              (forceWokwiUpdate and currentSlotUpdate == 0))
                                
                                if debugWokwi:
                                    safe_print(f"Slot {currentSlotUpdate}: Hash={current_diagram_hash[:50]}{'...' if len(current_diagram_hash) > 50 else ''}", Fore.BLUE)
                                    safe_print(f"Slot {currentSlotUpdate}: Needs update={needs_update}, Connections={len(connections)}", Fore.BLUE)
                                
                                if needs_update:
                                    lastDiagram[currentSlotUpdate] = current_diagram_hash
                                    
                                    # Send to Jumperless (only if not updating firmware)
                                    if updateInProgress == 0:
                                        with serial_lock:
                                            if serialconnected and ser and ser.is_open:
                                                try:
                                                    cmd = f"o Slot {currentSlotUpdate} f {command}".encode()
                                                    ser.write(cmd)
                                                    safe_print(f"Updated slot {currentSlotUpdate}", Fore.GREEN)
                                                    if debugWokwi:
                                                        safe_print(f"Command sent: {command}", Fore.CYAN)
                                                except Exception as e:
                                                    safe_print(f"Serial write error: {e}", Fore.RED)
                                                    portNotFound = 1
                                                    serialconnected = 0
                                
                                # Check for Arduino sketch changes and flash if needed
                                if not noArduinocli and not disableArduinoFlashing and updateInProgress == 0:
                                    try:
                                        # Process Arduino sketch for this slot
                                        process_wokwi_sketch_and_flash(current_wokwi_url, currentSlotUpdate)
                                    except Exception as e:
                                        if debugWokwi:
                                            safe_print(f"Arduino sketch processing error for slot {currentSlotUpdate}: {e}", Fore.YELLOW)
                                
                                # Reset forceWokwiUpdate after processing all slots once
                                if forceWokwiUpdate and currentSlotUpdate == 0:
                                    forceWokwiUpdate = 0
                                    
                                # Move to next slot (only process one slot per loop iteration)
                                currentSlotUpdate = (currentSlotUpdate + 1) % 8
                                
                        except Exception as e:
                            safe_print(f"Error processing Wokwi data: {e}", Fore.RED)
                            # Move to next slot even on error to prevent getting stuck
                            currentSlotUpdate = (currentSlotUpdate + 1) % 8
                    
                    elif found_local_file:
                        # Process local .ino file
                        try:
                            # Check for Arduino sketch changes and flash if needed
                            if not noArduinocli and not disableArduinoFlashing and updateInProgress == 0:
                                process_local_file_and_flash(currentSlotUpdate)
                            
                            # Move to next slot (only process one slot per loop iteration)
                            currentSlotUpdate = (currentSlotUpdate + 1) % 8
                            
                        except Exception as e:
                            safe_print(f"Error processing local file for slot {currentSlotUpdate}: {e}", Fore.RED)
                            # Move to next slot even on error to prevent getting stuck
                            currentSlotUpdate = (currentSlotUpdate + 1) % 8
            else:
                time.sleep(1)
            time.sleep(wokwiUpdateRate)

def setup_command_history():
    """Setup command history with persistent storage"""
    if not READLINE_AVAILABLE:
        return
    
    history_file = os.path.join(os.path.expanduser("~"), ".jumperless_history")
    
    # Load existing history
    try:
        if os.path.exists(history_file):
            readline.read_history_file(history_file)
    except Exception:
        pass  # Ignore errors loading history
    
    # Set up tab completion
    setup_tab_completion()
    
    # Set up automatic history saving
    import atexit
    def save_history():
        try:
            readline.write_history_file(history_file)
        except Exception:
            pass  # Ignore errors saving history
    
    atexit.register(save_history)

def setup_tab_completion():
    """Setup tab completion for commands from history"""
    if not READLINE_AVAILABLE:
        return
    
    def completer(text, state):
        """Tab completion function that uses command history"""
        # Only return the most recent match for state 0, nothing for other states
        if state > 0:
            return None
            
        # Get all commands from history that start with the typed text
        history_length = readline.get_current_history_length()
        
        # Search backwards through history to find the most recent match
        for i in range(history_length, 0, -1):
            try:
                history_item = readline.get_history_item(i)
                if history_item and history_item.lower().startswith(text.lower()):
                    return history_item
            except:
                continue
        
        # No match found
        return None
    
    # Set the completer function
    readline.set_completer(completer)
    
    # Configure tab completion behavior
    readline.parse_and_bind("tab: complete")
    
    # Set completion display options
    try:
        # Show all matches if there are multiple
        readline.parse_and_bind("set show-all-if-ambiguous off")
        # Don't show hidden files in completion
        readline.parse_and_bind("set match-hidden-files off")
        # Complete on first tab press
        readline.parse_and_bind("set show-all-if-unmodified off")
    except:
        pass  # Some systems might not support these options

def get_command_suggestions():
    """Get list of common commands for tab completion"""
    # This function is now only used for reference/documentation
    # Tab completion uses actual command history instead
    return [
        # App commands
        'menu', 'slots', 'wokwi', 'skip', 'rate', 'update', 'status', 'exit', 'help',
        # Jumperless device commands
        'r', 'm', 'f', 'n', 'l', 'b', 'c', 'x', 'clear', 'o', '?',
    ]

def upload_with_attempts_limit(sketch_dir, arduino_port, fqbn, input_dir, discovery_timeout="2s"):
    """Custom upload function that calls arduino-cli directly with attempts limit"""
    global ser, arduinoPort
    notInSyncString = "avrdude: stk500_recv(): programmer is not responding"
    arduino_serial = None
    
    try:
        import subprocess
        # ser.write(b"r")
        # time.sleep(0.5)
        # ser.write(b"r")
        
        # Get the arduino-cli path
        cli_path = None
        cli_paths = [
            resource_path("arduino-cli.exe" if sys.platform == "win32" else "arduino-cli"),
            "./arduino-cli.exe" if sys.platform == "win32" else "./arduino-cli",
            "arduino-cli"  # System PATH
        ]
        
        for path in cli_paths:
            if os.path.isfile(path) or shutil.which(path if path == "arduino-cli" else None):
                cli_path = path
                break
        
        if not cli_path:
            raise Exception("arduino-cli not found")
        
        # Ensure the Arduino port is closed before starting upload
        try:
            # Check if the port is open and close it
            arduino_serial = serial.Serial(arduino_port, 115200, timeout=0.5)
            # arduino_serial.close()
            safe_print(f"Arduino port {arduino_port} open before upload", Fore.CYAN)
        except Exception as port_error:
            # Port might already be closed or not exist
            safe_print(f"Note: Arduino port {arduino_port} not accessible, make sure it's not in use by something else", Fore.RED)
            safe_print(f"{port_error}", Fore.RED)
            try:
                arduino_serial.close()
            except:
                pass

            raise Exception("Could not open Arduino port")
        # Construct the command with attempts limit
        cmd = [
            cli_path,
            "upload",
            "-p", arduino_port,
            "-b", fqbn,
            "--input-dir", input_dir,
            "--discovery-timeout", discovery_timeout,
            "-v",
            # "--upload-property", "attempts=1",  # This is the key flag we want to add
            # "--upload-property", "avrdude.config.file=" + os.path.join(os.path.dirname(os.path.abspath(__file__)), "avrdudeCustom.conf"),
            sketch_dir
        ]
        
        if debugWokwi:
            safe_print(f"Running: {' '.join(cmd)}", Fore.BLUE)
        
        # Run the command with real-time output streaming
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, 
                                 text=True, bufsize=1)
        
        output_lines = []
        start_time = time.time()
        timeout = 45  # 45 second timeout
        
        # Stream output in real-time
        while True:
            # Check for timeout
            if time.time() - start_time > timeout:
                process.terminate()
                try:
                    process.wait(timeout=1)  # Give it 1 second to terminate gracefully
                except subprocess.TimeoutExpired:
                    process.kill()  # Force kill if it doesn't terminate
                raise Exception(f"Upload timed out after {timeout} seconds")
            
            output = process.stdout.readline()

            if output == '' and process.poll() is not None:
                break
            if notInSyncString in output:
                # ser.write(b"?")
                safe_print("Arduino not in sync, try pressing the reset button", Fore.RED)
                safe_print(f"{output.strip()}", Fore.RED)

            elif output:
                output_lines.append(output.strip())
                # Print output in real-time with a prefix to distinguish it
                safe_print(f"{output.strip()}", Fore.CYAN)
            else:
                # No output, sleep briefly to prevent busy waiting
                time.sleep(0.1)
        
        # Wait for process to complete and get return code
        return_code = process.poll()
        
        if return_code != 0:
            full_output = '\n'.join(output_lines)
            raise Exception(f"Upload failed with return code {return_code}: {full_output}")
        
        returnValue = {"success": True, "stdout": '\n'.join(output_lines), "stderr": ""}
        # safe_print(returnValue, Fore.GREEN)
        
        return returnValue
        
    except Exception as e:
        raise Exception(f"Custom upload failed: {e}")
    finally:
        # Ensure the Arduino port is closed in all cases (success, error, or timeout)
        if arduino_serial is not None:
            try:
                if arduino_serial.is_open:
                    arduino_serial.close()
                    safe_print(f"Closed Arduino port {arduino_port} after upload", Fore.YELLOW)
            except Exception as close_error:
                safe_print(f"Note: Error closing Arduino port: {close_error}", Fore.YELLOW)

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        safe_print("\nExiting...", Fore.YELLOW)
    except Exception as e:
        safe_print(f"Fatal error: {e}", Fore.RED)
    # finally:
        # Cleanup
        # try:
        #     with serial_lock:
        #         if ser and ser.is_open:
        #             ser.close()
        # except:
        #     pass