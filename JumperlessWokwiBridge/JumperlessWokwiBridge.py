# SPDX-License-Identifier: MIT
# Kevin Santo Cappuccio
# Jumperless Bridge App
# KevinC@ppucc.io
#

App_Version = "1.1.1.9"
new_requirements = True



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
import tempfile


# Try to import packaging for robust version comparison
try:
    from packaging import version
    PACKAGING_AVAILABLE = True
except ImportError:
    PACKAGING_AVAILABLE = False
    # Fallback version comparison will be used

# Command history support
try:
    import readline
    READLINE_AVAILABLE = True
    # Configure readline for better command history
    readline.set_history_length(100)  # Keep last 100 commands
    # Enable tab completion if available
    # try:
    #     readline.parse_and_bind("")
    # except:
    #     pass
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

# Enhanced keyboard input support
if sys.platform == "win32":
    try:
        from pynput import keyboard as pynput_keyboard
        PYINPUT_AVAILABLE = True
    except ImportError:
        PYINPUT_AVAILABLE = False

# Interactive mode will use termios on Unix-like systems
import shutil

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
arduino_flash_lock = threading.Lock()  # Prevent concurrent Arduino uploads

# Global process tracking for cleanup
active_processes = []
active_threads = []

# Interactive mode settings
interactive_mode = False
original_settings = None


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
forceArduinoFlash = 0  # Global flag to force Arduino flash regardless of content changes

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

# App Update URLs
app_update_repo = "Architeuthis-Flux/JumperlessV5"  # Repository for app updates
app_script_name = "JumperlessWokwiBridge.py"
app_requirements_name = "requirements.txt"

# Debug/Testing settings for app updates
debug_app_update = False # Set to True to use local file for testing
debug_test_file = "jumperlesswokwibridgecopyfortesting.py"  # Local test file

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
                
                # Check if content has actually changed OR if force flash is requested
                global forceArduinoFlash
                content_changed = lastsketch[slot_number] != content
                should_flash = content_changed or forceArduinoFlash
                
                if should_flash:
                    lastsketch[slot_number] = content
                    
                    if forceArduinoFlash:
                        safe_print(f"\nForce flashing local file for slot {slot_number}: {os.path.basename(file_path)}", Fore.MAGENTA)
                        forceArduinoFlash = 0  # Reset flag after use
                    else:
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
                        forceArduinoFlash = 0  # Reset flag on error too
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
        port_list = []
        
        for port in ports:
            # Get basic info
            device = port.device
            description = port.description
            hwid = port.hwid
            interface = getattr(port, 'interface', None)
            
            # Debug: Check for additional attributes that might contain function info
            additional_attrs = {}
            for attr_name in ['name', 'subsystem', 'serial_number', 'location', 'manufacturer', 'product', 'vid', 'pid']:
                try:
                    attr_value = getattr(port, attr_name, None)
                    if attr_value:
                        additional_attrs[attr_name] = attr_value
                except:
                    pass
            
            port_list.append((device, description, hwid, interface, additional_attrs))
        
        return port_list
    except Exception as e:
        safe_print(f"Error getting serial ports: {e}", Fore.RED)
        return []

def parse_hardware_id(hwid, des='unknown'):
    """Parse hardware ID to extract VID and PID"""
    # safe_print(f"hwid: {hwid}", Fore.CYAN)
    # safe_print(f"description: {des}", Fore.CYAN)
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

def is_jumperless_device(desc, pid, interface=None):
    """Check if device is a Jumperless device"""
    return (desc == "Jumperless" or 
            pid in ["ACAB", "1312"] or 
            "jumperless" in desc.lower())

def parse_firmware_version(response_str):
    """Extract and parse firmware version from response string"""
    global jumperlessFirmwareString, jumperlessFirmwareNumber, jumperlessV5
    
    try:
        # Look for the firmware version line
        lines = response_str.split('\n')
        for line in lines:
            # Clean up the line - remove carriage returns and extra whitespace
            clean_line = line.replace('\r', '').strip()
            
            if "Jumperless firmware version:" in clean_line:
                jumperlessFirmwareString = clean_line
                
                # Extract version part after the colon
                if ':' in clean_line:
                    version_part = clean_line.split(':', 1)[1].strip()
                    
                    # Clean version string - remove any non-digit/dot characters
                    import re
                    version_clean = re.sub(r'[^\d\.]', '', version_part)
                    
                    if version_clean:
                        version_numbers = version_clean.split('.')
                        # Filter out empty strings
                        version_numbers = [v for v in version_numbers if v]
                        
                        if len(version_numbers) >= 3:
                            jumperlessFirmwareNumber = version_numbers[:3]
                            try:
                                if int(version_numbers[0]) >= 5:
                                    jumperlessV5 = True
                            except ValueError:
                                pass
                        elif len(version_numbers) >= 1:
                            # Handle shorter version numbers
                            jumperlessFirmwareNumber = version_numbers + ['0'] * (3 - len(version_numbers))
                            try:
                                if int(version_numbers[0]) >= 5:
                                    jumperlessV5 = True
                            except ValueError:
                                pass
                    
                    if debugWokwi:
                        safe_print(f"Parsed version: {version_clean} -> {jumperlessFirmwareNumber}", Fore.CYAN)
                    
                    return True
    except Exception as e:
        if debugWokwi:
            safe_print(f"Error parsing firmware version: {e}", Fore.YELLOW)
    
    return False

def find_main_port(jumperless_ports, force_quit_python=False):
    """Find the main Jumperless port using '?' query"""
    
    for port_name, desc, hwid, interface, additional_attrs in jumperless_ports:
        try:
            with serial.Serial(port_name, 115200, timeout=1) as test_port:
                if debugWokwi:
                    safe_print(f"Testing {port_name} for main port", Fore.CYAN)
                
                # Clear buffers thoroughly
                test_port.reset_input_buffer()
                test_port.reset_output_buffer()
                time.sleep(0.6)
                
                # Send ? to check for firmware response
                if force_quit_python:
                    quit_command = "\x11"
                    test_port.write(quit_command.encode('utf-8', errors='ignore'))
                    # print(quit_command)
                    time.sleep(0.25)
                    test_port.write(quit_command.encode('utf-8', errors='ignore'))
                    time.sleep(0.25)
                    test_port.write(quit_command.encode('utf-8', errors='ignore'))
                    time.sleep(0.25)
                    test_port.write(quit_command.encode('utf-8', errors='ignore'))
                    time.sleep(0.25)
                    test_port.write(quit_command.encode('utf-8', errors='ignore'))
                    time.sleep(0.25)
                    test_port.write(quit_command.encode('utf-8', errors='ignore'))
                    time.sleep(0.25)
                    test_port.write(quit_command.encode('utf-8', errors='ignore'))
                    test_port.flush()
                    time.sleep(0.25)
                    test_port.write(quit_command.encode('utf-8', errors='ignore'))
                    time.sleep(0.25)
                    print(quit_command)
                    
                    test_port.flush()
                
                # Send ? to check for firmware response
                test_port.write(b'?')
                test_port.flush()
                time.sleep(0.85)  # Give more time for response
                
                if test_port.in_waiting > 0:
                    response_buffer = b''
                    start_time = time.time()
                    
                    # Read all available data with longer timeout
                    while time.time() - start_time < 1.3:
                        if test_port.in_waiting > 0:
                            response_buffer += test_port.read(test_port.in_waiting)
                            time.sleep(0.1)  # Small delay to catch additional data
                        else:
                            time.sleep(0.05)
                            # If no new data for 100ms, assume we have the complete response
                            if test_port.in_waiting == 0:
                                break
                    
                    if response_buffer:
                        try:
                            response_str = response_buffer.decode('utf-8', errors='ignore')
                            
                            if debugWokwi:
                                safe_print(f"Raw response from {port_name}: {repr(response_str[:200])}", Fore.BLUE)
                            
                            # Check if this looks like a firmware response
                            if "Jumperless firmware version:" in response_str or "firmware" in response_str.lower():
                                # safe_print(f"Found main port: {port_name}", Fore.GREEN)
                                
                                # Parse firmware info
                                if parse_firmware_version(response_str):
                                    test_port.write(b'\x10')
                                    test_port.flush()
                                    return port_name
                                else:
                                    # Even if parsing failed, this is still the main port
                                    safe_print("Firmware version parsing failed, but this is the main port", Fore.YELLOW)
                                    return port_name
                        except Exception as e:
                            if debugWokwi:
                                safe_print(f"Error decoding response from {port_name}: {e}", Fore.YELLOW)
                            
        except Exception as e:
            if debugWokwi:
                safe_print(f"Error testing {port_name}: {e}", Fore.YELLOW)
    
    return None

def query_port_function(port_name):
    """Query individual port with ENQ to get its function"""
    try:
        with serial.Serial(port_name, 115200, timeout=0.5) as port:
            # Clear buffers
            port.reset_input_buffer()
            port.reset_output_buffer()
            time.sleep(0.05)
            
            # Send ENQ (0x05)
            port.write(b"\x05\n")
            port.flush()
            time.sleep(0.2)
            
            # Read response
            response_buffer = b''
            start_time = time.time()
            
            while time.time() - start_time < 0.8:
                if port.in_waiting > 0:
                    response_buffer += port.read(port.in_waiting)
                    time.sleep(0.01)
                else:
                    time.sleep(0.01)
                    if port.in_waiting == 0:
                        break
            
            if response_buffer:
                response = response_buffer.decode('utf-8', errors='ignore').strip()
                if debugWokwi:
                    safe_print(f"Port {port_name} ENQ response: {response}", Fore.CYAN)
                
                # Parse the response to find this port's function
                lines = response.split('\n')
                for line in lines:
                    line = line.strip()
                    if line.startswith('CDC') and ':' in line:
                        try:
                            cdc_part, function_part = line.split(':', 1)
                            function_desc = function_part.strip()
                            return function_desc
                        except:
                            continue
            
    except Exception as e:
        if debugWokwi:
            safe_print(f"Error querying {port_name}: {e}", Fore.YELLOW)
    
    return None

def get_all_port_functions(main_port_name, all_jumperless_ports):
    """Get functions for all ports by querying main port first, then individual ports"""
    port_functions = {}
    
    # First, query the main port to get all CDC info
    try:
        with serial.Serial(main_port_name, 115200, timeout=1) as main_port:
            safe_print(f"Querying main port {main_port_name} for all CDC functions", Fore.CYAN)
            
            # Clear buffers
            main_port.reset_input_buffer()
            main_port.reset_output_buffer()
            time.sleep(0.1)
            
            # Send ENQ (0x05)
            main_port.write(b"\x05\n")
            main_port.flush()
            time.sleep(0.3)
            
            # Read response
            response_buffer = b''
            start_time = time.time()
            
            while time.time() - start_time < 1.0:
                if main_port.in_waiting > 0:
                    response_buffer += main_port.read(main_port.in_waiting)
                    time.sleep(0.01)
                else:
                    time.sleep(0.1)
                    if main_port.in_waiting == 0:
                        break
            
            if response_buffer:
                response = response_buffer.decode('utf-8', errors='ignore').strip()
                # Filter to only show CDC lines
                cdc_lines = [line for line in response.split('\n') if line.strip().startswith('CDC')]
                filtered_response = '\n'.join(cdc_lines)
                safe_print(f"Main port ENQ response:\n{filtered_response}", Fore.YELLOW)
                
                # Parse CDC info: "CDC0: Jumperless Main", "CDC1: JL Passthrough", etc.
                lines = response.split('\n')
                for line in lines:
                    line = line.strip()
                    if line.startswith('CDC') and ':' in line:
                        try:
                            cdc_part, function_part = line.split(':', 1)
                            cdc_num = int(cdc_part.replace('CDC', '').strip())
                            function_desc = function_part.strip()
                            port_functions[cdc_num] = function_desc
                            
                            if debugWokwi:
                                safe_print(f"  CDC{cdc_num}: {function_desc}", Fore.GREEN)
                        except (ValueError, IndexError) as e:
                            if debugWokwi:
                                safe_print(f"Error parsing line '{line}': {e}", Fore.YELLOW)
                            continue
    except Exception as e:
        safe_print(f"Error querying main port: {e}", Fore.RED)
    
    # If we didn't get all the port info, try querying each port individually
    num_detected_ports = len(all_jumperless_ports)
    if len(port_functions) < num_detected_ports:
        safe_print(f"Trying to query {num_detected_ports} individual ports for their functions", Fore.CYAN)
        
        for port_name, desc, hwid, interface, additional_attrs in all_jumperless_ports:
            if port_name != main_port_name:  # Skip main port, we already know it
                function = query_port_function(port_name)
                if function:
                    safe_print(f"Port {port_name}: {function}", Fore.GREEN)
    
    return port_functions

