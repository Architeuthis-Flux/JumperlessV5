#!/usr/bin/env python3
"""
Simple Auto Upload for Jumperless
Watches for a specific USB port, uploads firmware once, then waits for disconnect/reconnect.

Supports two upload methods:
1. PlatformIO upload (default) - for development builds
2. UF2 file copy - for pre-built firmware files with automatic bootloader triggering

Features:
- Automatic bootloader mode triggering via 1200 baud connection
- Cross-platform drive detection for UF2 uploads
- Backward compatible with existing workflows

Usage:
  python3 simple_auto_upload.py [PORT] [--uf2 UF2_FILE]

Options:
  PORT        USB port (e.g., /dev/cu.usbmodem101, COM3)
  --uf2 FILE  Use specific UF2 file for upload
  --uf2=FILE  Alternative syntax for UF2 file

Examples:
  python3 simple_auto_upload.py /dev/cu.usbmodem101
  python3 simple_auto_upload.py COM3
  python3 simple_auto_upload.py --uf2 firmware.uf2
  python3 simple_auto_upload.py /dev/cu.usbmodem101 --uf2 custom.uf2
"""

import subprocess
import time
import sys
import os
import shutil
import serial.tools.list_ports
import glob



#python autoUploader/simple_auto_upload.py

# Configuration
USB_PORT = "/dev/cu.usbmodem101"  # Change this to your Jumperless port
ENVIRONMENT = "jumperless_v5"
CHECK_INTERVAL = 1  # seconds

def find_uf2_files(directory="."):
    """Find all UF2 files in the given directory"""
    pattern = os.path.join(directory, "*.uf2")
    return glob.glob(pattern)

def find_jumperless_mount():
    """Find mounted Jumperless drive by looking for INFO_UF2.TXT"""
    if sys.platform == "darwin":  # macOS
        import subprocess
        try:
            result = subprocess.run(['mount'], capture_output=True, text=True)
            mount_lines = result.stdout.split('\n')
            for line in mount_lines:
                # Extract mount point from the line (format: "/dev/diskXsY on /Volumes/NAME (type, options)")
                if ' on /Volumes/' in line:
                    mount_point = line.split(' on ')[1].split(' (')[0]

                    # Check if this is a potential RP2040/RP2350 drive
                    if any(pattern in mount_point.upper() for pattern in ['RP2350', 'RP2040', 'RPI-RP2']):
                        info_file = os.path.join(mount_point, "INFO_UF2.TXT")
                        if os.path.exists(info_file):
                            return mount_point

                    # Also check any volume that has INFO_UF2.TXT (fallback for custom names)
                    info_file = os.path.join(mount_point, "INFO_UF2.TXT")
                    if os.path.exists(info_file):
                        try:
                            with open(info_file, 'r') as f:
                                content = f.read()
                                # Accept any valid UF2 bootloader (RP2040, RP2350, etc.)
                                if any(pattern in content.upper() for pattern in ['RASPBERRY PI', 'UF2 BOOTLOADER']):
                                    return mount_point
                        except:
                            pass
        except:
            pass
    elif sys.platform == "linux":
        # Linux mount detection
        try:
            with open('/proc/mounts', 'r') as f:
                for line in f:
                    parts = line.split()
                    if len(parts) >= 2:
                        mount_point = parts[1]

                        # Check if this is a potential RP2040/RP2350 drive
                        if any(pattern in mount_point.upper() for pattern in ['RP2350', 'RP2040', 'RPI-RP2']):
                            info_file = os.path.join(mount_point, "INFO_UF2.TXT")
                            if os.path.exists(info_file):
                                return mount_point

                        # Also check any mount point that has INFO_UF2.TXT (fallback)
                        info_file = os.path.join(mount_point, "INFO_UF2.TXT")
                        if os.path.exists(info_file):
                            try:
                                with open(info_file, 'r') as f:
                                    content = f.read()
                                    # Accept any valid UF2 bootloader (RP2040, RP2350, etc.)
                                    if any(pattern in content.upper() for pattern in ['RASPBERRY PI', 'UF2 BOOTLOADER']):
                                        return mount_point
                            except:
                                pass
        except:
            pass
    elif sys.platform == "win32":
        # Windows drive detection
        import string
        for drive in string.ascii_uppercase:
            drive_path = f"{drive}:\\"

            # Check if this is a potential RP2040/RP2350 drive
            if any(pattern in drive_path.upper() for pattern in ['RP2350', 'RP2040', 'RPI-RP2']):
                info_file = os.path.join(drive_path, "INFO_UF2.TXT")
                if os.path.exists(info_file):
                    return drive_path

            # Also check any drive that has INFO_UF2.TXT (fallback)
            info_file = os.path.join(drive_path, "INFO_UF2.TXT")
            if os.path.exists(info_file):
                try:
                    with open(info_file, 'r') as f:
                        content = f.read()
                        # Accept any valid UF2 bootloader (RP2040, RP2350, etc.)
                        if any(pattern in content.upper() for pattern in ['RASPBERRY PI', 'UF2 BOOTLOADER']):
                            return drive_path
                except:
                    pass

    return None

