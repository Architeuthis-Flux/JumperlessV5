#ifndef JUMPERLESS_MODULE_H
#define JUMPERLESS_MODULE_H


# Jumperless MicroPython Module
# This module provides a clean Python API for controlling Jumperless hardware
# Works with: MicroPython REPL, .py scripts on filesystem, and external Python over serial

import sys

# Check if we're running on the actual Jumperless hardware
_on_hardware = True  # Assume we're on hardware by default

class JumperlessError(Exception):
    """Exception raised when Jumperless commands fail"""
    pass

def _execute_command(command):
    """Execute a command on the Jumperless hardware"""
    if _on_hardware:
        # Running on actual hardware - use the EXEC: prefix mechanism
        # This will be intercepted by the C++ mp_hal_stdout_tx_strn_cooked function
        print(f"EXEC:{command}")
        return f"SUCCESS: {command} executed"
    else:
        # Running on external Python - send over serial
        print(f">COMMAND: {command}")
        return "<SUCCESS: Command sent (external mode)"

class DAC:
    """Digital-to-Analog Converter control"""
    
    @staticmethod
    def set(channel, voltage, save=False):
        """Set DAC output voltage
        
        Args:
            channel (int): DAC channel (0 or 1)
            voltage (float): Output voltage
            save (bool): Save to persistent storage
        
        Returns:
            str: Success message
        """
        cmd = f"dac(set, {channel}, {voltage}, save={str(save)})"
        return _execute_command(cmd)
    
    @staticmethod
    def get(channel):
        """Get DAC current setting
        
        Args:
            channel (int): DAC channel (0 or 1)
            
        Returns:
            str: Current DAC setting
        """
        cmd = f"dac(get, {channel})"
        return _execute_command(cmd)

class ADC:
    """Analog-to-Digital Converter control"""
    
    @staticmethod
    def get(channel):
        """Read ADC voltage
        
        Args:
            channel (int): ADC channel (0-4)
            
        Returns:
            str: ADC reading with voltage
        """
        cmd = f"adc(get, {channel})"
        return _execute_command(cmd)
    
    @staticmethod
    def read(channel):
        """Alias for get() - more intuitive name"""
        return ADC.get(channel)

class GPIO:
    """General Purpose Input/Output control"""
    
    HIGH = "HIGH"
    LOW = "LOW"
    OUTPUT = "OUTPUT"
    INPUT = "INPUT"
    
    @staticmethod
    def set(pin, value):
        """Set GPIO pin output
        
        Args:
            pin (int): GPIO pin number (1-10)
            value: Pin value (HIGH, LOW, 1, 0, True, False)
            
        Returns:
            str: Success message
        """
        if isinstance(value, bool):
            value = "HIGH" if value else "LOW"
        elif isinstance(value, int):
            value = "HIGH" if value else "LOW"
        
        cmd = f"gpio(set, {pin}, {value})"
        return _execute_command(cmd)
    
    @staticmethod
    def get(pin):
        """Read GPIO pin state
        
        Args:
            pin (int): GPIO pin number (1-10)
            
        Returns:
            str: Pin state (HIGH/LOW)
        """
        cmd = f"gpio(get, {pin})"
        return _execute_command(cmd)
    
    @staticmethod
    def direction(pin, direction):
        """Set GPIO pin direction
        
        Args:
            pin (int): GPIO pin number (1-10)
            direction (str): Direction (INPUT/OUTPUT)
            
        Returns:
            str: Success message
        """
        cmd = f"gpio(direction, {pin}, {direction})"
        return _execute_command(cmd)

class Nodes:
    """Node connection management"""
    
    @staticmethod
    def connect(node1, node2, save=False):
        """Connect two nodes
        
        Args:
            node1 (int): First node number
            node2 (int): Second node number
            save (bool): Save connection persistently
            
        Returns:
            str: Success message
        """
        cmd = f"nodes(connect, {node1}, {node2}, save={str(save)})"
        return _execute_command(cmd)
    
    @staticmethod
    def disconnect(node1, node2):
        """Disconnect two nodes
        
        Args:
            node1 (int): First node number
            node2 (int): Second node number
            
        Returns:
            str: Success message
        """
        cmd = f"nodes(remove, {node1}, {node2})"
        return _execute_command(cmd)
    
    @staticmethod
    def remove(node1, node2):
        """Alias for disconnect()"""
        return Nodes.disconnect(node1, node2)
    
    @staticmethod
    def clear():
        """Clear all node connections
        
        Returns:
            str: Success message
        """
        cmd = "nodes(clear)"
        return _execute_command(cmd)