def organize_jumperless_ports(main_port_name, all_jumperless_ports, port_functions):
    """Organize ports by function with fallback to numerical order"""
    organized_ports = {}
    
    # First, assign ports based on ENQ query results
    for cdc_num, function_desc in port_functions.items():
        # Try to find which physical port corresponds to this CDC number
        port_assigned = False
        
        # Method 1: Try to find the port by querying it individually
        for port_name, desc, hwid, interface, additional_attrs in all_jumperless_ports:
            if port_name == main_port_name and 'main' in function_desc.lower():
                organized_ports[port_name] = function_desc
                port_assigned = True
                break
        
        if not port_assigned:
            # Method 2: Fallback to numerical order mapping
            # Sort ports by name to get consistent ordering
            sorted_ports = sorted([p[0] for p in all_jumperless_ports])
            
            if cdc_num < len(sorted_ports):
                port_name = sorted_ports[cdc_num]
                organized_ports[port_name] = function_desc
                if debugWokwi:
                    safe_print(f"Fallback: CDC{cdc_num} -> {port_name} ({function_desc})", Fore.YELLOW)
    
    # If we still don't have enough ports mapped, map remaining ports in order
    unmapped_ports = [p[0] for p in all_jumperless_ports if p[0] not in organized_ports]
    if unmapped_ports:
        safe_print(f"Mapping {len(unmapped_ports)} remaining ports in numerical order", Fore.CYAN)
        
        for i, port_name in enumerate(sorted(unmapped_ports)):
            if port_name == main_port_name:
                organized_ports[port_name] = "Jumperless Main"
            else:
                organized_ports[port_name] = f"Jumperless Port {i + 1}"
    
    return organized_ports

def choose_arduino_port(organized_ports, main_port_name):
    """Choose the best port for Arduino programming"""
    # Look for specific Arduino-friendly functions in order of preference
    arduino_preferences = [
        "passthrough", "jl passthrough", "arduino", "serial"
    ]
    
    for port_name, function_desc in organized_ports.items():
        if port_name != main_port_name:  # Don't use main port for Arduino
            function_lower = function_desc.lower()
            for pref in arduino_preferences:
                if pref in function_lower:
                    return port_name
    
    # Fallback: use any non-main port
    for port_name in organized_ports:
        if port_name != main_port_name:
            return port_name
    
    return None

def open_serial():
    """Open serial connection with simplified port detection"""
    global portName, ser, arduinoPort, serialconnected, portSelected, updateInProgress
    
    portSelected = False
    
    safe_print("\nScanning for serial ports...\n", Fore.CYAN)
    
    while not portSelected and updateInProgress == 0:
        ports = get_available_ports()
        
        if not ports:
            safe_print("No serial ports found. Please connect your Jumperless.", Fore.RED)
            time.sleep(2)
            continue
        
        # Find all Jumperless devices
        jumperless_ports = []
        other_ports = []
        
        for i, (port, desc, hwid, interface, additional_attrs) in enumerate(ports, 1):
            vid, pid = parse_hardware_id(hwid, desc)
            
            if is_jumperless_device(desc, pid, interface):
                jumperless_ports.append((port, desc, hwid, interface, additional_attrs))
                safe_print(f"{i}: {port} [{desc}]", Fore.MAGENTA)
            else:
                other_ports.append((port, desc, hwid, interface, additional_attrs))
                safe_print(f"{i}: {port} [{desc}]", Fore.BLUE)
        
        if not jumperless_ports:
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
        else:
            # Step 1: Find the main port using '?' query
            safe_print(f"\nFound {len(jumperless_ports)} Jumperless device(s). Finding main port...", Fore.CYAN)
            
            main_port_name = find_main_port(jumperless_ports)
            
            if not main_port_name:
                
                # time.sleep(0.5)
                safe_print("Trying to find main port with force_quit_python=True", Fore.YELLOW)
                main_port_name = find_main_port(jumperless_ports, force_quit_python=True)
                
                    
            
            if not main_port_name:
                safe_print("Could not find main port automatically.", Fore.YELLOW)
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
                # Step 2: Query for all port functions
                safe_print(f"Main port found: {main_port_name}", Fore.GREEN)
                
                port_functions = get_all_port_functions(main_port_name, jumperless_ports)
                
                # Step 3: Organize ports by function
                organized_ports = organize_jumperless_ports(main_port_name, jumperless_ports, port_functions)
                
                # Step 4: Set main communication port
                portName = main_port_name
                portSelected = True
                
                # Step 5: Choose Arduino port
                arduinoPort = choose_arduino_port(organized_ports, main_port_name)
                
                # Display results
                safe_print(f"\nPort assignments:", Fore.CYAN)
                safe_print(f"Main communication: {portName} ({organized_ports.get(portName, 'Unknown')})", Fore.GREEN)
                
                if arduinoPort:
                    safe_print(f"Arduino programming: {arduinoPort} ({organized_ports.get(arduinoPort, 'Unknown')})", Fore.GREEN)
                else:
                    safe_print("Arduino programming: Not available", Fore.YELLOW)
                
                if debugWokwi:
                    safe_print(f"\nAll detected ports:", Fore.CYAN)
                    for port_name, function_desc in organized_ports.items():
                        safe_print(f"  {port_name}: {function_desc}", Fore.BLUE)
    
    # Attempt to open the selected port
    try:
        if updateInProgress == 0:
            with serial_lock:
                ser = serial.Serial(portName, 115200, timeout=1)
                serialconnected = 1
            safe_print(f"\nConnected to Jumperless at {portName}", Fore.GREEN)
            return ser
    except Exception as e:
        safe_print(f"Failed to open serial port {portName}: {e}", Fore.RED)
        with serial_lock:
            ser = None
            serialconnected = 0
        return None

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
        # Extract version using robust parsing
        version_found = False
        currentString = ""
        
        if ':' in jumperlessFirmwareString:
            version_part = jumperlessFirmwareString.split(':', 1)[1].strip()
            
            # Clean version string - remove any non-digit/dot characters at the start/end
            import re
            version_clean = re.sub(r'^[^\d]*|[^\d\.]*$', '', version_part)
            # Also remove any characters after the version (like spaces, newlines, etc.)
            version_clean = re.split(r'[^\d\.]', version_clean)[0]
            
            if version_clean and '.' in version_clean:
                currentString = version_clean
                version_found = True
            else:
                # Fallback: try to find version pattern in the string
                version_match = re.search(r'(\d+\.\d+\.\d+)', jumperlessFirmwareString)
                if version_match:
                    currentString = version_match.group(1)
                    version_found = True
        
        if not version_found:
            safe_print(f"Could not parse firmware version from: {jumperlessFirmwareString}", Fore.YELLOW)
            return False
        
        current_list = currentString.split('.')
        if len(current_list) < 3:
            # Pad short version numbers with zeros
            current_list = current_list + ['0'] * (3 - len(current_list))
        
        # Pad version numbers for comparison
        for i in range(len(current_list)):
            if len(current_list[i]) < 2:
                current_list[i] = '0' + current_list[i]
        
        # Determine if this is V5
        try:
            if int(current_list[0]) >= 5:
                jumperlessV5 = True
        except ValueError:
            safe_print(f"Invalid major version number: {current_list[0]}", Fore.YELLOW)
            return False
        
        # Check latest version online
        repo_url = ("https://github.com/Architeuthis-Flux/JumperlessV5/releases/latest" 
                   if jumperlessV5 else 
                   "https://github.com/Architeuthis-Flux/Jumperless/releases/latest")
        
        response = requests.get(repo_url, timeout=10)
        version = response.url.split("/").pop()
        
        latest_list = version.split('.')
        if len(latest_list) < 3:
            # Pad short version numbers
            latest_list = latest_list + ['0'] * (3 - len(latest_list))
        
        # Pad latest version numbers for comparison
        for i in range(len(latest_list)):
            if len(latest_list[i]) < 2:
                latest_list[i] = '0' + latest_list[i]
        
        try:
            latest_int = int("".join(latest_list))
            current_int = int("".join(current_list))
        except ValueError as e:
            safe_print(f"Version comparison failed: {e}", Fore.YELLOW)
            return False
        
        latestFirmware = version
        if latest_int > current_int:
            safe_print(f"\nLatest firmware: {version}", Fore.MAGENTA)
            safe_print(f"Current version: {currentString}", Fore.RED)
            update_jumperless_firmware(force=False)
            return True
        
        return False
        
    except Exception as e:
        safe_print(f"Could not check firmware version: {e}", Fore.YELLOW)
        if debugWokwi:
            safe_print(f"Firmware string was: {repr(jumperlessFirmwareString)}", Fore.BLUE)
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
                                            if volume_name.contains("RP2350"):
                                                foundVolume = p.mountpoint
                                                safe_print(f"Found Jumperless V5 at {foundVolume}", Fore.CYAN)
                                                break
                                        else:
                                            if volume_name.contains("RPI-RP2"):
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
                                # if jumperlessV5:
                                    if p.mountpoint.endswith("RP2350") or "RP2350" in p.mountpoint:
                                        foundVolume = p.mountpoint
                                        safe_print(f"Found Jumperless V5 at {foundVolume}", Fore.RED)
                                        break
                                # else:
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
# APP UPDATE MANAGEMENT
# ============================================================================

def compare_versions(version1, version2):
    """Compare two version strings using packaging.version for robust comparison"""
    if PACKAGING_AVAILABLE:
        try:
            return version.parse(version1) < version.parse(version2)
        except Exception as e:
            safe_print(f"Error comparing versions with packaging: {e}", Fore.YELLOW)
            # Fall through to simple comparison
    
    # Fallback: simple version comparison
    try:
        # Split versions into parts and compare numerically
        v1_parts = [int(x) for x in version1.split('.')]
        v2_parts = [int(x) for x in version2.split('.')]
        
        # Pad shorter version with zeros
        max_len = max(len(v1_parts), len(v2_parts))
        v1_parts.extend([0] * (max_len - len(v1_parts)))
        v2_parts.extend([0] * (max_len - len(v2_parts)))
        
        return v1_parts < v2_parts
    except Exception as e:
        safe_print(f"Error in fallback version comparison: {e}", Fore.YELLOW)
        # Last resort: string comparison
        return version1 != version2

def get_latest_app_version():
    """Get the latest app version by downloading and reading the script file"""
    try:
        # Debug mode: read from local file
        if debug_app_update:
            safe_print(f"DEBUG MODE: Reading version from local file: {debug_test_file}", Fore.MAGENTA)
            
            if not os.path.exists(debug_test_file):
                safe_print(f"Debug test file not found: {debug_test_file}", Fore.RED)
                return None, None
            
            try:
                with open(debug_test_file, 'r', encoding='utf-8') as f:
                    lines = f.readlines()
                
                app_version = None
                for line in lines[:20]:  # Check first 20 lines
                    line = line.strip()
                    if line.startswith('App_Version') and '=' in line:
                        try:
                            # Extract version from line like: App_Version = "1.1.1.1"
                            version_part = line.split('=', 1)[1].strip()
                            
                            # Find the quoted string
                            if '"' in version_part:
                                # Extract content between first pair of double quotes
                                start_quote = version_part.find('"')
                                end_quote = version_part.find('"', start_quote + 1)
                                if end_quote > start_quote:
                                    app_version = version_part[start_quote + 1:end_quote]
                                else:
                                    app_version = version_part.strip('"\'').strip()
                            elif "'" in version_part:
                                # Extract content between first pair of single quotes
                                start_quote = version_part.find("'")
                                end_quote = version_part.find("'", start_quote + 1)
                                if end_quote > start_quote:
                                    app_version = version_part[start_quote + 1:end_quote]
                                else:
                                    app_version = version_part.strip('"\'').strip()
                            else:
                                # No quotes, take everything until comment or end of line
                                app_version = version_part.split('#')[0].strip()
                            break
                        except Exception as e:
                            safe_print(f"Error parsing version line '{line}': {e}", Fore.YELLOW)
                            continue
                
                if app_version:
                    safe_print(f"Found app version in test file: {app_version}", Fore.GREEN)
                    return app_version, "debug://local-file"
                else:
                    safe_print("Could not find App_Version in test file", Fore.YELLOW)
                    return None, None
                    
            except Exception as e:
                safe_print(f"Error reading test file: {e}", Fore.RED)
                return None, None
        
        # Production mode: download from GitHub
        # First get the latest release info
        response = requests.get(
            f"https://api.github.com/repos/{app_update_repo}/releases/latest",
            timeout=5
        )
        if response.status_code != 200:
            safe_print("Could not fetch latest release info from GitHub", Fore.YELLOW)
            return None, None
        
        release_data = response.json()
        firmware_version = release_data.get('tag_name', '').lstrip('v')
        release_url = release_data.get('html_url', '')
        
        if not firmware_version:
            safe_print("Could not determine firmware version from release", Fore.YELLOW)
            return None, None
        
        # Download the app script to read its version
        script_url = f"https://github.com/{app_update_repo}/releases/download/{firmware_version}/{app_script_name}"
        
        safe_print(f"Downloading app script to check version: {script_url}", Fore.BLUE)
        
        script_response = requests.get(script_url, timeout=10)
        if script_response.status_code != 200:
            safe_print(f"Could not download app script (HTTP {script_response.status_code})", Fore.YELLOW)
            return None, None
        
        # Read the first few lines to find App_Version
        script_content = script_response.text
        lines = script_content.split('\n')
        
        app_version = None
        for line in lines[:20]:  # Check first 20 lines
            line = line.strip()
            if line.startswith('App_Version') and '=' in line:
                try:
                    # Extract version from line like: App_Version = "1.1.1.1"
                    version_part = line.split('=', 1)[1].strip()
                    
                    # Find the quoted string
                    if '"' in version_part:
                        # Extract content between first pair of double quotes
                        start_quote = version_part.find('"')
                        end_quote = version_part.find('"', start_quote + 1)
                        if end_quote > start_quote:
                            app_version = version_part[start_quote + 1:end_quote]
                        else:
                            app_version = version_part.strip('"\'').strip()
                    elif "'" in version_part:
                        # Extract content between first pair of single quotes
                        start_quote = version_part.find("'")
                        end_quote = version_part.find("'", start_quote + 1)
                        if end_quote > start_quote:
                            app_version = version_part[start_quote + 1:end_quote]
                        else:
                            app_version = version_part.strip('"\'').strip()
                    else:
                        # No quotes, take everything until comment or end of line
                        app_version = version_part.split('#')[0].strip()
                    break
                except Exception as e:
                    if debugWokwi:
                        safe_print(f"Error parsing version line '{line}': {e}", Fore.YELLOW)
                    continue
        
        if app_version:
            safe_print(f"Found app version in script: {app_version}", Fore.GREEN)
            return app_version, release_url
        else:
            safe_print("Could not find App_Version in downloaded script", Fore.YELLOW)
            return None, None
        
    except Exception as e:
        safe_print(f"Error checking for app updates: {e}", Fore.YELLOW)
        return None, None

