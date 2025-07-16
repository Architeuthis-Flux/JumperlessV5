#!/usr/bin/env python3
"""
Simple Auto Upload for Jumperless
Watches for a specific USB port, uploads firmware once, then waits for disconnect/reconnect.
python ../jumperlesswokwibridge/jumperlesswokwibridge.py
Usage:
  python3 simple_auto_upload.py [PORT]
  
Examples:
  python3 simple_auto_upload.py /dev/cu.usbmodem101
  python3 simple_auto_upload.py COM3
"""

import subprocess
import time
import sys
import os
import serial.tools.list_ports

# Configuration
USB_PORT = "/dev/cu.usbmodem101"  # Change this to your Jumperless port
ENVIRONMENT = "jumperless_v5"
CHECK_INTERVAL = 1  # seconds

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
    if len(sys.argv) > 1:
        global USB_PORT
        USB_PORT = sys.argv[1]
        print(f"Using custom port: {USB_PORT}")
    
    print("○ Simple Jumperless Auto-Upload")
    print("=" * 40)
    print(f"Target port: {USB_PORT}")
    print(f"Environment: {ENVIRONMENT}")
    print()
    
    uploader = SimpleJumperlessUploader()
    uploader.run()

if __name__ == "__main__":
    main() 