class OLED:
    """OLED display control"""
    
    @staticmethod
    def connect():
        """Connect to OLED display
        
        Returns:
            str: Success/error message
        """
        cmd = "oled(connect)"
        return _execute_command(cmd)
    
    @staticmethod
    def disconnect():
        """Disconnect OLED display
        
        Returns:
            str: Success message
        """
        cmd = "oled(disconnect)"
        return _execute_command(cmd)
    
    @staticmethod
    def print(text, size=2):
        """Print text to OLED (clears, prints, shows)
        
        Args:
            text (str): Text to display
            size (int): Text size (1-4)
            
        Returns:
            str: Success message
        """
        cmd = f'oled(print, "{text}", {size})'
        return _execute_command(cmd)
    
    @staticmethod
    def clear():
        """Clear OLED display
        
        Returns:
            str: Success message
        """
        cmd = "oled(clear)"
        return _execute_command(cmd)
    
    @staticmethod
    def show():
        """Update OLED display
        
        Returns:
            str: Success message
        """
        cmd = "oled(show)"
        return _execute_command(cmd)
    
    @staticmethod
    def set_cursor(x, y):
        """Set cursor position
        
        Args:
            x (int): X coordinate
            y (int): Y coordinate
            
        Returns:
            str: Success message
        """
        cmd = f"oled(setCursor, {x}, {y})"
        return _execute_command(cmd)
    
    @staticmethod
    def set_text_size(size):
        """Set text size
        
        Args:
            size (int): Text size (1-4)
            
        Returns:
            str: Success message
        """
        cmd = f"oled(setTextSize, {size})"
        return _execute_command(cmd)
    
    @staticmethod
    def cycle_font():
        """Cycle to next font
        
        Returns:
            str: Success message with font info
        """
        cmd = "oled(cycleFont)"
        return _execute_command(cmd)
    
    @staticmethod
    def set_font(font):
        """Set specific font
        
        Args:
            font: Font name (str) or index (int)
            
        Returns:
            str: Success message
        """
        if isinstance(font, str):
            cmd = f'oled(setFont, "{font}")'
        else:
            cmd = f"oled(setFont, {font})"
        return _execute_command(cmd)
    
    @staticmethod
    def is_connected():
        """Check if OLED is connected
        
        Returns:
            str: Connection status
        """
        cmd = "oled(isConnected)"
        return _execute_command(cmd)

class Arduino:
    """Arduino control functions"""
    
    @staticmethod
    def reset():
        """Reset connected Arduino
        
        Returns:
            str: Success message
        """
        cmd = "arduino(reset)"
        return _execute_command(cmd)
    
    @staticmethod
    def flash():
        """Flash Arduino (if implemented)
        
        Returns:
            str: Success/error message
        """
        cmd = "arduino(flash)"
        return _execute_command(cmd)

class UART:
    """UART connection control"""
    
    @staticmethod
    def connect():
        """Connect UART to Arduino D0/D1
        
        Returns:
            str: Success message
        """
        cmd = "uart(connect)"
        return _execute_command(cmd)
    
    @staticmethod
    def disconnect():
        """Disconnect UART from Arduino D0/D1
        
        Returns:
            str: Success message
        """
        cmd = "uart(disconnect)"
        return _execute_command(cmd)

class INA:
    """INA219 current/voltage/power sensor control"""
    
    @staticmethod
    def get_current(sensor):
        """Get current measurement from INA sensor
        
        Args:
            sensor (int): INA sensor number (0 or 1)
            
        Returns:
            str: Current measurement in mA
        """
        cmd = f"ina(getCurrent, {sensor})"
        return _execute_command(cmd)
    
    @staticmethod
    def get_voltage(sensor):
        """Get shunt voltage measurement from INA sensor
        
        Args:
            sensor (int): INA sensor number (0 or 1)
            
        Returns:
            str: Shunt voltage measurement in mV
        """
        cmd = f"ina(getVoltage, {sensor})"
        return _execute_command(cmd)
    
    @staticmethod
    def get_bus_voltage(sensor):
        """Get bus voltage measurement from INA sensor
        
        Args:
            sensor (int): INA sensor number (0 or 1)
            
        Returns:
            str: Bus voltage measurement in V
        """
        cmd = f"ina(getBusVoltage, {sensor})"
        return _execute_command(cmd)
    
    @staticmethod
    def get_power(sensor):
        """Get power measurement from INA sensor
        
        Args:
            sensor (int): INA sensor number (0 or 1)
            
        Returns:
            str: Power measurement in mW
        """
        cmd = f"ina(getPower, {sensor})"
        return _execute_command(cmd)