def check_for_app_updates():
    """Check if there's a newer version of the app available"""
    global App_Version
    
    safe_print("Checking for app updates...", Fore.CYAN)
    
    latest_version, release_url = get_latest_app_version()
    if not latest_version:
        return False
    
    try:
        if compare_versions(App_Version, latest_version):
            safe_print(f"\nNew app version available!", Fore.GREEN)
            safe_print(f"Current version: {App_Version}", Fore.YELLOW)
            safe_print(f"Latest version: {latest_version}", Fore.GREEN)
            if release_url:
                safe_print(f"Release notes: {release_url}", Fore.BLUE)
            
            return True
        else:
            safe_print(f"App is up to date (version {App_Version})", Fore.GREEN)
            return False
            
    except Exception as e:
        safe_print(f"Error during version comparison: {e}", Fore.RED)
        return False

def download_app_update():
    """Download the latest version of the app script"""
    try:
        # Debug mode: copy from local file
        if debug_app_update:
            safe_print(f"DEBUG MODE: Copying from local file: {debug_test_file}", Fore.MAGENTA)
            
            if not os.path.exists(debug_test_file):
                safe_print(f"Debug test file not found: {debug_test_file}", Fore.RED)
                return None, None
            
            # Create a temporary file and copy the test file content
            with tempfile.NamedTemporaryFile(mode='w+b', delete=False, suffix='.py') as temp_file:
                temp_script_path = temp_file.name
                
                # Copy the test file to temp location
                with open(debug_test_file, 'rb') as source_file:
                    temp_file.write(source_file.read())
            
            safe_print("Test file copied successfully", Fore.GREEN)
            
            # No requirements in debug mode
            requirements_path = None
            
            return temp_script_path, requirements_path
        
        # Production mode: download from GitHub
        # Get the latest firmware version (release tag) for download URL
        response = requests.get(
            f"https://api.github.com/repos/{app_update_repo}/releases/latest",
            timeout=3
        )
        if response.status_code != 200:
            safe_print("Could not fetch latest release info", Fore.RED)
            return None, None
        
        release_data = response.json()
        firmware_version = release_data.get('tag_name', '').lstrip('v')
        
        if not firmware_version:
            safe_print("Could not determine firmware version for download", Fore.RED)
            return None, None
        
        # Download the main script using firmware version in URL
        script_url = f"https://github.com/{app_update_repo}/releases/download/{firmware_version}/{app_script_name}"
        
        safe_print(f"Downloading {app_script_name} (release {firmware_version})...", Fore.CYAN)
        
        # Create a temporary file for download
        with tempfile.NamedTemporaryFile(mode='wb', delete=False, suffix='.py') as temp_file:
            temp_script_path = temp_file.name
            
            # Download the script
            response = requests.get(script_url, timeout=30)
            response.raise_for_status()
            
            temp_file.write(response.content)
        
        safe_print("App script downloaded successfully", Fore.GREEN)
        
        # Check if requirements.txt should be downloaded
        requirements_path = None
        if new_requirements:
            try:
                safe_print("Downloading requirements.txt...", Fore.CYAN)
                requirements_url = f"https://github.com/{app_update_repo}/releases/download/{firmware_version}/{app_requirements_name}"
                
                with tempfile.NamedTemporaryFile(mode='wb', delete=False, suffix='.txt') as temp_req_file:
                    requirements_path = temp_req_file.name
                    
                    req_response = requests.get(requirements_url, timeout=15)
                    req_response.raise_for_status()
                    
                    temp_req_file.write(req_response.content)
                
                safe_print("Requirements.txt downloaded successfully", Fore.GREEN)
                
            except Exception as e:
                safe_print(f"Could not download requirements.txt: {e}", Fore.YELLOW)
                safe_print("Continuing with app update only...", Fore.CYAN)
                requirements_path = None
        
        return temp_script_path, requirements_path
        
    except Exception as e:
        safe_print(f"Error downloading app update: {e}", Fore.RED)
        return None, None

def backup_current_app():
    """Create a backup of the current app script"""
    current_script = sys.argv[0]
    if not current_script.endswith('.py'):
        current_script = __file__
    
    try:
        # Create backup directory
        backup_dir = "JumperlessFiles/appBackups"
        pathlib.Path(backup_dir).mkdir(parents=True, exist_ok=True)
        
        # Create backup with timestamp
        timestamp = time.strftime("%Y%m%d_%H%M%S")
        script_name = os.path.basename(current_script)
        backup_name = f"{script_name}.backup_{timestamp}"
        backup_path = os.path.join(backup_dir, backup_name)
        
        shutil.copy2(current_script, backup_path)
        safe_print(f"Current version backed up as: {backup_path}", Fore.CYAN)
        return backup_path
        
    except Exception as e:
        safe_print(f"Warning: Could not create backup: {e}", Fore.YELLOW)
        return None

def cleanup_old_backups():
    """Clean up old backup files to prevent accumulation"""
    try:
        backup_dir = "JumperlessFiles/appBackups"
        
        # Check if backup directory exists
        if not os.path.exists(backup_dir):
            return
        
        backup_pattern = f"{app_script_name}.backup_"
        
        # Find all backup files
        backup_files = []
        for filename in os.listdir(backup_dir):
            if filename.startswith(backup_pattern):
                backup_path = os.path.join(backup_dir, filename)
                backup_files.append((backup_path, os.path.getmtime(backup_path)))
        
        # Sort by modification time (newest first)
        backup_files.sort(key=lambda x: x[1], reverse=True)
        
        # Keep only the 3 most recent backups
        max_backups = 3
        if len(backup_files) > max_backups:
            safe_print(f"Cleaning up old backups (keeping {max_backups} most recent)...", Fore.CYAN)
            
            for backup_path, _ in backup_files[max_backups:]:
                try:
                    os.remove(backup_path)
                    safe_print(f"Removed old backup: {os.path.basename(backup_path)}", Fore.BLUE)
                except Exception as e:
                    safe_print(f"Could not remove {backup_path}: {e}", Fore.YELLOW)
                    
    except Exception as e:
        safe_print(f"Error during backup cleanup: {e}", Fore.YELLOW)

def install_requirements(requirements_path):
    """Install requirements from downloaded requirements.txt"""
    try:
        safe_print("Installing new requirements...", Fore.CYAN)
        
        # Run pip install
        cmd = [sys.executable, '-m', 'pip', 'install', '-r', requirements_path]
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        
        if result.returncode == 0:
            safe_print("Requirements installed successfully", Fore.GREEN)
            return True
        else:
            safe_print(f"Failed to install requirements:", Fore.RED)
            safe_print(result.stderr, Fore.RED)
            return False
            
    except Exception as e:
        safe_print(f"Error installing requirements: {e}", Fore.RED)
        return False

def perform_app_update():
    """Perform the complete app update process"""
    global updateInProgress, serialconnected, ser
    
    updateInProgress = 1
    
    try:
        # Close serial connection during update
        if ser and serialconnected:
            with serial_lock:
                try:
                    ser.close()
                except:
                    pass
                ser = None
                serialconnected = 0
        
        # Clean up old backups first
        cleanup_old_backups()
        
        # Create backup of current version
        backup_path = backup_current_app()
        
        # Download new version
        new_script_path, requirements_path = download_app_update()
        if not new_script_path:
            safe_print("Update download failed", Fore.RED)
            return False
        
        # Install requirements if needed
        if requirements_path:
            requirements_success = install_requirements(requirements_path)
            if not requirements_success:
                safe_print("Requirements installation failed, but continuing with app update...", Fore.YELLOW)
        
        # Get current script path
        current_script = sys.argv[0]
        if not current_script.endswith('.py'):
            current_script = __file__
        
        # Replace current script with new version
        safe_print("Installing new app version...", Fore.CYAN)
        
        try:
            # On Windows, we might need to handle file locking differently
            if sys.platform == "win32":
                # Create a batch script to handle the replacement
                batch_script = f"""
@echo off
timeout /t 2 /nobreak > nul
move "{new_script_path}" "{current_script}"
echo App update completed!
pause
"""
                batch_path = "update_app.bat"
                with open(batch_path, 'w') as f:
                    f.write(batch_script)
                
                safe_print("App update prepared. Please run update_app.bat after this script exits.", Fore.YELLOW)
                safe_print("The app will restart automatically after update.", Fore.CYAN)
                
            else:
                # Unix-like systems: direct replacement
                shutil.move(new_script_path, current_script)
                safe_print("App updated successfully!", Fore.GREEN)
        
        except Exception as e:
            safe_print(f"Error replacing app file: {e}", Fore.RED)
            # Try to restore backup
            if backup_path and os.path.exists(backup_path):
                try:
                    shutil.copy2(backup_path, current_script)
                    safe_print("Restored backup version", Fore.YELLOW)
                except Exception as restore_error:
                    safe_print(f"Could not restore backup: {restore_error}", Fore.RED)
            return False
        
        # Clean up temporary files
        try:
            if os.path.exists(new_script_path):
                os.remove(new_script_path)
            if requirements_path and os.path.exists(requirements_path):
                os.remove(requirements_path)
        except Exception as cleanup_error:
            safe_print(f"Note: Could not clean up temporary files: {cleanup_error}", Fore.YELLOW)
        
        # safe_print("\n" + "="*60, Fore.GREEN)
        safe_print("APP UPDATE COMPLETED SUCCESSFULLY!", Fore.GREEN)
        # safe_print("="*60, Fore.GREEN)
        safe_print("Please restart the application to use the new version.", Fore.CYAN)
        # safe_print("Your backup is saved in case you need to revert.", Fore.BLUE)
        
        return True
        
    except Exception as e:
        safe_print(f"Unexpected error during app update: {e}", Fore.RED)
        return False
    finally:
        updateInProgress = 0

def update_app_if_needed():
    """Check for and optionally perform app update"""
    try:
        if check_for_app_updates():
            safe_print("\nWould you like to update the app now?", Fore.CYAN)
            
            # Use simple input() instead of timeout input for better reliability
            try:
                user_response = input("Update now? (y/N): ").strip().lower()
            except (EOFError, KeyboardInterrupt):
                user_response = "n"
            
            if debugWokwi:
                safe_print(f"User response: '{user_response}'", Fore.BLUE)
            
            if user_response in ['y', 'yes']:
                safe_print("Starting app update...", Fore.GREEN)
                success = perform_app_update()
                
                if success:
                    # # Ask user if they want to restart immediately
                    # try:
                    #     restart_response = input("Restart the app now? (y/N): ").strip().lower()
                    # except (EOFError, KeyboardInterrupt):
                    #     restart_response = "n"
                    
                    # if restart_response in ['y', 'yes']:
                    safe_print("Restarting app...", Fore.CYAN)
                    # Restart the app
                    try:
                        cleanup_on_exit()
                        os.execv(sys.executable, [sys.executable] + sys.argv)
                    except Exception as restart_error:
                        safe_print(f"Could not restart automatically: {restart_error}", Fore.YELLOW)
                        safe_print("Please restart the app manually.", Fore.CYAN)
                        sys.exit(0)
                else:
                    safe_print("App update failed. Continuing with current version.", Fore.YELLOW)
            else:
                safe_print("App update skipped.", Fore.BLUE)
                
    except Exception as e:
        safe_print(f"Error during app update check: {e}", Fore.RED)

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
# FLASH COMMAND FUNCTIONS
# ============================================================================