class SimpleJumperlessUploader:
    def __init__(self):
        self.uploaded_to_current_device = False
        self.device_connected = False
        self.ignore_next_reconnect = False
        self.last_upload_time = 0
        
    def is_device_connected(self):
        """Check if the target USB port exists"""
        ports = [port.device for port in serial.tools.list_ports.comports()]
        return USB_PORT in ports
    
    def run_upload(self):
        """Run PlatformIO upload command"""
        print(f"↑ Uploading to {USB_PORT}...")
        print("=" * 50)
        
        cmd = [
            "platformio", "run", 
            "--target", "upload",
            "--environment", ENVIRONMENT,
            "--upload-port", USB_PORT
        ]
        
        try:
            # Run command and show output in real-time
            result = subprocess.run(cmd, text=True)
            
            print("=" * 50)
            if result.returncode == 0:
                print("☺ Upload successful!")
                self.last_upload_time = time.time()
                self.ignore_next_reconnect = True
                return True
            else:
                print("☹ Upload failed!")
                return False
                
        except FileNotFoundError:
            print("☹ PlatformIO not found! Install with: pip install platformio")
            return False
        except Exception as e:
            print(f"☹ Upload error: {e}")
            return False

    def trigger_bootloader(self):
        """Trigger RP2350B bootloader by connecting at 1200 baud"""
        print(f"🔧 Triggering bootloader on {USB_PORT}...")
        try:
            # Connect at 1200 baud for ~100ms to trigger bootloader
            with serial.Serial(USB_PORT, 1200, timeout=0.1) as ser:
                time.sleep(0.1)  # Hold connection briefly
            print("✅ Bootloader trigger sent")
            return True
        except Exception as e:
            print(f"⚠️ Could not trigger bootloader: {e}")
            return False

    def run_uf2_upload(self, uf2_file=None):
        """Upload UF2 file by copying to mounted Jumperless drive"""
        print(f"🔄 UF2 Upload mode - looking for mounted Jumperless drive...")

        # Find UF2 file if not provided
        if not uf2_file:
            uf2_files = find_uf2_files()
            if not uf2_files:
                print("☹ No UF2 files found in current directory!")
                return False
            uf2_file = uf2_files[0]
            print(f"📁 Using UF2 file: {uf2_file}")

        if not os.path.exists(uf2_file):
            print(f"☹ UF2 file not found: {uf2_file}")
            return False

        print("=" * 50)

        # Trigger bootloader mode
        print("🔧 Attempting to trigger bootloader mode...")
        if not self.trigger_bootloader():
            print("💡 Manual bootloader entry may be required")

        # Give device time to enter bootloader mode
        print("⏳ Waiting for device to enter bootloader mode...")
        time.sleep(2)

        # Wait for Jumperless drive to appear
        max_wait = 30  # seconds
        for i in range(max_wait):
            mount_point = find_jumperless_mount()
            if mount_point:
                break
            if i == 0:
                print(f"🔍 Looking for Jumperless drive...")
            time.sleep(1)

        if not mount_point:
            print("☹ Could not find mounted Jumperless drive!")
            print("💡 Device may not have entered bootloader mode. Try double-tap reset if needed.")
            return False

        print(f"🎯 Found Jumperless drive at: {mount_point}")

        try:
            # Copy UF2 file to the drive
            destination = os.path.join(mount_point, "firmware.uf2")
            print(f"📋 Copying {uf2_file} to {destination}...")
            shutil.copy2(uf2_file, destination)

            print("=" * 50)
            print("☺ UF2 upload successful!")
            print("✨ Device should restart automatically.")
            return True

        except Exception as e:
            print(f"☹ UF2 copy error: {e}")
            return False

    def run(self):
        """Main loop"""
        print(f"👀 Watching for Jumperless on {USB_PORT}")
        print("Press Ctrl+C to stop")
        
        try:
            while True:
                currently_connected = self.is_device_connected()
                
                # Device just connected
                if currently_connected and not self.device_connected:
                    # Check if we should ignore this reconnect (happens after upload)
                    time_since_upload = time.time() - self.last_upload_time
                    if self.ignore_next_reconnect and time_since_upload < 10:  # 30 second window
                        print(f"🔄 Device reconnected after upload - ignoring...")
                        
                        self.device_connected = True
                        self.ignore_next_reconnect = False
                    else:
                        print(f"◎ Device connected on {USB_PORT}")
                        self.device_connected = True
                        self.uploaded_to_current_device = False
                        self.ignore_next_reconnect = False
                        
                        # Wait a moment for device to settle
                        time.sleep(2)

                        # Upload if we haven't already
                        if not self.uploaded_to_current_device:
                            # Check if we should use UF2 upload method
                            if self.uf2_file or find_uf2_files():
                                print("🔄 Using UF2 upload method...")
                                uf2_file_to_use = self.uf2_file if self.uf2_file else None
                                success = self.run_uf2_upload(uf2_file_to_use)
                            else:
                                print("🔧 Using PlatformIO upload method...")
                                success = self.run_upload()

                            if success:
                                self.uploaded_to_current_device = True
                                print("✨ Ready! Disconnect and reconnect to upload again.")
                
                # Device disconnected
                elif not currently_connected and self.device_connected:
                    if self.ignore_next_reconnect:
                        print(f"🔄 Device disconnected during upload process...")
                    else:
                        print(f"◯ Device disconnected from {USB_PORT}")
                        self.uploaded_to_current_device = False
                    self.device_connected = False
                
                time.sleep(CHECK_INTERVAL)
                
        except KeyboardInterrupt:
            print("\n🛑 Stopping...")

def main():
    global USB_PORT
    usb_port = USB_PORT
    uf2_file = None

    # Parse command line arguments
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == "--uf2" and i + 1 < len(sys.argv):
            uf2_file = sys.argv[i + 1]
            i += 2
        elif sys.argv[i].startswith("--uf2="):
            uf2_file = sys.argv[i][6:]  # Remove --uf2=
            i += 1
        elif not sys.argv[i].startswith("--"):
            # Assume it's a port specification
            usb_port = sys.argv[i]
            i += 1
        else:
            i += 1

    USB_PORT = usb_port

    if uf2_file:
        print(f"Using custom UF2 file: {uf2_file}")

    print("○ Simple Jumperless Auto-Upload")
    print("=" * 40)
    print(f"Target port: {USB_PORT}")
    print(f"Environment: {ENVIRONMENT}")
    if uf2_file:
        print(f"UF2 file: {uf2_file}")
    print()

    uploader = SimpleJumperlessUploader()
    # Store the UF2 file for use in upload method
    uploader.uf2_file = uf2_file
    uploader.run()

if __name__ == "__main__":
    main() 