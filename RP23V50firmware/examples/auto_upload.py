#!/usr/bin/env python3
"""
Auto Upload Script for Jumperless using PlatformIO
Automatically detects when a Jumperless device is plugged in and uploads firmware using PlatformIO.

Usage:
  python3 auto_upload.py [--watch] [--environment ENV] [--project-dir DIR]
  
Examples:
  python3 auto_upload.py
  python3 auto_upload.py --watch
  python3 auto_upload.py --environment jumperless_v5 --project-dir ../
"""

import serial.tools.list_ports
import time
import sys
import argparse
import threading
import subprocess
import os
from typing import Set, Optional, List
import signal

class JumperlessPlatformIOUploader:
    """Automatic PlatformIO uploader for Jumperless devices"""
    
    def __init__(self, environment: str = "jumperless_v5", project_dir: str = ".", watch_mode: bool = False):
        self.environment = environment
        self.project_dir = os.path.abspath(project_dir)
        self.watch_mode = watch_mode
        self.known_ports: Set[str] = set()
        self.running = True
        self.upload_lock = threading.Lock()
        
        # Verify project directory has platformio.ini
        if not os.path.exists(os.path.join(self.project_dir, "platformio.ini")):
            raise ValueError(f"No platformio.ini found in {self.project_dir}")
        
        # Initialize known ports
        self._scan_existing_ports()
        
    
    def _scan_existing_ports(self):
        """Scan for existing serial ports to establish baseline"""
        try:
            ports = serial.tools.list_ports.comports()
            self.known_ports = {port.device for port in ports}
            print(f"Found {len(self.known_ports)} existing serial ports")
        except Exception as e:
            print(f"Error scanning existing ports: {e}")
    
    def _is_jumperless_port(self, port_info) -> bool:
        """Check if a port looks like a Jumperless device"""
        # Common patterns for Jumperless/Arduino-like devices
        jumperless_patterns = [
            'USB', 'ACM', 'ttyUSB', 'ttyACM', 'COM',
            'Serial', 'Arduino', 'Pico', 'RP2040'
        ]
        
        port_str = f"{port_info.device} {port_info.description} {port_info.manufacturer or ''}"
        
        for pattern in jumperless_patterns:
            if pattern.lower() in port_str.lower():
                return True
        
        return False
    
    def _get_available_ports(self) -> List[str]:
        """Get list of potential Jumperless ports"""
        ports = serial.tools.list_ports.comports()
        jumperless_ports = []
        
        for port_info in ports:
            if self._is_jumperless_port(port_info):
                jumperless_ports.append(port_info.device)
        
        return jumperless_ports
    
    def _run_platformio_command(self, port: str = None) -> bool:
        """Run PlatformIO upload command"""
        with self.upload_lock:
            try:
                print(f"\n↑ Starting PlatformIO upload...")
                if port:
                    print(f"   Target port: {port}")
                print(f"   Environment: {self.environment}")
                print(f"   Project dir: {self.project_dir}")
                
                # Build the PlatformIO command
                cmd = [
                    "platformio", "run",
                    "--target", "upload",
                    "--environment", self.environment,
                    "--project-dir", self.project_dir
                ]
                
                # Add upload port if specified
                if port:
                    cmd.extend(["--upload-port", port])
                
                print(f"   Command: {' '.join(cmd)}")
                print()
                
                # Run the command
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    cwd=self.project_dir
                )
                
                if result.returncode == 0:
                    print("☺ PlatformIO upload completed successfully!")
                    if result.stdout:
                        print("Build output:")
                        print(result.stdout)
                    return True
                else:
                    print("☹ PlatformIO upload failed!")
                    if result.stderr:
                        print("Error output:")
                        print(result.stderr)
                    if result.stdout:
                        print("Build output:")
                        print(result.stdout)
                    return False
                    
            except FileNotFoundError:
                print("❌ PlatformIO not found! Please install PlatformIO:")
                print("   pip install platformio")
                return False
            except Exception as e:
                print(f"❌ Upload failed: {e}")
                return False
    
    def _handle_new_device(self, port: str):
        """Handle a newly detected device"""
        print(f"\n◎ New device detected: {port}")
        
        # Wait a moment for device to settle
        time.sleep(0.5)
        
        # Upload using PlatformIO
        success = self._run_platformio_command(port)
        if success:
            print(f"🎉 Successfully uploaded firmware to {port}")
        else:
            print(f"☹ Failed to upload firmware to {port}")
    
    def _monitor_ports(self):
        """Monitor for new serial ports"""
        print("○ Monitoring for new Jumperless devices...")
        print("Press Ctrl+C to stop")
        
        while self.running:
            try:
                # Get current ports
                current_ports = set()
                ports = serial.tools.list_ports.comports()
                
                for port_info in ports:
                    current_ports.add(port_info.device)
                    
                    # Check for new ports that look like Jumperless devices
                    if (port_info.device not in self.known_ports and 
                        self._is_jumperless_port(port_info)):
                        
                        # Handle new device in a separate thread to avoid blocking
                        device_thread = threading.Thread(
                            target=self._handle_new_device,
                            args=(port_info.device,),
                            daemon=True
                        )
                        device_thread.start()
                
                # Update known ports
                self.known_ports = current_ports
                
                time.sleep(1)  # Check every second
                
            except Exception as e:
                print(f"Error during port monitoring: {e}")
                time.sleep(2)
    
    def run_once(self) -> bool:
        """Run upload once on any detected Jumperless device"""
        print("○ Scanning for Jumperless devices...")
        
        jumperless_ports = self._get_available_ports()
        
        if not jumperless_ports:
            print("☹ No potential Jumperless devices found")
            print("⟐ Trying upload without specific port (PlatformIO will auto-detect)...")
            return self._run_platformio_command()
        
        print(f"Found {len(jumperless_ports)} potential Jumperless device(s): {jumperless_ports}")
        
        if len(jumperless_ports) == 1:
            # Single device - upload to it specifically
            return self._run_platformio_command(jumperless_ports[0])
        else:
            # Multiple devices - let PlatformIO handle port selection
            print("Multiple devices found, letting PlatformIO auto-select port...")
            return self._run_platformio_command()
    
    def run_watch(self):
        """Run in watch mode - continuously monitor for new devices"""
        try:
            self._monitor_ports()
        except KeyboardInterrupt:
            print("\n🛑 Stopping auto-upload monitor...")
            self.running = False