def handle_flash_command():
    """Handle the flash command - flash Arduino with assigned slot content"""
    global slotURLs, slotFilePaths, numAssignedSlots, noArduinocli, disableArduinoFlashing, forceArduinoFlash
    
    if noArduinocli:
        safe_print("Arduino CLI not available", Fore.RED)
        safe_print("Install pyduinocli with: pip install pyduinocli", Fore.CYAN)
        return
    
    # Set force flag to bypass "sketch unchanged" checks and disableArduinoFlashing
    forceArduinoFlash = 1
    
    if disableArduinoFlashing:
        safe_print("Arduino flashing is disabled, but 'flash' command overrides this setting", Fore.YELLOW)
    
    # Count and find assigned slots
    assigned_slots = []
    for i in range(8):
        if slotURLs[i] != '!' or slotFilePaths[i] != '!':
            assigned_slots.append(i)
    
    if not assigned_slots:
        safe_print("No slots assigned. Use 'slots' command to assign projects first.", Fore.YELLOW)
        forceArduinoFlash = 0  # Reset flag
        return
    
    # If only one slot assigned, flash it directly
    if len(assigned_slots) == 1:
        slot_to_flash = assigned_slots[0]
        safe_print(f"Flashing slot {slot_to_flash}...", Fore.CYAN)
        
        # Check if it's a local file or Wokwi URL
        if slotFilePaths[slot_to_flash] != '!':
            # Local .ino file
            file_path = slotFilePaths[slot_to_flash]
            if is_valid_ino_file(file_path):
                content = read_local_ino_file(file_path)
                if content:
                    safe_print(f"Flashing local file: {os.path.basename(file_path)}", Fore.GREEN)
                    flash_thread = flash_arduino_sketch_threaded(content, "", slot_to_flash)
                    return flash_thread is not None
                else:
                    safe_print(f"Could not read file: {file_path}", Fore.RED)
            else:
                safe_print(f"Invalid or missing file: {file_path}", Fore.RED)
        elif slotURLs[slot_to_flash] != '!':
            # Wokwi URL
            wokwi_url = slotURLs[slot_to_flash]
            safe_print(f"Flashing Wokwi project: {wokwi_url}", Fore.GREEN)
            return process_wokwi_sketch_and_flash(wokwi_url, slot_to_flash)
    else:
        # Multiple slots assigned, ask which one to flash
        safe_print(f"Multiple slots assigned ({len(assigned_slots)}). Choose which to flash:", Fore.CYAN)
        safe_print("")
        
        for slot_num in assigned_slots:
            if slotFilePaths[slot_num] != '!':
                file_path = slotFilePaths[slot_num]
                file_name = os.path.basename(file_path)
                file_status = "(valid)" if is_valid_ino_file(file_path) else "(missing)"
                safe_print(f"  {slot_num}: [LOCAL] {file_name} {file_status}", 
                          Fore.GREEN if is_valid_ino_file(file_path) else Fore.RED)
            elif slotURLs[slot_num] != '!':
                safe_print(f"  {slot_num}: [WOKWI] {slotURLs[slot_num]}", Fore.CYAN)
        
        try:
            choice = input(f"\nEnter slot number to flash (0-7): ").strip()
            
            if not choice.isdigit():
                safe_print("Invalid input. Please enter a number.", Fore.RED)
                return False
            
            slot_choice = int(choice)
            
            if slot_choice not in assigned_slots:
                safe_print(f"Slot {slot_choice} is not assigned. Available slots: {assigned_slots}", Fore.RED)
                return False
            
            # Flash the chosen slot
            safe_print(f"Flashing slot {slot_choice}...", Fore.CYAN)
            
            if slotFilePaths[slot_choice] != '!':
                # Local .ino file
                file_path = slotFilePaths[slot_choice]
                if is_valid_ino_file(file_path):
                    content = read_local_ino_file(file_path)
                    if content:
                        safe_print(f"Flashing local file: {os.path.basename(file_path)}", Fore.GREEN)
                        flash_thread = flash_arduino_sketch_threaded(content, "", slot_choice)
                        return flash_thread is not None
                    else:
                        safe_print(f"Could not read file: {file_path}", Fore.RED)
                else:
                    safe_print(f"Invalid or missing file: {file_path}", Fore.RED)
            elif slotURLs[slot_choice] != '!':
                # Wokwi URL
                wokwi_url = slotURLs[slot_choice]
                safe_print(f"Flashing Wokwi project: {wokwi_url}", Fore.GREEN)
                return process_wokwi_sketch_and_flash(wokwi_url, slot_choice)
            
        except (ValueError, KeyboardInterrupt):
            safe_print("Flash cancelled.", Fore.YELLOW)
            forceArduinoFlash = 0  # Reset flag on cancellation
            return False
    
    # Safety reset in case flag wasn't reset elsewhere
    forceArduinoFlash = 0
    return False

def upload_with_attempts_limit(sketch_dir, arduino_port, fqbn, build_dir, discovery_timeout="2s"):
    """Custom upload function that calls arduino-cli directly with attempts limit"""
    global ser, arduinoPort
    notInSyncString = "avrdude: stk500_recv(): programmer is not responding"
    arduino_serial = None
    
    try:
        import subprocess
        # Get the arduino-cli path
        cli_path = None
        cli_paths = [
            resource_path("arduino-cli.exe" if sys.platform == "win32" else "arduino-cli"),
            "./arduino-cli.exe" if sys.platform == "win32" else "./arduino-cli",
            "arduino-cli"  # System PATH
        ]
        
        for path in cli_paths:
            if os.path.isfile(path) or (path == "arduino-cli" and shutil.which(path)):
                cli_path = path
                break
        
        if not cli_path:
            raise Exception("arduino-cli not found")
        
        # Verify FQBN is valid by listing available boards
        try:
            list_cmd = [cli_path, "board", "listall", "--format", "text"]
            list_result = subprocess.run(list_cmd, capture_output=True, text=True, timeout=10)
            if list_result.returncode == 0:
                available_boards = list_result.stdout
                if "arduino:avr:nano" not in available_boards:
                    safe_print("Warning: arduino:avr:nano not found in available boards", Fore.YELLOW)
                    # Look for alternative nano boards
                    nano_boards = [line for line in available_boards.split('\n') if 'nano' in line.lower()]
                    if nano_boards:
                        safe_print("Available Nano boards:", Fore.CYAN)
                        for board in nano_boards[:3]:
                            safe_print(f"  {board}", Fore.CYAN)
                else:
                    safe_print("Verified: arduino:avr:nano is available", Fore.GREEN)
            else:
                safe_print(f"Could not verify FQBN (command failed: {list_result.stderr})", Fore.YELLOW)
        except Exception as verify_error:
            safe_print(f"Could not verify FQBN: {verify_error}", Fore.YELLOW)
        
        # Force clear and verify Arduino port before upload
        arduino_serial = None
        
        # Step 1: Ensure Arduino port is accessible
        safe_print("Verifying Arduino port accessibility...", Fore.YELLOW)
        
        # Step 2: Quick port accessibility check
        for attempt in range(3):
            try:
                # Test if the port is available
                arduino_serial = serial.Serial(arduino_port, 115200, timeout=0.1)
                arduino_serial.close()  # Close it immediately after testing
                arduino_serial = None
                if debugWokwi:
                    safe_print(f"Arduino port {arduino_port} verified available (attempt {attempt + 1})", Fore.CYAN)
                break
            except Exception as port_error:
                if arduino_serial:
                    try:
                        arduino_serial.close()
                    except:
                        pass
                    arduino_serial = None
                
                if attempt < 2:  # Not the last attempt
                    if debugWokwi:
                        safe_print(f"Arduino port busy, waiting... (attempt {attempt + 1})", Fore.YELLOW)
                    time.sleep(0.5)  # Wait between attempts
                else:
                    # Final attempt - just warn but continue
                    safe_print(f"Warning: Arduino port may be busy: {port_error}", Fore.YELLOW)
                    safe_print("Attempting upload anyway...", Fore.CYAN)
        
        # Step 3: Brief stabilization delay
        time.sleep(0.1)
        # Build the command using the configurable system
        # Note: Arduino CLI will automatically find compiled files in the sketch directory
        cmd = build_arduino_cli_command(
            cli_path=cli_path,
            arduino_port=arduino_port,
            fqbn=fqbn,
            build_path=build_dir,
            discovery_timeout=discovery_timeout,
            sketch_dir=sketch_dir
        )
        
        if debugWokwi:
            safe_print(f"Generated upload command: {' '.join(cmd)}", Fore.MAGENTA)
        
        # Always show the Arduino CLI command being executed
        safe_print(f"Arduino CLI command: {' '.join(cmd)}", Fore.BLUE)
        
        # Run the command with real-time output streaming
        process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, 
                                 text=True, bufsize=1)
        
        # Track this process for cleanup on exit
        global active_processes
        active_processes.append(process)
        
        output_lines = []
        start_time = time.time()
        timeout = 45  # 45 second timeout
        flash_failed = False
        retries = 0
        
        # Stream output in real-time
        while True:
            # Check for timeout
            if time.time() - start_time > timeout:
                safe_print("Upload timed out, terminating process...", Fore.YELLOW)
                # Try to clean up on timeout
                try:
                    if arduino_serial and arduino_serial.is_open:
                        arduino_serial.close()
                except:
                    pass
                process.terminate()
                try:
                    process.wait(timeout=3)  # Give it 2 seconds to terminate gracefully
                except subprocess.TimeoutExpired:
                    safe_print("Process didn't terminate gracefully, force killing...", Fore.RED)
                    process.kill()  # Force kill if it doesn't terminate
                    try:
                        process.wait(timeout=1)  # Wait for kill to complete
                    except subprocess.TimeoutExpired:
                        pass  # Process is really stuck, move on
                raise Exception(f"Upload timed out after {timeout} seconds")
            
            output = process.stdout.readline()

            if output == '' and process.poll() is not None:
                break
            if (notInSyncString in output):
                # Arduino flash error detected - handle sync errors
                safe_print(f"Arduino sync error detected (attempt {retries + 1}): {output.strip()}", Fore.YELLOW)
                retries += 1
                
                # Try to send reset command to Arduino
                # try:
                #     if ser and ser.is_open:
                #         ser.write(b"r")  # Send reset command
                #         time.sleep(0.5)  # CRITICAL: Wait for reset to take effect
                # except Exception as e:
                #     safe_print(f"Error sending reset command: {e}", Fore.RED)
                
                if retries > 2:
                    safe_print("Too many sync errors, terminating upload", Fore.RED)
                    
                    # Gracefully terminate the process
                    try:
                        if process.poll() is None:  # Only terminate if process is still running
                            process.terminate()
                            process.wait(timeout=2)  # Give it 2 seconds to terminate gracefully
                    except subprocess.TimeoutExpired:
                        safe_print("Process didn't terminate gracefully, force killing...", Fore.RED)
                        try:
                            process.kill()
                            process.wait(timeout=1)  # Wait for kill to complete
                        except subprocess.TimeoutExpired:
                            safe_print("Process is unresponsive, continuing anyway...", Fore.RED)
                    except Exception as term_error:
                        safe_print(f"Error terminating process: {term_error}", Fore.RED)
                    
                    raise Exception(f"Upload failed after {retries} sync error attempts")
            elif output:
                output_lines.append(output.strip())
                # Print output in real-time with a prefix to distinguish it
                safe_print(f"{output.strip()}", Fore.CYAN)
            else:
                # No output, sleep briefly to prevent busy waiting
                time.sleep(0.1)
        
        # Wait for process to complete and get return code
        return_code = process.poll()
        
        # If we detected a flash failure, ensure we have a non-zero return code
        if flash_failed and return_code == 0:
            return_code = 1
        
        if return_code != 0:
            full_output = '\n'.join(output_lines)
            raise Exception(f"Upload failed with return code {return_code}: {full_output}")
        
        returnValue = {"success": True, "stdout": '\n'.join(output_lines), "stderr": ""}
        # safe_print(returnValue, Fore.GREEN)
        try:
            if process.poll() is None:  # Only terminate if process is still running
                process.terminate()
                process.wait(timeout=2)  # Give it 2 seconds to terminate gracefully
        except subprocess.TimeoutExpired:
            safe_print("Process didn't terminate gracefully, force killing...", Fore.RED)
            try:
                process.kill()
                process.wait(timeout=1)  # Wait for kill to complete
            except subprocess.TimeoutExpired:
                # safe_print("Process is unresponsive, continuing anyway...", Fore.RED)
                pass
        except Exception as term_error:
            safe_print(f"Error terminating process: {term_error}", Fore.RED)
        
        return returnValue
        
    except Exception as e:
        raise Exception(f"Custom upload failed: {e}")
    finally:
        # Remove process from tracking
        try:
            if process in active_processes:
                active_processes.remove(process)
        except:
            pass
        
        # Ensure the Arduino port is closed in all cases (success, error, or timeout)
        if arduino_serial is not None:
            try:
                if arduino_serial.is_open:
                    arduino_serial.close()
                    safe_print(f"Closed Arduino port {arduino_port} after upload", Fore.YELLOW)
            except Exception as close_error:
                safe_print(f"Note: Error closing Arduino port: {close_error}", Fore.YELLOW)
        
        # Give a small delay to ensure port is fully released
        time.sleep(0.1)
        
        
# ============================================================================
# MENU SYSTEM
# ============================================================================

