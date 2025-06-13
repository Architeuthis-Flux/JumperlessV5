#ifndef JUMPERLESS_MODULE_H
#define JUMPERLESS_MODULE_H

const char* full_module = R"""(

_on_hardware = True

# Global variables for synchronous execution (must be initialized)
_sync_result_ready = False
_sync_value = ''
_sync_type = ''
_sync_result = 0

def _execute_sync(cmd):
    """Execute a command synchronously and return the actual result"""
    global _sync_result_ready, _sync_value, _sync_type, _sync_result
    _sync_result_ready = False
    print('SYNC_EXEC:' + cmd)
    # Wait briefly for C code to process - try different timing approaches
    try:
        import time
        time.sleep_ms(1)
    except:
        # Fallback: simple busy wait
        for i in range(1000):
            pass
    
    # Check multiple times with small delays
    for attempt in range(10):
        if _sync_result_ready:
            if _sync_type == 'bool':
                return _sync_value == 'True'
            elif _sync_type == 'float':
                return float(_sync_value)
            elif _sync_type == 'int':
                return int(_sync_value)
            elif _sync_type == 'error':
                raise RuntimeError(_sync_value)
            else:
                return _sync_value
        # Small delay between checks
        try:
            import time
            time.sleep_ms(1)
        except:
            for i in range(100):
                pass
    
    # If we get here, sync failed
    raise RuntimeError('Sync execution failed - timeout after 10 attempts')

class JumperlessError(Exception):
    pass

def _execute_command(cmd):
    if _on_hardware:
        # Use synchronous execution that returns actual typed results
        return _execute_sync(cmd)
    else:
        print('>COMMAND: ' + cmd)
        return '<SUCCESS: Command sent (external mode)'

class DAC:
    def set(self, channel, voltage, save=False):
        cmd = 'dac(set, ' + str(channel) + ', ' + str(voltage) + ', save=' + str(save) + ')'
        return _execute_command(cmd)
    def get(self, channel):
        cmd = 'dac(get, ' + str(channel) + ')'
        return _execute_command(cmd)

class ADC:
    def get(self, channel):
        cmd = 'adc(get, ' + str(channel) + ')'
        return _execute_command(cmd)
    def read(self, channel):
        return self.get(channel)

class GPIO:
    def __init__(self):
        self.HIGH = 'HIGH'
        self.LOW = 'LOW'
        self.OUTPUT = 'OUTPUT'
        self.INPUT = 'INPUT'
    def set(self, pin, value):
        if isinstance(value, bool):
            value = 'HIGH' if value else 'LOW'
        elif isinstance(value, int):
            value = 'HIGH' if value else 'LOW'
        cmd = 'gpio(set, ' + str(pin) + ', ' + str(value) + ')'
        return _execute_command(cmd)
    def get(self, pin):
        cmd = 'gpio(get, ' + str(pin) + ')'
        return _execute_command(cmd)
    def direction(self, pin, direction):
        cmd = 'gpio(direction, ' + str(pin) + ', ' + str(direction) + ')'
        return _execute_command(cmd)

class Nodes:
    def connect(self, node1, node2, save=False):
        cmd = 'nodes(connect, ' + str(node1) + ', ' + str(node2) + ', save=' + str(save) + ')'
        return _execute_command(cmd)
    def disconnect(self, node1, node2):
        cmd = 'nodes(remove, ' + str(node1) + ', ' + str(node2) + ')'
        return _execute_command(cmd)
    def remove(self, node1, node2):
        return self.disconnect(node1, node2)
    def clear(self):
        cmd = 'nodes(clear)'
        return _execute_command(cmd)

class OLED:
    def connect(self):
        cmd = 'oled(connect)'
        return _execute_command(cmd)
    def disconnect(self):
        cmd = 'oled(disconnect)'
        return _execute_command(cmd)
    def print(self, text, size=2):
        cmd = 'oled(print, "' + str(text) + '", ' + str(size) + ')'
        return _execute_command(cmd)
    def clear(self):
        cmd = 'oled(clear)'
        return _execute_command(cmd)
    def show(self):
        cmd = 'oled(show)'
        return _execute_command(cmd)
    def set_cursor(self, x, y):
        cmd = 'oled(setCursor, ' + str(x) + ', ' + str(y) + ')'
        return _execute_command(cmd)
    def set_text_size(self, size):
        cmd = 'oled(setTextSize, ' + str(size) + ')'
        return _execute_command(cmd)
    def cycle_font(self):
        cmd = 'oled(cycleFont)'
        return _execute_command(cmd)
    def set_font(self, font):
        if isinstance(font, str):
            cmd = 'oled(setFont, "' + str(font) + '")'
        else:
            cmd = 'oled(setFont, ' + str(font) + ')'
        return _execute_command(cmd)
    def is_connected(self):
        cmd = 'oled(isConnected)'
        return _execute_command(cmd)

class Arduino:
    def reset(self):
        cmd = 'arduino(reset)'
        return _execute_command(cmd)
    def flash(self):
        cmd = 'arduino(flash)'
        return _execute_command(cmd)

class UART:
    def connect(self):
        cmd = 'uart(connect)'
        return _execute_command(cmd)
    def disconnect(self):
        cmd = 'uart(disconnect)'
        return _execute_command(cmd)

class INA:
    def get_current(self, sensor):
        cmd = 'ina(getCurrent, ' + str(sensor) + ')'
        return _execute_command(cmd)
    def get_voltage(self, sensor):
        cmd = 'ina(getVoltage, ' + str(sensor) + ')'
        return _execute_command(cmd)
    def get_bus_voltage(self, sensor):
        cmd = 'ina(getBusVoltage, ' + str(sensor) + ')'
        return _execute_command(cmd)
    def get_power(self, sensor):
        cmd = 'ina(getPower, ' + str(sensor) + ')'
        return _execute_command(cmd)

class Probe:
    def tap(self, node):
        cmd = 'probe(tap, ' + str(node) + ')'
        return _execute_command(cmd)
    def click(self, action='click'):
        cmd = 'probe(' + str(action) + ')'
        return _execute_command(cmd)

class Clickwheel:
    def up(self, clicks=1):
        cmd = 'clickwheel(up, ' + str(clicks) + ')'
        return _execute_command(cmd)
    def down(self, clicks=1):
        cmd = 'clickwheel(down, ' + str(clicks) + ')'
        return _execute_command(cmd)
    def press(self):
        cmd = 'clickwheel(press)'
        return _execute_command(cmd)
    def hold(self, time=0.5):
        cmd = 'clickwheel(hold, time=' + str(time) + ')'
        return _execute_command(cmd)
    def get_press(self):
        cmd = 'clickwheel(get_press)'
        return _execute_command(cmd)

# Create module instances
dac = DAC()
adc = ADC()
ina = INA()
gpio = GPIO()
nodes = Nodes()
oled = OLED()
arduino = Arduino()
uart = UART()
probe = Probe()
clickwheel = Clickwheel()

class JL:
    def __init__(self):
        self.dac = dac
        self.adc = adc
        self.ina = ina
        self.gpio = gpio
        self.nodes = nodes
        self.oled = oled
        self.arduino = arduino
        self.uart = uart
        self.probe = probe
        self.clickwheel = clickwheel
    
    def read_voltage(self, channel):
        return self.adc.get(channel)
    
    def set_voltage(self, channel, voltage, save=False):
        return self.dac.set(channel, voltage, save)
    
    def connect_nodes(self, node1, node2, save=False):
        return self.nodes.connect(node1, node2, save)
    
    def display(self, text, size=2):
        return self.oled.print(text, size)
    
    def reset_arduino(self):
        return self.arduino.reset()
    
    def help(self):
        print('Jumperless MicroPython Module (Full Embedded)\n\r')
        print('Main Namespace: jl.*\n\r')
        print('Hardware Modules:\n\r')
        print('  jl.dac.set(0, 2.5)      # Set DAC voltage')
        print('  jl.adc.get(0)           # Read ADC')
        print('  jl.ina.get_current(0)   # Read INA current')
        print('  jl.gpio.set(5, "HIGH")  # Set GPIO')
        print('  jl.nodes.connect(1, 5)  # Connect nodes')
        print('  jl.oled.print("Hi")     # Display text')
        print('  jl.arduino.reset()      # Reset Arduino')
        print('  jl.uart.connect()       # Connect UART')
        print('  jl.clickwheel.up(1)     # Scroll up')
        print('  jl.clickwheel.down(3)   # Scroll down 3 clicks')
        print('  jl.clickwheel.press()   # Press clickwheel')
        print('  jl.clickwheel.hold(0.5) # Hold for 0.5 seconds')
        print('\n\rConvenience methods:\n\r')
        print('  jl.read_voltage(0)      # Quick ADC read')
        print('  jl.set_voltage(0, 2.5)  # Quick DAC set')
        print('  jl.display("Hello!")    # Quick OLED print')
        print('  jl.connect_nodes(1, 5)  # Quick node connect')
        print('\n\rNew: Return Values & Variables:\n\r')
        print('voltage = jl.adc.get(0)        # Get actual voltage')
        print('current = jl.ina.get_current(0) # Get actual current')
        print('pin_state = jl.gpio.get(5)     # Get actual state')
        print('if jl.gpio.get(1):\n\r\tprint("Pin 1 is HIGH")')
        print('if voltage > 3.0:\n\r\tjl.oled.print("High voltage!")')
        print('')
        return 'Help displayed'

jl = JL()

def help_jumperless():
    return jl.help()

def connect_nodes(node1, node2, save=False):
    return nodes.connect(node1, node2, save)

def set_voltage(channel, voltage, save=False):
    return dac.set(channel, voltage, save)

def read_voltage(channel):
    return adc.get(channel)

def display(text, size=2):
    return oled.print(text, size)

def reset_arduino():
    return arduino.reset()
)""";


#endif