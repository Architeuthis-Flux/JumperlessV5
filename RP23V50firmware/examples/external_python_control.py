#!/usr/bin/env python3
"""
External Python Control for Jumperless
This script allows controlling Jumperless hardware from a computer over serial

Usage:
  python3 external_python_control.py [PORT] [BAUDRATE]
  
Examples:
  python3 external_python_control.py /dev/ttyUSB0 115200
  python3 external_python_control.py COM3 115200
"""

import serial
import time
import sys
import json
from typing import Union, Optional

class JumperlessSerial:
    """Serial interface for external Python control of Jumperless"""
    
    def __init__(self, port: str = None, baudrate: int = 115200, timeout: float = 1.0):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.serial = None
        self.connected = False
        
        if port:
            self.connect(port, baudrate)
    
    def connect(self, port: str, baudrate: int = 115200) -> bool:
        """Connect to Jumperless over serial"""
        try:
            self.serial = serial.Serial(port, baudrate, timeout=self.timeout)
            time.sleep(2)  # Wait for connection to stabilize
            
            # Test connection
            response = self.execute("help")
            if "SUCCESS" in response or "ERROR" in response:
                self.connected = True
                print(f"Connected to Jumperless on {port}")
                return True
            else:
                print(f"Failed to verify connection to {port}")
                return False
                
        except Exception as e:
            print(f"Failed to connect to {port}: {e}")
            return False
    
    def disconnect(self):
        """Disconnect from serial port"""
        if self.serial and self.serial.is_open:
            self.serial.close()
        self.connected = False
        print("Disconnected from Jumperless")
    
    def execute(self, command: str) -> str:
        """Execute a command on Jumperless and return response"""
        if not self.connected:
            return "ERROR: Not connected to Jumperless"
        
        try:
            # Send command
            self.serial.write(f"{command}\n".encode())
            self.serial.flush()
            
            # Read response (with timeout)
            response = ""
            start_time = time.time()
            
            while time.time() - start_time < self.timeout:
                if self.serial.in_waiting > 0:
                    data = self.serial.read(self.serial.in_waiting).decode('utf-8', errors='ignore')
                    response += data
                    
                    # Check for command completion
                    if "SUCCESS:" in response or "ERROR:" in response:
                        break
                        
                time.sleep(0.01)
            
            return response.strip()
            
        except Exception as e:
            return f"ERROR: Communication failed: {e}"

class ExternalJumperless:
    """Python API for external control of Jumperless hardware"""
    
    def __init__(self, port: str = None, baudrate: int = 115200):
        self.serial = JumperlessSerial(port, baudrate)
        
        # Create API instances
        self.dac = self.DAC(self.serial)
        self.adc = self.ADC(self.serial)
        self.gpio = self.GPIO(self.serial)
        self.nodes = self.Nodes(self.serial)
        self.oled = self.OLED(self.serial)
        self.arduino = self.Arduino(self.serial)
        self.uart = self.UART(self.serial)
        self.probe = self.Probe(self.serial)
    
    def connect(self, port: str, baudrate: int = 115200) -> bool:
        """Connect to Jumperless"""
        return self.serial.connect(port, baudrate)
    
    def disconnect(self):
        """Disconnect from Jumperless"""
        self.serial.disconnect()
    
    def execute(self, command: str) -> str:
        """Execute raw command"""
        return self.serial.execute(command)
    
    class DAC:
        def __init__(self, serial): self.serial = serial
        def set(self, channel: int, voltage: float, save: bool = False) -> str:
            return self.serial.execute(f"dac(set, {channel}, {voltage}, save={str(save)})")
        def get(self, channel: int) -> str:
            return self.serial.execute(f"dac(get, {channel})")
    
    class ADC:
        def __init__(self, serial): self.serial = serial
        def get(self, channel: int) -> str:
            return self.serial.execute(f"adc(get, {channel})")
        def read(self, channel: int) -> str:
            return self.get(channel)
    
    class GPIO:
        HIGH = "HIGH"
        LOW = "LOW"
        OUTPUT = "OUTPUT"
        INPUT = "INPUT"
        
        def __init__(self, serial): self.serial = serial
        def set(self, pin: int, value: Union[str, int, bool]) -> str:
            if isinstance(value, bool):
                value = "HIGH" if value else "LOW"
            elif isinstance(value, int):
                value = "HIGH" if value else "LOW"
            return self.serial.execute(f"gpio(set, {pin}, {value})")
        def get(self, pin: int) -> str:
            return self.serial.execute(f"gpio(get, {pin})")
        def direction(self, pin: int, direction: str) -> str:
            return self.serial.execute(f"gpio(direction, {pin}, {direction})")
    
    class Nodes:
        def __init__(self, serial): self.serial = serial
        def connect(self, node1: int, node2: int, save: bool = False) -> str:
            return self.serial.execute(f"nodes(connect, {node1}, {node2}, save={str(save)})")
        def disconnect(self, node1: int, node2: int) -> str:
            return self.serial.execute(f"nodes(remove, {node1}, {node2})")
        def remove(self, node1: int, node2: int) -> str:
            return self.disconnect(node1, node2)
        def clear(self) -> str:
            return self.serial.execute("nodes(clear)")
    
    class OLED:
        def __init__(self, serial): self.serial = serial
        def connect(self) -> str:
            return self.serial.execute("oled(connect)")
        def disconnect(self) -> str:
            return self.serial.execute("oled(disconnect)")
        def print(self, text: str, size: int = 2) -> str:
            return self.serial.execute(f'oled(print, "{text}", {size})')
        def clear(self) -> str:
            return self.serial.execute("oled(clear)")
        def show(self) -> str:
            return self.serial.execute("oled(show)")
        def set_cursor(self, x: int, y: int) -> str:
            return self.serial.execute(f"oled(setCursor, {x}, {y})")
        def set_text_size(self, size: int) -> str:
            return self.serial.execute(f"oled(setTextSize, {size})")
        def cycle_font(self) -> str:
            return self.serial.execute("oled(cycleFont)")
        def set_font(self, font: Union[str, int]) -> str:
            if isinstance(font, str):
                return self.serial.execute(f'oled(setFont, "{font}")')
            else:
                return self.serial.execute(f"oled(setFont, {font})")
        def is_connected(self) -> str:
            return self.serial.execute("oled(isConnected)")
    
    class Arduino:
        def __init__(self, serial): self.serial = serial
        def reset(self) -> str:
            return self.serial.execute("arduino(reset)")
        def flash(self) -> str:
            return self.serial.execute("arduino(flash)")
    
    class UART:
        def __init__(self, serial): self.serial = serial
        def connect(self) -> str:
            return self.serial.execute("uart(connect)")
        def disconnect(self) -> str:
            return self.serial.execute("uart(disconnect)")
    
    class Probe:
        def __init__(self, serial): self.serial = serial
        def tap(self, node: Union[int, str]) -> str:
            return self.serial.execute(f"probe(tap, {node})")
        def click(self, action: str = "click") -> str:
            return self.serial.execute(f"probe({action})")