def bridge_menu():
    """Main bridge menu"""
    global menuEntered, wokwiUpdateRate, numAssignedSlots, currentString, noWokwiStuff, disableArduinoFlashing, noArduinocli, arduinoPort, debugWokwi, interactive_mode, debug_app_update

    safe_print("\n\n         Jumperless App Menu\n", Fore.MAGENTA)
    
    safe_print(" 'menu'        to open the app menu (this menu)", Fore.BLUE)  
    safe_print(" 'interactive' to " + ("disable" if interactive_mode else "enable") + " real-time character mode - " + ("ON" if interactive_mode else "OFF")  , Fore.RED if interactive_mode else Fore.GREEN)
    safe_print(" 'wokwi'       to " + ("enable" if noWokwiStuff else "disable") + " Wokwi updates " + ("and just use as a terminal" if not noWokwiStuff else ""), Fore.CYAN)
    safe_print(" 'rate'        to change the Wokwi update rate", Fore.GREEN)
    safe_print(" 'slots'       to assign Wokwi projects to slots - " + str(numAssignedSlots) + " assigned", Fore.YELLOW)
    safe_print(" 'flash'       to flash Arduino with assigned slot content (works outside menu too)", Fore.MAGENTA)
    safe_print(" 'arduino'     to " + ("enable" if disableArduinoFlashing else "disable") + " Arduino flashing from wokwi", Fore.RED)
    safe_print(" 'debug'       to " + ("disable" if debugWokwi else "enable") + " Wokwi debug output - " + ("on" if debugWokwi else "off"), Fore.MAGENTA)
    safe_print(" 'config'      to edit Arduino CLI upload configuration", Fore.YELLOW)
    safe_print(" 'update'      to force firmware update - yours is up to date (" + currentString + ")", Fore.BLUE)
    debug_status = " [DEBUG MODE]" if debug_app_update else ""
    safe_print(" 'appupdate'   to check for app updates - current version " + App_Version + debug_status, Fore.MAGENTA)
    safe_print(" 'debugupdate' to " + ("disable" if debug_app_update else "enable") + " app update debug mode", Fore.BLUE)
    safe_print(" 'status'      to check the serial connection status", Fore.CYAN) 
    safe_print(" [enter]       to exit the menu and return to Jumperless", Fore.GREEN)
    
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
            elif choice == 'interactive':
                if interactive_mode:
                    disable_interactive_mode()
                else:
                    if enable_interactive_mode():
                        # Exit menu when entering interactive mode
                        menuEntered = 0
                        ser.write(b'm')
                        return
                continue
            elif choice == 'debug':
                debugWokwi = not debugWokwi
                safe_print(f"Wokwi debug output {'enabled' if debugWokwi else 'disabled'}", Fore.CYAN)
                continue
            elif choice == 'config':
                edit_arduino_cli_config()
                continue
            elif choice == 'update':
                update_jumperless_firmware(force=True)
                menuEntered = 0
                ser.write(b'm')
                return
            elif choice == 'appupdate':
                safe_print("Checking for app updates...", Fore.CYAN)
                update_app_if_needed()
                continue
            elif choice == 'debugupdate':
                debug_app_update = not debug_app_update
                mode_text = "enabled" if debug_app_update else "disabled"
                safe_print(f"App update debug mode {mode_text}", Fore.CYAN)
                if debug_app_update:
                    safe_print(f"Will use local test file: {debug_test_file}", Fore.BLUE)
                else:
                    safe_print("Will use GitHub releases for updates", Fore.BLUE)
                continue
            elif choice == 'slots':
                changes_made, return_to_menu = assign_wokwi_slots()
                if return_to_menu:
                    continue
                else:
                    menuEntered = 0
                    ser.write(b'm')
                    return
            elif choice == 'flash':
                handle_flash_command()
                continue
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

def get_arduino_cli_config():
    """Get arduino-cli command configuration from file, create default if not exists"""
    config_file = "JumperlessFiles/arduino_cli_config.txt"
    
    # Default configuration
    default_config = [
        "# Arduino CLI Upload Configuration",
        "# Edit this file to customize arduino-cli upload options",
        "# One option per line, comments start with #",
        "# Available placeholders: {cli_path}, {arduino_port}, {fqbn}, {build_path}, {discovery_timeout}, {sketch_dir}",
        "# IMPORTANT: Arduino CLI requires NO SPACES between option flags and values",
        "",
        "# Full upload command with common options",
        "upload",
        "-p{arduino_port}",
        "-b{fqbn}",
        "-v",
        "# --verify",
        "# --discovery-timeout {discovery_timeout}",
        "",
        "# Upload field options (uncomment to use):",
        "# --upload-field timeout=10",
        "# --upload-field attempts=1", 
        "# --upload-field wait_for_upload_port=true",
        "",
        "# Build path options (uncomment ONE if needed):",
        "# --build-path{build_path}   # Specify build directory",
        "# --input-dir{build_path}    # Alternative build directory flag", 
        "",
        "# Custom avrdude config (uncomment and adjust path):",
        "# --upload-field avrdude.config.file=./avrdudeCustom.conf",
        "",
        "# The sketch directory must be last (Arduino CLI auto-detects compiled files)",
        "{sketch_dir}"
    ]
    
    try:
        # Create directory if it doesn't exist
        pathlib.Path(config_file).parent.mkdir(parents=True, exist_ok=True)
        
        # Check if config file exists
        if not os.path.exists(config_file):
            # Create default config file
            with open(config_file, 'w') as f:
                f.write('\n'.join(default_config))
            safe_print(f"Created default Arduino CLI config: {config_file}", Fore.CYAN)
            safe_print("You can edit this file to customize arduino-cli upload options", Fore.BLUE)
        
        # Read config file
        with open(config_file, 'r') as f:
            lines = f.readlines()
        
        # Parse config (filter out comments and empty lines)
        config_args = []
        for line in lines:
            line = line.strip()
            if line and not line.startswith('#'):
                config_args.append(line)
        
        return config_args
        
    except Exception as e:
        safe_print(f"Error reading Arduino CLI config, using defaults: {e}", Fore.YELLOW)
        # Return default config without comments
        return [arg for arg in default_config if arg and not arg.startswith('#')]

def build_arduino_cli_command(cli_path, arduino_port, fqbn, build_path, discovery_timeout, sketch_dir):
    """Build arduino-cli command from configuration file"""
    config_args = get_arduino_cli_config()
    
    # Create substitution dictionary
    substitutions = {
        'cli_path': cli_path,
        'arduino_port': arduino_port,
        'fqbn': fqbn,
        'build_path': build_path or '',  # Empty if None
        'input_dir': build_path or '',   # Legacy support
        'discovery_timeout': discovery_timeout,
        'sketch_dir': sketch_dir
    }
    
    # Build command
    cmd = [cli_path]  # Start with the CLI path
    
    for arg in config_args:
        try:
            # Substitute placeholders
            formatted_arg = arg.format(**substitutions)
            # Skip empty arguments and flags with empty values (can happen when build_path is None/empty)
            stripped_arg = formatted_arg.strip()
            if stripped_arg and not any(stripped_arg.endswith(' ') and flag in stripped_arg 
                                      for flag in ['--build-path', '--input-dir']):
                cmd.append(formatted_arg)
        except KeyError as e:
            safe_print(f"Warning: Unknown placeholder in Arduino CLI config: {e}", Fore.YELLOW)
            if arg.strip():  # Only add non-empty args
                cmd.append(arg)  # Use as-is if substitution fails
        except Exception as e:
            safe_print(f"Warning: Error formatting Arduino CLI argument '{arg}': {e}", Fore.YELLOW)
            if arg.strip():  # Only add non-empty args
                cmd.append(arg)  # Use as-is if formatting fails
    
    return cmd

def command_to_inline_format(config_args):
    """Convert config args with placeholders to inline command format for editing"""
    # Use sample values for display/editing
    sample_substitutions = {
        'cli_path': 'arduino-cli',
        'arduino_port': '/dev/ttyUSB0',
        'fqbn': 'arduino:avr:nano',
        'build_path': './build',
        'input_dir': './build',  # Legacy support
        'discovery_timeout': ' 2s',
        'sketch_dir': './sketch'
    }
    
    cmd_parts = []
    for arg in config_args:
        try:
            formatted_arg = arg.format(**sample_substitutions)
            cmd_parts.append(formatted_arg)
        except:
            cmd_parts.append(arg)
    
    return ' '.join(cmd_parts)

def inline_format_to_config(command_string):
    """Convert inline command format back to config args with placeholders"""
    # Split command into parts
    parts = command_string.strip().split()
    
    if not parts:
        return []
    
    # Remove the cli_path (first part) since that's handled separately
    if parts[0] in ['arduino-cli', './arduino-cli', './arduino-cli.exe', 'arduino-cli.exe']:
        parts = parts[1:]
    
    config_args = []
    i = 0
    while i < len(parts):
        part = parts[i]
        
        # Handle arguments that take values
        if part in ['-p', '--port']:
            config_args.append('-p{arduino_port}')
            i += 2  # Skip the value
        elif part in ['-b', '--fqbn']:
            config_args.append('-b{fqbn}')
            i += 2  # Skip the value
        elif part == '--build-path':
            config_args.append('--build-path{build_path}')
            i += 2  # Skip the value
        elif part == '--input-dir':
            config_args.append('--build-path{build_path}')  # Convert legacy to new format
            i += 2  # Skip the value
        elif part == '--discovery-timeout':
            config_args.append('--discovery-timeout{discovery_timeout}')
            i += 2  # Skip the value
        elif part.startswith('./') and (part.endswith('sketch') or 'sketch' in part):
            # This is likely the sketch directory - put it at the end
            config_args.append('{sketch_dir}')
            i += 1
        elif part.startswith('/') or part.startswith('./') or part.startswith('sketch'):
            # Other paths might be sketch directory
            config_args.append('{sketch_dir}')
            i += 1
        else:
            # Single argument without value
            config_args.append(part)
            i += 1
    
    return config_args