class Probe:
    """Probe simulation functions"""
    
    @staticmethod
    def tap(node):
        """Simulate probe tap on node
        
        Args:
            node: Node identifier (int or str)
            
        Returns:
            str: Success message
        """
        cmd = f"probe(tap, {node})"
        return _execute_command(cmd)
    
    @staticmethod
    def click(action="click"):
        """Simulate probe button click
        
        Args:
            action (str): Click type (click, short_click, release)
            
        Returns:
            str: Success message
        """
        cmd = f"probe({action})"
        return _execute_command(cmd)

# Create convenient module-level instances
dac = DAC()
adc = ADC()
ina = INA()
gpio = GPIO()
nodes = Nodes()
oled = OLED()
arduino = Arduino()
uart = UART()
probe = Probe()

# Create main Jumperless namespace object
class JL:
    """Main Jumperless hardware control namespace
    
    Provides organized access to all hardware control modules.
    Use as: jl.dac.set(0, 2.5) or jl.read_voltage(0)
    """
    
    def __init__(self):
        # Hardware control modules
        self.dac = dac
        self.adc = adc
        self.ina = ina
        self.gpio = gpio
        self.nodes = nodes
        self.oled = oled
        self.arduino = arduino
        self.uart = uart
        self.probe = probe
    
    # Convenience methods on the main jl object
    def read_voltage(self, channel):
        """Shortcut to read ADC voltage"""
        return self.adc.get(channel)
    
    def set_voltage(self, channel, voltage, save=False):
        """Shortcut to set DAC voltage"""
        return self.dac.set(channel, voltage, save)
    
    def connect_nodes(self, node1, node2, save=False):
        """Shortcut to connect two nodes"""
        return self.nodes.connect(node1, node2, save)
    
    def display(self, text, size=2):
        """Shortcut to display text on OLED"""
        return self.oled.print(text, size)
    
    def reset_arduino(self):
        """Shortcut to reset Arduino"""
        return self.arduino.reset()

# Create the main jl namespace instance
jl = JL()

# Convenience functions for common operations
def connect_nodes(node1, node2, save=False):
    """Shortcut to connect two nodes"""
    return nodes.connect(node1, node2, save)

def set_voltage(channel, voltage, save=False):
    """Shortcut to set DAC voltage"""
    return dac.set(channel, voltage, save)

def read_voltage(channel):
    """Shortcut to read ADC voltage"""
    return adc.get(channel)

def display(text, size=2):
    """Shortcut to display text on OLED"""
    return oled.print(text, size)

def reset_arduino():
    """Shortcut to reset Arduino"""
    return arduino.reset()

# Module info
__version__ = "1.0.0"
__author__ = "Kevin Santo Cappuccio"
__description__ = "MicroPython module for controlling Jumperless hardware"

def help():
    """Print module help"""
    print("""
Jumperless MicroPython Module v{}

Main Namespace:
- jl       : Main hardware control namespace (recommended)

Hardware Control Modules:
- dac      : Digital-to-Analog Converter control
- adc      : Analog-to-Digital Converter control  
- ina      : INA219 control
- gpio     : General Purpose Input/Output control
- nodes    : Node connection management
- oled     : OLED display control
- arduino  : Arduino control functions
- uart     : UART connection control
- probe    : Probe simulation functions

Quick Examples (using jl namespace):
  jl.dac.set(0, 2.5)        # Set DAC 0 to 2.5V
  jl.read_voltage(0)        # Read ADC channel 0
  jl.ina.get_current(0)     # Read current from INA sensor 0
  jl.gpio.set(5, jl.gpio.HIGH)  # Set GPIO pin 5 HIGH
  jl.nodes.connect(1, 5)    # Connect nodes 1 and 5
  jl.display("Hello!")      # Display text on OLED
  jl.reset_arduino()        # Reset connected Arduino

Direct Access Examples:
  dac.set(0, 2.5)           # Set DAC 0 to 2.5V
  voltage = adc.get(0)      # Read ADC channel 0
  nodes.connect(1, 5)       # Connect nodes 1 and 5

Convenience Functions:
  jl.read_voltage(0)        # Quick ADC reading
  jl.set_voltage(0, 3.3)    # Quick DAC setting
  jl.connect_nodes(1, 5)    # Quick node connection
  jl.display("Hello!")      # Quick OLED display

For detailed help on any class, use: help(classname)
Example: help(jl.dac) or help(dac)
    """.format(__version__))

if __name__ == "__main__":
    help()

#endif // JUMPERLESS_MODULE_H