# Convenience functions
def auto_detect_port():
    """Try to auto-detect Jumperless serial port"""
    import serial.tools.list_ports
    
    # Common patterns for Jumperless devices
    patterns = ['USB', 'ACM', 'ttyUSB', 'ttyACM', 'COM']
    
    ports = serial.tools.list_ports.comports()
    for port in ports:
        for pattern in patterns:
            if pattern in port.device or pattern in str(port.description):
                print(f"Found potential Jumperless port: {port.device} - {port.description}")
                return port.device
    
    if ports:
        print(f"No obvious Jumperless port found. Available ports:")
        for port in ports:
            print(f"  {port.device} - {port.description}")
        return ports[0].device
    
    return None

def interactive_mode(jl: ExternalJumperless):
    """Interactive command mode"""
    print("\nJumperless Interactive Mode")
    print("Type 'help' for commands, 'quit' to exit")
    
    while True:
        try:
            command = input(">>> ").strip()
            
            if command.lower() in ['quit', 'exit', 'q']:
                break
            elif command.lower() == 'help':
                print("""
Available commands:
  jl.dac.set(0, 2.5)         # Set DAC 0 to 2.5V
  jl.adc.get(0)              # Read ADC channel 0
  jl.gpio.set(5, True)       # Set GPIO 5 HIGH
  jl.nodes.connect(1, 5)     # Connect nodes 1 and 5
  jl.oled.print("Hello!")    # Display on OLED
  jl.arduino.reset()         # Reset Arduino
  
  Or use raw commands:
  dac(set, 0, 2.5)
  nodes(connect, 1, 5, save=False)
                """)
            elif command:
                if command.startswith('jl.'):
                    # Python API command
                    try:
                        result = eval(command)
                        print(result)
                    except Exception as e:
                        print(f"Error: {e}")
                else:
                    # Raw command
                    result = jl.execute(command)
                    print(result)
                    
        except KeyboardInterrupt:
            break
        except EOFError:
            break
    
    print("\nExiting interactive mode")

def run_demo(jl: ExternalJumperless):
    """Run demo sequence"""
    print("\n=== External Python Demo ===")
    
    # Test basic functions
    print("1. Testing DAC...")
    print(jl.dac.set(0, 2.5))
    
    print("2. Testing ADC...")
    print(jl.adc.get(0))
    
    print("3. Testing GPIO...")
    print(jl.gpio.set(5, True))
    print(jl.gpio.get(5))
    
    print("4. Testing nodes...")
    print(jl.nodes.connect(1, 5))
    time.sleep(0.5)
    print(jl.nodes.disconnect(1, 5))
    
    print("5. Testing OLED...")
    print(jl.oled.print("External!", 2))
    
    print("\n=== Demo Complete ===")

def main():
    """Main function"""
    # Parse command line arguments
    port = None
    baudrate = 115200
    
    if len(sys.argv) > 1:
        port = sys.argv[1]
    if len(sys.argv) > 2:
        baudrate = int(sys.argv[2])
    
    # Auto-detect port if not specified
    if not port:
        port = auto_detect_port()
        if not port:
            print("Error: No serial port found. Please specify port manually.")
            print("Usage: python3 external_python_control.py [PORT] [BAUDRATE]")
            return
    
    # Connect to Jumperless
    print(f"Connecting to Jumperless on {port} at {baudrate} baud...")
    jl = ExternalJumperless()
    
    if not jl.connect(port, baudrate):
        print("Failed to connect to Jumperless")
        return
    
    try:
        # Run demo
        run_demo(jl)
        
        # Enter interactive mode
        interactive_mode(jl)
        
    except KeyboardInterrupt:
        print("\nInterrupted by user")
    finally:
        jl.disconnect()

if __name__ == "__main__":
    main() 