def edit_arduino_cli_config():
    """Interactive editor for Arduino CLI configuration"""
    config_file = "JumperlessFiles/arduino_cli_config.txt"
    
    # Ensure config file exists
    get_arduino_cli_config()  # This will create the file if it doesn't exist
    
    try:
        # Read current config
        with open(config_file, 'r') as f:
            lines = f.readlines()
        
        safe_print(f"\nCurrent Arduino CLI Configuration ({config_file}):", Fore.CYAN)
        safe_print("=" * 60, Fore.BLUE)
        
        # Display current config with line numbers
        for i, line in enumerate(lines, 1):
            line_color = Fore.GREEN if not line.strip().startswith('#') and line.strip() else Fore.YELLOW
            safe_print(f"{i:2d}: {line.rstrip()}", line_color)
        
        safe_print("=" * 60, Fore.BLUE)
        safe_print("\nOptions:", Fore.CYAN)
        safe_print("  'edit' - Open file in system editor", Fore.GREEN)
        safe_print("  'inline' - Edit command directly (simple format)", Fore.MAGENTA)
        safe_print("  'view' - View current configuration again", Fore.BLUE)
        safe_print("  'reset' - Reset to default configuration", Fore.YELLOW)
        safe_print("  'test' - Test current configuration", Fore.MAGENTA)
        safe_print("  'enter' - Return to menu", Fore.WHITE)
        
        while True:
            choice = input("\nconfig> ").strip().lower()
            
            if choice == '' or choice == 'exit' or choice == 'menu':
                break
            elif choice == 'edit':
                # Try to open in system editor
                try:
                    import subprocess
                    if sys.platform == "win32":
                        subprocess.run(['notepad', config_file])
                    elif sys.platform == "darwin":  # macOS
                        subprocess.run(['open', '-t', config_file])
                    else:  # Linux and others
                        # Try common editors
                        editors = ['nano', 'vim', 'vi', 'gedit']
                        editor_found = False
                        for editor in editors:
                            try:
                                subprocess.run([editor, config_file], check=True)
                                editor_found = True
                                break
                            except (subprocess.CalledProcessError, FileNotFoundError):
                                continue
                        
                        if not editor_found:
                            safe_print("No editor found. Please manually edit: " + config_file, Fore.YELLOW)
                    
                    safe_print("File opened for editing. Press Enter when done...", Fore.GREEN)
                    input()
                    
                    # Re-read and display updated config
                    with open(config_file, 'r') as f:
                        updated_lines = f.readlines()
                    
                    safe_print("\nUpdated configuration:", Fore.GREEN)
                    for i, line in enumerate(updated_lines, 1):
                        line_color = Fore.GREEN if not line.strip().startswith('#') and line.strip() else Fore.YELLOW
                        safe_print(f"{i:2d}: {line.rstrip()}", line_color)
                    
                except Exception as e:
                    safe_print(f"Could not open editor: {e}", Fore.RED)
                    safe_print(f"Please manually edit: {config_file}", Fore.YELLOW)
                
            elif choice == 'inline':
                # Edit the command in simple inline format
                try:
                    # Get current config args (non-comment lines)
                    current_config = get_arduino_cli_config()
                    
                    # Convert to inline format for editing
                    current_command = command_to_inline_format(current_config)
                    
                    safe_print("\nCurrent command:", Fore.CYAN)
                    safe_print(current_command, Fore.GREEN)
                    safe_print("\nEdit the command below (or press Enter to cancel):", Fore.YELLOW)
                    
                    # Get user input
                    new_command = input("arduino-cli> ").strip()
                    
                    if not new_command:
                        safe_print("Edit cancelled", Fore.BLUE)
                        continue
                    
                    # Add arduino-cli prefix if not present
                    if not new_command.startswith('arduino-cli'):
                        new_command = 'arduino-cli ' + new_command
                    
                    # Convert back to config format
                    new_config_args = inline_format_to_config(new_command)
                    
                    if not new_config_args:
                        safe_print("Invalid command format", Fore.RED)
                        continue
                    
                    # Create the new config file content
                    new_config_content = [
                        "# Arduino CLI Upload Configuration",
                        "# Auto-generated from inline edit",
                        "# Edit this file to customize arduino-cli upload options",
                        "# Available placeholders: {cli_path}, {arduino_port}, {fqbn}, {build_path}, {discovery_timeout}, {sketch_dir}",
                        "",
                    ]
                    
                    # Add the config args
                    for arg in new_config_args:
                        new_config_content.append(arg)
                    
                    # Save to file
                    with open(config_file, 'w') as f:
                        f.write('\n'.join(new_config_content))
                    
                    safe_print("\nConfiguration updated!", Fore.GREEN)
                    
                    # Show the new command
                    test_cmd = build_arduino_cli_command(
                        cli_path="arduino-cli",
                        arduino_port="/dev/ttyUSB0",
                        fqbn="arduino:avr:nano",
                        build_path="./build",
                        discovery_timeout=" 2s",
                        sketch_dir="./sketch"
                    )
                    
                    safe_print("New command:", Fore.CYAN)
                    safe_print(" ".join(test_cmd), Fore.GREEN)
                    
                except Exception as e:
                    safe_print(f"Error in inline edit: {e}", Fore.RED)
                
            elif choice == 'view':
                # Re-read and display current config
                with open(config_file, 'r') as f:
                    current_lines = f.readlines()
                
                safe_print(f"\nCurrent Arduino CLI Configuration:", Fore.CYAN)
                safe_print("=" * 60, Fore.BLUE)
                for i, line in enumerate(current_lines, 1):
                    line_color = Fore.GREEN if not line.strip().startswith('#') and line.strip() else Fore.YELLOW
                    safe_print(f"{i:2d}: {line.rstrip()}", line_color)
                safe_print("=" * 60, Fore.BLUE)
                
            elif choice == 'reset':
                confirm = input("Reset to default configuration? (y/N): ").strip().lower()
                if confirm in ['y', 'yes']:
                    # Remove the file so it gets recreated with defaults
                    try:
                        os.remove(config_file)
                        get_arduino_cli_config()  # Recreate with defaults
                        safe_print("Configuration reset to defaults", Fore.GREEN)
                        
                        # Display new config
                        with open(config_file, 'r') as f:
                            reset_lines = f.readlines()
                        
                        safe_print("\nReset configuration:", Fore.GREEN)
                        for i, line in enumerate(reset_lines, 1):
                            line_color = Fore.GREEN if not line.strip().startswith('#') and line.strip() else Fore.YELLOW
                            safe_print(f"{i:2d}: {line.rstrip()}", line_color)
                    except Exception as e:
                        safe_print(f"Error resetting configuration: {e}", Fore.RED)
                else:
                    safe_print("Reset cancelled", Fore.BLUE)
                    
            elif choice == 'test':
                # Test the current configuration by building a sample command
                try:
                    test_cmd = build_arduino_cli_command(
                        cli_path="arduino-cli",
                        arduino_port="/dev/ttyUSB0",
                        fqbn="arduino:avr:nano",
                        build_path="./build",
                        discovery_timeout=" 2s",
                        sketch_dir="./sketch"
                    )
                    
                    safe_print("\nTest command that would be executed:", Fore.CYAN)
                    safe_print(" ".join(test_cmd), Fore.GREEN)
                    
                except Exception as e:
                    safe_print(f"Configuration test failed: {e}", Fore.RED)
                    safe_print("Please check your configuration for errors", Fore.YELLOW)
            else:
                safe_print("Invalid option. Use 'edit', 'inline', 'view', 'reset', 'test', or press Enter to return", Fore.RED)
    
    except Exception as e:
        safe_print(f"Error accessing Arduino CLI configuration: {e}", Fore.RED)

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

def check_port_usage(port_name):
    """Check what processes are using the Arduino port"""
    try:
        import subprocess
        import sys
        
        if sys.platform == "darwin":  # macOS
            result = subprocess.run(['lsof', port_name], capture_output=True, text=True, timeout=5)
            if result.returncode == 0 and result.stdout.strip():
                safe_print(f"Processes using {port_name}:", Fore.YELLOW)
                for line in result.stdout.strip().split('\n')[1:]:  # Skip header
                    safe_print(f"  {line}", Fore.YELLOW)
                return True
        elif sys.platform.startswith("linux"):
            result = subprocess.run(['lsof', port_name], capture_output=True, text=True, timeout=5)
            if result.returncode == 0 and result.stdout.strip():
                safe_print(f"Processes using {port_name}:", Fore.YELLOW)
                for line in result.stdout.strip().split('\n')[1:]:  # Skip header
                    safe_print(f"  {line}", Fore.YELLOW)
                return True
        # Windows: could use netstat or handle.exe but those are more complex
        return False
    except Exception:
        return False

def force_clear_arduino_port():
    """Manually force clear the Arduino port"""
    global ser, serialconnected, arduinoPort
    
    if not arduinoPort:
        safe_print("No Arduino port configured", Fore.RED)
        return
    
    safe_print(f"Force clearing Arduino port {arduinoPort}...", Fore.CYAN)
    
    # Step 1: Check what's using the port
    safe_print("Checking port usage:", Fore.YELLOW)
    port_in_use = check_port_usage(arduinoPort)
    if not port_in_use:
        safe_print("No processes found using the port", Fore.GREEN)
    
    # Step 2: Force Jumperless to disconnect
    # safe_print("Sending disconnect commands to Jumperless...", Fore.YELLOW)
    # try:
    #     if ser and serialconnected:
    #         ser.write(b"a")  # Disconnect UART
    #         time.sleep(0.3)
    #         ser.write(b"a")  # Send again
    #         time.sleep(0.3)
    #         ser.write(b"m")  # Menu mode
    #         time.sleep(0.5)
    #         safe_print("Jumperless disconnect commands sent", Fore.GREEN)
    #     else:
    #         safe_print("Jumperless not connected", Fore.YELLOW)
    # except Exception as e:
    #     safe_print(f"Error sending disconnect commands: {e}", Fore.RED)
    
    # Step 3: Test port accessibility
    safe_print("Testing Arduino port accessibility...", Fore.YELLOW)
    for attempt in range(5):
        try:
            test_serial = serial.Serial(arduinoPort, 115200, timeout=0.1)
            test_serial.close()
            safe_print(f"Arduino port {arduinoPort} is now accessible! (attempt {attempt + 1})", Fore.GREEN)
            return
        except Exception as e:
            if attempt < 4:
                safe_print(f"Port still busy, waiting... (attempt {attempt + 1}/5)", Fore.YELLOW)
                time.sleep(1.0)
            else:
                safe_print(f"Port still not accessible after 5 attempts: {e}", Fore.RED)
    
    # Step 4: Final diagnosis
    safe_print("Final port usage check:", Fore.YELLOW)
    check_port_usage(arduinoPort)
    safe_print("Try unplugging and reconnecting the Arduino USB cable", Fore.CYAN)

def flash_arduino_sketch_threaded(sketch_content, libraries_content="", slot_number=None):
    """Thread wrapper for Arduino sketch flashing"""
    def flash_worker():
        arduino_port_was_busy = False
        try:
            result = flash_arduino_sketch(sketch_content, libraries_content, slot_number)
            if result:
                safe_print(f"Arduino flash completed successfully for slot {slot_number}", Fore.GREEN)
            else:
                safe_print(f"Arduino flash failed for slot {slot_number}", Fore.RED)
        except Exception as e:
            safe_print(f"Arduino flash thread error for slot {slot_number}: {e}", Fore.RED)
        finally:
            # Remove this thread from tracking
            try:
                current_thread = threading.current_thread()
                if current_thread in active_threads:
                    active_threads.remove(current_thread)
            except:
                pass
            
            # Simplified thread cleanup - just verify port is released
            try:
                if debugWokwi:
                    safe_print(f"Thread cleanup for slot {slot_number}...", Fore.CYAN)
                
                # Brief wait for port to stabilize
                time.sleep(0.2)
                
                # Single verification that Arduino port is released
                try:
                    test_serial = serial.Serial(arduinoPort, 115200, timeout=0.1)
                    test_serial.close()
                    if debugWokwi:
                        safe_print(f"Arduino port {arduinoPort} released successfully", Fore.GREEN)
                except Exception as port_test_error:
                    if debugWokwi:
                        safe_print(f"Note: Arduino port may still be busy: {port_test_error}", Fore.YELLOW)
                    
            except Exception as cleanup_error:
                if debugWokwi:
                    safe_print(f"Thread cleanup error: {cleanup_error}", Fore.YELLOW)
    
    # Start the flash operation in its own thread
    flash_thread = threading.Thread(target=flash_worker, daemon=False)  # Non-daemon for proper cleanup
    flash_thread.start()
    
    # Track this thread for cleanup on exit
    global active_threads
    active_threads.append(flash_thread)
    
    safe_print(f"Arduino flash started in background for slot {slot_number}...", Fore.CYAN)
    
    # Return thread so caller can wait for it if needed
    return flash_thread