def signal_handler(signum, frame):
    """Handle Ctrl+C gracefully"""
    print("\n🛑 Received interrupt signal, stopping...")
    sys.exit(0)

def check_platformio():
    """Check if PlatformIO is available"""
    try:
        result = subprocess.run(
            ["platformio", "--version"],
            capture_output=True,
            text=True
        )
        if result.returncode == 0:
            print(f"☺ PlatformIO found: {result.stdout.strip()}")
            return True
        else:
            return False
    except FileNotFoundError:
        return False

def main():
    """Main function"""
    parser = argparse.ArgumentParser(description='Auto-upload Jumperless firmware using PlatformIO')
    parser.add_argument('--watch', '-w', action='store_true', 
                       help='Watch mode - continuously monitor for new devices')
    parser.add_argument('--environment', '-e', default='jumperless_v5',
                       help='PlatformIO environment (default: jumperless_v5)')
    parser.add_argument('--project-dir', '-d', default='.',
                       help='PlatformIO project directory (default: current directory)')
    
    args = parser.parse_args()
    
    # Set up signal handler for graceful exit
    signal.signal(signal.SIGINT, signal_handler)
    
    print("○ Jumperless PlatformIO Auto-Upload Script")
    print("=" * 50)
    
    # Check PlatformIO availability
    if not check_platformio():
        print("☹ PlatformIO not found!")
        print("◈ Install PlatformIO with: pip install platformio")
        print("◉ Or visit: https://platformio.org/install")
        sys.exit(1)
    
    try:
        # Create uploader
        uploader = JumperlessPlatformIOUploader(
            environment=args.environment,
            project_dir=args.project_dir,
            watch_mode=args.watch
        )
        
        print(f"Environment: {args.environment}")
        print(f"Project dir: {os.path.abspath(args.project_dir)}")
        print()
        
        if args.watch:
            # Watch mode - continuously monitor
            uploader.run_watch()
        else:
            # One-time upload
            success = uploader.run_once()
            sys.exit(0 if success else 1)
            
    except ValueError as e:
        print(f"☹ Configuration error: {e}")
        sys.exit(1)
    except Exception as e:
        print(f"☹ Unexpected error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main() 