def flash_arduino_sketch(sketch_content, libraries_content="", slot_number=None):
    """Flash Arduino sketch to connected Arduino"""
    global arduino, arduinoPort, ser, menuEntered, serialconnected, arduino_flash_lock, forceArduinoFlash
    
    if noArduinocli or not arduino:
        safe_print("Arduino CLI not available", Fore.RED)
        return False
    
    if disableArduinoFlashing and not forceArduinoFlash:
        safe_print("Arduino flashing is disabled", Fore.YELLOW)
        return False
    
    if not arduinoPort:
        safe_print("No Arduino port configured", Fore.RED)
        return False
    
    if not sketch_content or len(sketch_content.strip()) < 10:
        safe_print("Invalid or empty sketch content", Fore.RED)
        return False
    
    # Acquire flash lock to prevent concurrent uploads
    flash_acquired = arduino_flash_lock.acquire(blocking=False)
    if not flash_acquired:
        safe_print(f"Arduino flash already in progress, skipping slot {slot_number}", Fore.YELLOW)
        return False
    
    # Flag to track UART state
    uart_was_connected = False
    arduino_serial = None
    port_available = False
    uart_mode_changed = False

    try:
        # Set UART mode for Arduino flashing
        try:
            ser.write(b"A?")
            time.sleep(0.3)
            response = ser.read_all() if hasattr(ser, 'read_all') else ser.read(ser.in_waiting or 1)
            if b"Y" in response:
                safe_print("UART lines are already connected", Fore.GREEN)
                uart_was_connected = True
            else:
                safe_print("Setting UART mode...", Fore.CYAN)
                ser.write(b"A")
                uart_was_connected = False
                uart_mode_changed = True
            time.sleep(1.0)  # Allow UART mode to stabilize
        except Exception as uart_error:
            safe_print(f"UART mode setup issue: {uart_error}", Fore.YELLOW)
            # Fallback: just set UART mode
            ser.write(b"A")
            uart_was_connected = False
            uart_mode_changed = True
            time.sleep(1.0)
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
            
            # List available boards to debug FQBN issues
            try:
                board_list = arduino.board.listall()
                if debugWokwi:
                    safe_print("Available boards:", Fore.CYAN)
                    for board in board_list[:5]:  # Show first 5 boards
                        safe_print(f"  {board}", Fore.CYAN)
            except Exception as board_error:
                safe_print(f"Could not list boards: {board_error}", Fore.YELLOW)
                
        except Exception as e:
            safe_print(f"Warning: Arduino core installation issue: {e}", Fore.YELLOW)
            # Try to continue anyway
            safe_print("Attempting to proceed without core verification...", Fore.YELLOW)
        
        # Compile the sketch
        safe_print("Compiling Arduino sketch...", Fore.CYAN)
        # if ser and serialconnected:
        #     try:
        #         ser.write(b"_")  # Progress indicator
        #     except:
        #         pass
        
        try:
            # Compile without specifying build_path - let Arduino CLI use default location
            compile_result = arduino.compile(
                sketch_dir,
                port=arduinoPort,
                fqbn="arduino:avr:nano",
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
        # Reset Arduino to bootloader mode before upload
        safe_print("Resetting Arduino to bootloader mode...", Fore.CYAN)
        try:
            ser.write(b"r")
            time.sleep(2.5)  # Give Arduino time to reset and enter bootloader
            safe_print("Arduino reset complete", Fore.GREEN)
        except Exception as reset_error:
            safe_print(f"Could not send reset command: {reset_error}", Fore.YELLOW)

        # Force clear Arduino port after reset
        safe_print("Ensuring Arduino port is available...", Fore.CYAN)
        try:
            # Send disconnect command to ensure clean port state
            # if ser and ser.is_open:
            #     ser.write(b"a")  # Disconnect UART briefly
            #     time.sleep(0.2)
            
            # Test port accessibility
            test_serial = serial.Serial(arduinoPort, 115200, timeout=0.1)
            test_serial.close()
            safe_print("Arduino port verified accessible", Fore.GREEN)
        except Exception as port_error:
            safe_print(f"Warning: Arduino port may be busy: {port_error}", Fore.YELLOW)
        
        # # Reset Arduino to enter bootloader mode
        # safe_print("Resetting Arduino to bootloader mode...", Fore.YELLOW)
        # try:
        #     # Open Arduino port briefly to trigger DTR reset
        #     arduino_reset_serial = serial.Serial(arduinoPort, 1200, timeout=0.1)
        #     arduino_reset_serial.setDTR(False)
        #     time.sleep(0.1)
        #     arduino_reset_serial.setDTR(True)
        #     time.sleep(0.1)
        #     arduino_reset_serial.close()
            
        #     # Wait for Arduino to reset and enter bootloader
        #     safe_print("Waiting for Arduino bootloader...", Fore.YELLOW)
        #     time.sleep(2.0)  # Give Arduino time to reset and start bootloader
            
        # except Exception as reset_error:
        #     safe_print(f"Could not reset Arduino via DTR: {reset_error}", Fore.YELLOW)
        #     # Fallback: try manual reset command
        #     try:
        #         ser.write(b"r")
        #         time.sleep(2.0)
        #         safe_print("Sent manual reset command", Fore.YELLOW)
        #     except Exception as manual_reset_error:
        #         safe_print(f"Manual reset also failed: {manual_reset_error}", Fore.RED)
        
        # Upload to Arduino
        safe_print("Uploading to Arduino...", Fore.CYAN)
        
        # First attempt without reset
        try:
            upload_result = upload_with_attempts_limit(
                sketch_dir,
                arduinoPort,
                "arduino:avr:nano",
                None,  # No custom build dir - Arduino CLI will use default
                "2s"
            )
            
            safe_print("Arduino flashed successfully!", Fore.GREEN)
            return True
            
        except Exception as e:
            error_msg = str(e).lower()
            
            # Check if it's a sync/communication error that might be fixed with a reset
            sync_errors = ['not responding', 'timeout', 'sync', 'stk500_recv', 'stk500_getsync']
            is_sync_error = any(err in error_msg for err in sync_errors)
            
            if is_sync_error:
                safe_print(f"Upload failed with sync error, trying reset and retry...", Fore.YELLOW)
                
                # Reset Arduino and wait for it to stabilize
                try:
                    ser.write(b"r")
                    time.sleep(2.0)  # Give Arduino time to reset and be ready
                    safe_print("Arduino reset, retrying upload...", Fore.CYAN)
                    
                    # Second attempt after reset
                    upload_result = upload_with_attempts_limit(
                        sketch_dir,
                        arduinoPort,
                        "arduino:avr:nano",
                        None,
                        "2s"
                    )
                    
                    safe_print("Arduino flashed successfully on retry!", Fore.GREEN)
                    return True
                    
                except Exception as retry_error:
                    safe_print(f"Retry also failed: {retry_error}", Fore.RED)
            else:
                safe_print(f"Upload failed: {e}", Fore.RED)
            
            return False
    
    except Exception as e:
        safe_print(f"Arduino flashing error: {e}", Fore.RED)
        return False
    
    finally:
        # Always cleanup, regardless of success or failure
        try:
            # Close Arduino serial port if we opened it
            if arduino_serial is not None:
                try:
                    if arduino_serial.is_open:
                        arduino_serial.close()
                        safe_print(f"Closed Arduino port {arduinoPort} in cleanup", Fore.YELLOW)
                except Exception as close_error:
                    safe_print(f"Note: Error closing Arduino port in cleanup: {close_error}", Fore.YELLOW)
            
            # Restore UART state if we changed it
            if uart_mode_changed and ser and serialconnected and not uart_was_connected:
                try:
                    time.sleep(0.1)
                    ser.write(b"a")  # Disconnect UART
                    time.sleep(0.1)
                    safe_print("Restored UART mode", Fore.CYAN)
                except Exception as uart_error:
                    safe_print(f"Note: Error restoring UART mode: {uart_error}", Fore.YELLOW)
            
            # Send menu command to return to normal mode
            if ser and serialconnected:
                try:
                    time.sleep(0.1)
                    ser.write(b'm')
                    time.sleep(0.1)
                except Exception as menu_error:
                    safe_print(f"Note: Error sending menu command: {menu_error}", Fore.YELLOW)
                    
        except Exception as cleanup_error:
            safe_print(f"Error in flash cleanup: {cleanup_error}", Fore.RED)
        finally:
            # Always release the flash lock - CRITICAL for preventing deadlocks
            if flash_acquired:
                try:
                    arduino_flash_lock.release()
                    if debugWokwi:
                        safe_print(f"Released Arduino flash lock for slot {slot_number}", Fore.CYAN)
                except Exception as lock_error:
                    safe_print(f"Error releasing flash lock: {lock_error}", Fore.RED)

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
            
            # Check if sketch has changed since last time OR if force flash is requested
            global forceArduinoFlash
            sketch_changed = lastsketch[slot_number] != sketch_content
            should_flash = sketch_changed or forceArduinoFlash
            
            if should_flash:
                lastsketch[slot_number] = sketch_content
                
                if debugWokwi:
                    if forceArduinoFlash:
                        safe_print(f"Force flashing sketch for slot {slot_number}:", Fore.GREEN)
                    else:
                        safe_print(f"Found new sketch for slot {slot_number}:", Fore.GREEN)
                    safe_print(sketch_content[:200] + ('...' if len(sketch_content) > 200 else ''), Fore.CYAN)
                
                # Flash the Arduino if the sketch is valid
                if len(sketch_content) > 10:
                    if forceArduinoFlash:
                        safe_print(f"\nForce flashing Arduino sketch for slot {slot_number}...", Fore.MAGENTA)
                        forceArduinoFlash = 0  # Reset flag after use
                    else:
                        safe_print(f"\nNew Arduino sketch for slot {slot_number} - flashing...", Fore.MAGENTA)
                    
                    # Only flash once
                    try:
                        # Start threaded flash and return the thread object
                        flash_thread = flash_arduino_sketch_threaded(sketch_content, libraries_content, slot_number)
                        return flash_thread is not None
                    except Exception as e:
                        safe_print(f"Error during Arduino flashing: {e}", Fore.RED)
                        forceArduinoFlash = 0  # Reset flag on error too
                        return False
                else:
                    safe_print(f"Sketch too short. Not flashing.", Fore.YELLOW)
                    forceArduinoFlash = 0  # Reset flag
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
                        
                        safe_print(f"\rReconnected to {correct_port}", Fore.GREEN)
                        safe_print(f"\r", Fore.GREEN)
                        disable_interactive_mode()
                        
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
    """Handle incoming serial data with reliable byte-by-byte reading and interactive mode control"""
    global serialconnected, ser, menuEntered, portNotFound, justreconnected, updateInProgress, interactive_mode
    
    while True:
        # Always check for interactive mode control characters, even during menu/updates
        if updateInProgress == 0:
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
                            # Always check for interactive mode control characters
                            filtered_buffer = b''
                            
                            for byte in input_buffer:
                                if byte == 0x0E:  # SO (Shift Out) - Enable interactive mode
                                    if debugWokwi:
                                        safe_print(f"\nReceived SO (0x0E) - Interactive mode request", end="\n\r", color=Fore.BLUE)
                                    if not interactive_mode:
                                        safe_print("\nInteractive mode enabled by device", end="\n\r", color=Fore.RED)
                                        enable_interactive_mode()
                                elif byte == 0x0F:  # SI (Shift In) - Disable interactive mode
                                    if debugWokwi:
                                        safe_print(f"\nReceived SI (0x0F) - Interactive mode disable", end="\n\r", color=Fore.BLUE)
                                    if interactive_mode:
                                        disable_interactive_mode()
                                        safe_print("Interactive mode disabled by device",end="\n\r", color=Fore.GREEN)
                                else:
                                    # Keep all other bytes for normal processing
                                    filtered_buffer += bytes([byte])
                            
                            # Only print output if not in menu mode and there's actual content
                            if menuEntered == 0 and filtered_buffer:
                                decoded_string = filtered_buffer.decode('utf-8', errors='ignore')
                                if interactive_mode:
                                    print(decoded_string, end='')
                                else:
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
    """Handle outgoing serial commands with command history support and interactive mode"""
    global serialconnected, ser, menuEntered, forceWokwiUpdate, noWokwiStuff, justreconnected, portNotFound, updateInProgress, interactive_mode, output_buffer
    
    while True:
        if menuEntered == 0 and updateInProgress == 0:
            try:
                # Check if we should enter interactive mode
                if interactive_mode and not menuEntered:
                    # safe_print("Interactive mode is active, starting handler", Fore.CYAN)
                    handle_interactive_input()
                    continue
                
                # Clear any pending input buffer before reading
                if hasattr(sys.stdin, 'flush'):
                    sys.stdin.flush()
                
                # Use readline for command history if available, otherwise fallback to input()
                if READLINE_AVAILABLE:
                    try:
                        output_buffer = input() # readline automatically handles history
                    except (EOFError, KeyboardInterrupt):
                        # Handle Ctrl+D or Ctrl+C gracefully
                        if menuEntered == 0:
                            continue
                        else:
                            break
                else:
                    output_buffer = input() 
                
                # Handle empty input (just pressing enter)
                if not output_buffer:  # This catches empty string when user just presses enter
                    # Send newline to serial port
                    # safe_print("sending newline\n\r", end="")
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            try:
                                ser.write(b'\n')
                                # if debugWokwi:
                                safe_print("\n\r", end="")
                            except Exception:
                                pass
                    continue
                
                # Add command to history if readline is available and it's not a duplicate
                if READLINE_AVAILABLE and output_buffer:
                    # Check if this command is different from the last one to avoid duplicates
                    history_length = readline.get_current_history_length()
                    if history_length == 0 or (history_length > 0 and 
                        readline.get_history_item(history_length) != output_buffer):
                        readline.add_history(output_buffer)
                
                # Handle special commands - check if it's a menu command
                menu_commands = ['menu', 'slots', 'wokwi', 'update', 'appupdate', 'debugupdate', 'status', 'rate', 'exit', 'arduino', 'debug', 'config']
                
                if output_buffer.lower() in menu_commands:
                    # print("*", output_buffer, "*")
                    menuEntered = 1
                    bridge_menu()
                    continue
                elif output_buffer.lower() == 'interactive':
                    # Toggle interactive mode
                    if enable_interactive_mode():
                        continue
                elif output_buffer.lower() == 'status':
                    # Quick status check
                    safe_print(f"Interactive mode: {interactive_mode}", Fore.CYAN)
                    safe_print(f"Menu entered: {menuEntered}", Fore.CYAN)
                    safe_print(f"Update in progress: {updateInProgress}", Fore.CYAN)
                    continue
                elif output_buffer.lower() == 'flash':
                    # Handle flash command directly without entering menu
                    handle_flash_command()
                    continue
                elif output_buffer.lower() == 'clearport':
                    # Force clear Arduino port
                    force_clear_arduino_port()
                    continue
                elif output_buffer.lower() == 'newline' or output_buffer.lower() == '\n':
                    # Send just a newline
                    # safe_print("\n\rNewline", end="")
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            try:
                                ser.write(b'\n')
                                # safe_print("\n\r", end="")
                            except Exception:
                                pass
                    continue
                elif output_buffer.lower() == '\t':
                    # Send a tab character
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            try:
                                ser.write(b'\t')
                            except Exception:
                                pass
                    continue
                
                # Send to serial port (only non-empty commands)
                with serial_lock:
                    if serialconnected and ser and ser.is_open:
                        try:
                            ser.write(output_buffer.encode('ascii'))
                            # Check for reset command
                            # if output_buffer.lower() == 'r':
                            #     justreconnected = 1
                        except Exception:
                            # Only mark as disconnected if not updating firmware
                            if updateInProgress == 0:
                                portNotFound = 1
                                serialconnected = 0
                    else:
                        if updateInProgress == 0:
                            safe_print("Serial not connected", Fore.YELLOW)
                        
            except KeyboardInterrupt:
                # Make sure to disable interactive mode on Ctrl+C
                if interactive_mode:
                    disable_interactive_mode()
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
    print("\nNote: This app looks best on a dark background")
    create_directories()
        # Check for app updates
    update_app_if_needed()
    
    
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
    active_threads.append(port_monitor)
    
    serial_in = threading.Thread(target=serial_term_in, daemon=True)
    serial_in.start()
    active_threads.append(serial_in)
    
    serial_out = threading.Thread(target=serial_term_out, daemon=True)
    serial_out.start()
    active_threads.append(serial_out)
    
    time.sleep(0.1)
    safe_print("Type 'menu' for App Menu, 'flash' to flash Arduino, 'interactive' for real-time mode", Fore.CYAN)
    safe_print("Device can auto-enable interactive mode with SO (0x0E) and disable with SI (0x0F)", Fore.BLUE)
    
    if sys.platform == "win32":
        safe_print("Interactive mode available (Windows pynput - full Ctrl key support)", Fore.GREEN)
    else:
        safe_print("Interactive mode available (Unix termios)", Fore.GREEN)
    
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
            # Give precedence to serial_term_out thread for user input responsiveness
            # Small sleep to yield CPU time to other threads, especially when processing lots of serial data
            time.sleep(0.01)
            
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
                                    
                                    # Send to Jumperless (only if not updating firmware and not empty)
                                    if updateInProgress == 0:
                                        # Check if command is empty (just "{ }" or similar)
                                        command_content = command.strip().replace('{', '').replace('}', '').replace(' ', '').replace(',', '')
                                        
                                        if command_content:  # Only send if there's actual content
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
                                        else:
                                            if debugWokwi:
                                                safe_print(f"Skipping empty project for slot {currentSlotUpdate}", Fore.BLUE)
                                
                                # Check for Arduino sketch changes and flash if needed
                                if not noArduinocli and (not disableArduinoFlashing or forceArduinoFlash) and updateInProgress == 0:
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
                            if not noArduinocli and (not disableArduinoFlashing or forceArduinoFlash) and updateInProgress == 0:
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
    # readline.parse_and_bind("tab: complete")
    
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

def enable_interactive_mode():
    """Enable character-by-character input mode"""
    global interactive_mode, original_settings
    
    if interactive_mode:
        return True
    
    try:
        # Disable readline completion and history during interactive mode
        if READLINE_AVAILABLE:
            # Save current completer and disable it
            original_settings = {
                'completer': readline.get_completer(),
                'delims': readline.get_completer_delims()
            }
            readline.set_completer(None)  # Disable tab completion
            readline.parse_and_bind("tab: self-insert")  # Make tab insert literal tab
        
        interactive_mode = True
        
        # Platform-specific messages
        if sys.platform == "win32":
            safe_print("\nInteractive mode ON - (Ctrl+C to exit)", Fore.BLUE)
            safe_print("Using pynput for enhanced keyboard input (full Ctrl key support)", Fore.CYAN)
        else:
            safe_print("\nInteractive mode ON - (ESC to force off)", Fore.BLUE)
            safe_print("Using termios for raw terminal input", Fore.CYAN)
        
        return True
        
    except Exception as e:
        safe_print(f"Could not enable interactive mode: {e}", Fore.RED)
        interactive_mode = False
        return False

def disable_interactive_mode():
    """Disable interactive mode and restore normal input"""
    global interactive_mode, original_settings
    
    if not interactive_mode:
        return
    
    try:
        # Restore readline completion and history
        if READLINE_AVAILABLE and original_settings:
            try:
                readline.set_completer(original_settings.get('completer'))
                readline.parse_and_bind("tab: complete")  # Restore tab completion
                original_settings = None
            except Exception as e:
                safe_print(f"Error restoring readline settings: {e}", Fore.YELLOW)
        
        interactive_mode = False
        safe_print("\nInteractive mode OFF - normal line input restored\r", Fore.GREEN)
        
    except Exception as e:
        safe_print(f"Error disabling interactive mode: {e}", Fore.RED)

def handle_interactive_input_simple():
    """Simple approach using sys.stdin with raw mode"""
    global interactive_mode, serialconnected, ser
    
    # safe_print("Using simple character-by-character input", Fore.GREEN)
    
    try:
        import termios
        import tty
        import select
        
        # Save original terminal settings (separate from readline settings)
        original_terminal_settings = termios.tcgetattr(sys.stdin.fileno())
        
        try:
            # Set terminal to raw mode
            tty.setraw(sys.stdin.fileno())
            
            while interactive_mode:
                # Check if input is available with a short timeout
                if select.select([sys.stdin], [], [], 0.1)[0]:
                    char = sys.stdin.read(1)
                    
                    # Handle special characters
                    # if char == '\x1b':  # ESC key
                        # break  # Exit the loop, disable_interactive_mode will be called below
                    if char == '\x03':  # Ctrl+C
                        raise KeyboardInterrupt
                    elif char == '\r':
                        char = '\n'  # Convert to newline for MicroPython compatibility
                    if char == '\x1b':  # ESC key
                        esc_sequence = char + sys.stdin.read(2)
                        with serial_lock:
                            if serialconnected and ser and ser.is_open:
                                try:
                                    ser.write(esc_sequence.encode('utf-8', errors='ignore'))
                                except Exception as e:
                                    safe_print(f"Error sending character: {e}", Fore.RED)
                        continue
                    else:
                    # Send character to serial port
                        with serial_lock:
                            if serialconnected and ser and ser.is_open:
                                try:
                                    ser.write(char.encode('utf-8', errors='ignore'))
                                except Exception as e:
                                    safe_print(f"Error sending character: {e}", Fore.RED)
                                
        finally:
            # Always restore original terminal settings
            try:
                termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, original_terminal_settings)
            except:
                pass
            
            # Disable interactive mode if we exited the loop
            if interactive_mode:
                disable_interactive_mode()
                
    except ImportError:
        safe_print("WARNING: termios not available - cannot use interactive mode", Fore.YELLOW)
        disable_interactive_mode()
    except KeyboardInterrupt:
        disable_interactive_mode()
        safe_print("\nInteractive mode interrupted", Fore.YELLOW)
    except Exception as e:
        safe_print(f"Interactive mode error: {e}", Fore.RED)
        disable_interactive_mode()

def handle_interactive_input_windows():
    """Windows-specific approach using pynput for complete key support"""
    global interactive_mode, serialconnected, ser
    
    # Check if pynput is available
    try:
        import pynput.keyboard as pynput_keyboard
    except ImportError:
        safe_print("WARNING: pynput not available - falling back to simple input mode", Fore.YELLOW)
        disable_interactive_mode()
        return
    
    try:
        safe_print("Using pynput for enhanced Windows keyboard input (supports all Ctrl combinations)", Fore.GREEN)
        
        # Create a controller instance for modifier state checking
        controller = pynput_keyboard.Controller()
        
        def on_key_press(key):
            if not interactive_mode:
                return False  # Stop listener
            
            try:
                # Handle special keys
                if key == pynput_keyboard.Key.esc:
                    # Send ESC sequence
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b')
                elif key == pynput_keyboard.Key.enter:
                    # Send newline
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\n')
                elif key == pynput_keyboard.Key.backspace:
                    # Send backspace
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x08')
                elif key == pynput_keyboard.Key.tab:
                    # Send tab
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\t')
                # Arrow keys
                elif key == pynput_keyboard.Key.up:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[A')
                elif key == pynput_keyboard.Key.down:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[B')
                elif key == pynput_keyboard.Key.right:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[C')
                elif key == pynput_keyboard.Key.left:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[D')
                # Navigation keys
                elif key == pynput_keyboard.Key.home:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[H')
                elif key == pynput_keyboard.Key.end:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[F')
                elif key == pynput_keyboard.Key.page_up:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[5~')
                elif key == pynput_keyboard.Key.page_down:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[6~')
                elif key == pynput_keyboard.Key.insert:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[2~')
                elif key == pynput_keyboard.Key.delete:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1b[3~')
                # Function keys
                elif key == pynput_keyboard.Key.f1:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1bOP')
                elif key == pynput_keyboard.Key.f2:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1bOQ')
                elif key == pynput_keyboard.Key.f3:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1bOR')
                elif key == pynput_keyboard.Key.f4:
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(b'\x1bOS')
                # Handle printable characters (including Ctrl combinations)
                elif hasattr(key, 'char') and key.char:
                    char_to_send = key.char
                    
                    # Handle Ctrl combinations properly
                    if hasattr(key, 'vk'):
                        # Check for Ctrl combinations by checking current modifier state
                        try:
                            with controller.modifiers as modifiers:
                                if pynput_keyboard.Key.ctrl in modifiers or pynput_keyboard.Key.ctrl_l in modifiers or pynput_keyboard.Key.ctrl_r in modifiers:
                                    # Convert to control character
                                    if key.char and len(key.char) == 1:
                                        char_code = ord(key.char.upper())
                                        if 65 <= char_code <= 90:  # A-Z
                                            ctrl_char = chr(char_code - 64)  # Ctrl+A = 1, Ctrl+B = 2, etc.
                                            char_to_send = ctrl_char
                        except:
                            pass  # Use original character if modifier check fails
                    
                    # Send the character
                    with serial_lock:
                        if serialconnected and ser and ser.is_open:
                            ser.write(char_to_send.encode('utf-8', errors='ignore'))
                            
                elif debugWokwi:
                    safe_print(f"Unhandled special key: {key}", Fore.YELLOW)
                    
            except Exception as e:
                if debugWokwi:
                    safe_print(f"Error handling key press: {e}", Fore.RED)
        
        # Start listener
        with pynput_keyboard.Listener(on_press=on_key_press, suppress=False) as listener:
            listener.join()
            
    except Exception as e:
        safe_print(f"pynput interactive mode error: {e}", Fore.RED)
        disable_interactive_mode()
    finally:
        if interactive_mode:
            disable_interactive_mode()



def handle_interactive_input():
    """Handle character-by-character input in interactive mode"""
    global interactive_mode, serialconnected, ser
    
    if sys.platform == "win32":
        # Use Windows-specific msvcrt approach
        return handle_interactive_input_windows()
    else:
        # Use Unix/Linux/macOS termios approach
        return handle_interactive_input_simple()

def get_command_suggestions():
    """Get list of common commands for tab completion"""
    # This function is now only used for reference/documentation
    # Tab completion uses actual command history instead
    return [
        # App commands
        'menu', 'slots', 'wokwi', 'flash', 'clearport', 'skip', 'rate', 'update', 'status', 'exit', 'help', 'interactive',
        # Jumperless device commands
        'r', 'm', 'f', 'n', 'l', 'b', 'c', 'x', 'clear', 'o', '?',
    ]

        
        

def cleanup_on_exit():
    """Clean up all active processes and threads on script exit"""
    global active_processes, active_threads, ser, serialconnected, interactive_mode
    import subprocess
    
    # Disable interactive mode first
    if interactive_mode:
        disable_interactive_mode()
    
    # safe_print("\nCleaning up processes and threads...", Fore.YELLOW)
    
    # Terminate active subprocesses
    for process in active_processes[:]:  # Copy list to avoid modification during iteration
        try:
            if process.poll() is None:  # Process is still running
                # safe_print(f"Terminating subprocess PID {process.pid}...", Fore.CYAN)
                process.terminate()
                try:
                    process.wait(timeout=0.2)
                    safe_print(f"Subprocess PID {process.pid} terminated gracefully", Fore.GREEN)
                except subprocess.TimeoutExpired:
                    safe_print(f"Force killing subprocess PID {process.pid}...", Fore.RED)
                    process.kill()
                    try:
                        process.wait(timeout=1)
                    except subprocess.TimeoutExpired:
                        safe_print(f"Subprocess PID {process.pid} is unresponsive", Fore.RED)
            active_processes.remove(process)
        except Exception as e:
            safe_print(f"Error terminating subprocess: {e}", Fore.RED)
    
    # Signal threads to stop (daemon threads will be killed automatically)
    # Non-daemon threads need to be handled if they don't respond to interrupts
    non_daemon_threads = [t for t in active_threads if t.is_alive() and not t.daemon]
    if non_daemon_threads:
        safe_print(f"Waiting for {len(non_daemon_threads)} non-daemon threads to finish...", Fore.CYAN)
        for thread in non_daemon_threads:
            try:
                thread.join(timeout=2)  # Wait up to 2 seconds for each thread
                if thread.is_alive():
                    safe_print(f"Thread {thread.name} is still running (will be force-terminated)", Fore.YELLOW)
                else:
                    safe_print(f"Thread {thread.name} finished cleanly", Fore.GREEN)
            except Exception as e:
                safe_print(f"Error waiting for thread {thread.name}: {e}", Fore.RED)
    
    # Close serial connections
    try:
        if ser and serialconnected:
            ser.close()
            safe_print("Closed Jumperless serial connection", Fore.GREEN)
    except Exception as e:
        safe_print(f"Error closing serial connection: {e}", Fore.RED)
    
    # Force clear Arduino port one final time
    try:
        if arduinoPort:
            force_clear_arduino_port()
    except Exception as e:
        safe_print(f"Error in final Arduino port clear: {e}", Fore.RED)
    
    safe_print("Cleanup completed", Fore.GREEN)
    

if __name__ == "__main__":
    # Register cleanup function for normal exit
    import atexit
    atexit.register(cleanup_on_exit)
    
    try:
        main()
    except KeyboardInterrupt:
        safe_print("\nKeyboard interrupt received (Ctrl+C)", Fore.YELLOW)
        cleanup_on_exit()
        safe_print("Exiting...", Fore.YELLOW)
    except Exception as e:
        safe_print(f"Fatal error: {e}", Fore.RED)
        cleanup_on_exit()
    finally:
        # Ensure cleanup happens even if something goes wrong
        try:
            # Only call cleanup if it hasn't been called yet
            if active_processes or (ser and serialconnected):
                cleanup_on_exit()
        except:
            pass