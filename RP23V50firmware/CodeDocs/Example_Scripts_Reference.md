# Example Scripts Reference

Collection of practical, ready-to-use Python scripts for common Jumperless tasks and applications.

## Overview

This reference provides complete, working examples that demonstrate the capabilities of the Jumperless system. All scripts are designed to run directly in the MicroPython REPL or be saved as `.py` files for repeated use.

## Script Categories

- [Basic Examples](#basic-examples)
- [Hardware Testing](#hardware-testing)
- [Measurement and Monitoring](#measurement-and-monitoring)
- [Signal Generation](#signal-generation)
- [Interactive Applications](#interactive-applications)
- [Circuit Templates](#circuit-templates)
- [Advanced Applications](#advanced-applications)
- [Utility Scripts](#utility-scripts)

---

## Basic Examples

### Hello World
Simple introduction to Jumperless control.

```python
import jumperless
import time

# Display welcome message
jumperless.oled_print("Hello Jumperless!")

# Create a simple connection
jumperless.connect("D2", "A0")
print("Connected D2 to A0")

# Set a voltage
jumperless.dac_set(0, 3.3)
print("DAC0 set to 3.3V")

# Read it back
voltage = jumperless.adc_get(0)
print(f"ADC0 reads: {voltage:.3f}V")

# Clean up
jumperless.nodes_clear()
print("All connections cleared")
```

### LED Blink
Classic blinking LED using GPIO.

```python
import jumperless
import time

# Setup
jumperless.gpio_set_dir(1, True)  # Set GPIO 1 as output
jumperless.connect("GPIO_1", "10")  # Connect to breadboard hole 10

print("Blinking LED on GPIO 1...")
print("Connect LED from hole 10 to GND with current limiting resistor")

# Blink 10 times
for i in range(10):
    jumperless.gpio_set(1, True)   # LED on
    time.sleep(0.5)
    jumperless.gpio_set(1, False)  # LED off
    time.sleep(0.5)
    print(f"Blink {i+1}")

print("Blinking complete")
```

### Button and LED
Interactive button control with LED feedback.

```python
import jumperless
import time

# Setup button input
jumperless.gpio_set_dir(1, False)  # GPIO 1 as input
jumperless.gpio_set_pull(1, 1)     # Enable pull-up
jumperless.connect("GPIO_1", "15") # Connect button to hole 15

# Setup LED output
jumperless.gpio_set_dir(2, True)   # GPIO 2 as output
jumperless.connect("GPIO_2", "10") # Connect LED to hole 10

print("Button and LED Demo")
print("Connect button from hole 15 to GND")
print("Connect LED from hole 10 to GND with resistor")
print("Press Ctrl+C to exit")

led_state = False

try:
    while True:
        # Read button (LOW when pressed due to pull-up)
        button_pressed = jumperless.gpio_get(1) == "LOW"
        
        if button_pressed:
            led_state = not led_state  # Toggle LED
            jumperless.gpio_set(2, led_state)
            print(f"Button pressed! LED {'ON' if led_state else 'OFF'}")
            time.sleep(0.3)  # Debounce delay
        
        time.sleep(0.1)
        
except KeyboardInterrupt:
    print("\nDemo stopped")
    jumperless.gpio_set(2, False)  # Turn off LED
```

---

## Hardware Testing

### System Diagnostics
Comprehensive hardware test suite.

```python
import jumperless
import time

def test_dacs():
    """Test all DAC channels"""
    print("=== Testing DACs ===")
    test_voltages = [0.0, 1.0, 2.5, 3.3, -1.0, -2.5]
    
    for channel in range(4):
        print(f"Testing DAC {channel}:")
        for voltage in test_voltages:
            try:
                jumperless.dac_set(channel, voltage)
                actual = jumperless.dac_get(channel)
                error = abs(actual - voltage)
                status = "PASS" if error < 0.1 else "FAIL"
                print(f"  {voltage:+5.1f}V -> {actual:+5.1f}V [{status}]")
                time.sleep(0.1)
            except Exception as e:
                print(f"  {voltage:+5.1f}V -> ERROR: {e}")
        print()

def test_adcs():
    """Test all ADC channels"""
    print("=== Testing ADCs ===")
    for channel in range(5):
        try:
            voltage = jumperless.adc_get(channel)
            print(f"ADC {channel}: {voltage:+6.3f}V")
        except Exception as e:
            print(f"ADC {channel}: ERROR - {e}")
    print()

def test_gpio():
    """Test GPIO pins"""
    print("=== Testing GPIO ===")
    for pin in range(1, 9):
        try:
            # Test as output
            jumperless.gpio_set_dir(pin, True)
            jumperless.gpio_set(pin, True)
            state_high = jumperless.gpio_get(pin)
            
            jumperless.gpio_set(pin, False)
            state_low = jumperless.gpio_get(pin)
            
            # Test as input with pull-up
            jumperless.gpio_set_dir(pin, False)
            jumperless.gpio_set_pull(pin, 1)
            state_pulled = jumperless.gpio_get(pin)
            
            print(f"GPIO {pin}: OUT_H={state_high}, OUT_L={state_low}, IN_PU={state_pulled}")
            
        except Exception as e:
            print(f"GPIO {pin}: ERROR - {e}")
    print()

def test_current_sensors():
    """Test current monitoring"""
    print("=== Testing Current Sensors ===")
    for sensor in range(2):
        try:
            current = jumperless.ina_get_current(sensor)
            voltage = jumperless.ina_get_bus_voltage(sensor)
            power = jumperless.ina_get_power(sensor)
            print(f"INA {sensor}: {current:+7.3f}A, {voltage:+6.2f}V, {power:+7.3f}W")
        except Exception as e:
            print(f"INA {sensor}: ERROR - {e}")
    print()

def test_connections():
    """Test connection system"""
    print("=== Testing Connections ===")
    test_pairs = [
        ("1", "2"),
        ("D2", "A0"),
        ("GPIO_1", "10"),
        ("GND", "30")
    ]
    
    for node1, node2 in test_pairs:
        try:
            # Connect
            jumperless.connect(node1, node2)
            connected = jumperless.is_connected(node1, node2)
            
            # Disconnect
            jumperless.disconnect(node1, node2)
            disconnected = not jumperless.is_connected(node1, node2)
            
            status = "PASS" if connected and disconnected else "FAIL"
            print(f"{node1:>8} <-> {node2:<8}: {status}")
            
        except Exception as e:
            print(f"{node1:>8} <-> {node2:<8}: ERROR - {e}")
    print()

# Run all tests
def run_diagnostics():
    print("Jumperless Hardware Diagnostics")
    print("=" * 40)
    
    test_dacs()
    test_adcs()
    test_gpio()
    test_current_sensors()
    test_connections()
    
    print("Diagnostics complete!")

# Execute tests
run_diagnostics()
```

### Loopback Test
Test DAC to ADC signal path.

```python
import jumperless
import time

def dac_adc_loopback_test():
    """Test DAC output accuracy using ADC loopback"""
    print("DAC-ADC Loopback Test")
    print("Connecting DAC0 to ADC0...")
    
    # Create loopback connection
    jumperless.connect("DAC0", "ADC0")
    time.sleep(0.1)  # Settle time
    
    test_voltages = [-5.0, -2.5, -1.0, 0.0, 1.0, 2.5, 3.3, 5.0]
    
    print("Voltage    DAC Set    ADC Read   Error")
    print("-" * 40)
    
    max_error = 0.0
    
    for voltage in test_voltages:
        # Set DAC voltage
        jumperless.dac_set(0, voltage)
        time.sleep(0.05)  # Settling time
        
        # Read back from ADC
        measured = jumperless.adc_get(0)
        error = abs(measured - voltage)
        max_error = max(max_error, error)
        
        print(f"{voltage:+6.1f}V   {voltage:+6.1f}V   {measured:+6.3f}V   {error:+6.3f}V")
    
    print("-" * 40)
    print(f"Maximum error: {max_error:.3f}V")
    
    if max_error < 0.1:
        print("✓ PASS: Loopback test successful")
    else:
        print("✗ FAIL: High error detected")
    
    # Clean up
    jumperless.disconnect("DAC0", "ADC0")

dac_adc_loopback_test()
```

---

## Measurement and Monitoring

### Voltage Logger
Log voltages to display trends.

```python
import jumperless
import time

def voltage_logger(channel=0, duration=60, interval=1.0):
    """Log voltage measurements over time"""
    print(f"Voltage Logger - ADC Channel {channel}")
    print(f"Duration: {duration}s, Interval: {interval}s")
    print("Time(s)   Voltage(V)")
    print("-" * 20)
    
    start_time = time.time()
    measurements = []
    
    while time.time() - start_time < duration:
        elapsed = time.time() - start_time
        voltage = jumperless.adc_get(channel)
        measurements.append((elapsed, voltage))
        
        print(f"{elapsed:6.1f}    {voltage:+8.3f}")
        time.sleep(interval)
    
    # Calculate statistics
    voltages = [v for t, v in measurements]
    avg_voltage = sum(voltages) / len(voltages)
    min_voltage = min(voltages)
    max_voltage = max(voltages)
    
    print("-" * 20)
    print(f"Average: {avg_voltage:+8.3f}V")
    print(f"Minimum: {min_voltage:+8.3f}V")
    print(f"Maximum: {max_voltage:+8.3f}V")
    print(f"Range:   {max_voltage - min_voltage:8.3f}V")
    
    return measurements

# Log voltage for 30 seconds
data = voltage_logger(0, 30, 0.5)
```

### Power Monitor
Real-time power consumption monitoring.

```python
import jumperless
import time

def power_monitor(sensor=0, duration=30):
    """Monitor power consumption with statistics"""
    print(f"Power Monitor - Sensor {sensor}")
    print(f"Monitoring for {duration} seconds...")
    print()
    print("Time    Current   Voltage   Power")
    print("-" * 35)
    
    start_time = time.time()
    measurements = []
    
    while time.time() - start_time < duration:
        elapsed = time.time() - start_time
        
        current = jumperless.ina_get_current(sensor)
        voltage = jumperless.ina_get_bus_voltage(sensor)
        power = jumperless.ina_get_power(sensor)
        
        measurements.append({
            'time': elapsed,
            'current': current,
            'voltage': voltage,
            'power': power
        })
        
        print(f"{elapsed:4.0f}s   {current:+7.3f}A   {voltage:6.2f}V   {power:+7.3f}W")
        time.sleep(1)
    
    # Calculate statistics
    currents = [m['current'] for m in measurements]
    voltages = [m['voltage'] for m in measurements]
    powers = [m['power'] for m in measurements]
    
    print("-" * 35)
    print("STATISTICS:")
    print(f"Average Current: {sum(currents)/len(currents):+7.3f}A")
    print(f"Peak Current:    {max(currents):+7.3f}A")
    print(f"Average Voltage: {sum(voltages)/len(voltages):6.2f}V")
    print(f"Average Power:   {sum(powers)/len(powers):+7.3f}W")
    print(f"Peak Power:      {max(powers):+7.3f}W")
    
    # Energy calculation (Wh)
    energy_wh = sum(powers) * (1/3600)  # Convert to Wh (1 sample per second)
    print(f"Energy Used:     {energy_wh:7.3f}Wh")
    
    return measurements

# Monitor power for 30 seconds
data = power_monitor(0, 30)
```

### Multi-Channel Scanner
Scan multiple ADC channels continuously.

```python
import jumperless
import time

def multi_channel_scanner(channels=[0, 1, 2], scan_rate=2.0):
    """Continuously scan multiple ADC channels"""
    print(f"Multi-Channel Scanner")
    print(f"Channels: {channels}, Rate: {scan_rate} scans/sec")
    print("Press Ctrl+C to stop")
    print()
    
    # Create header
    header = "Time(s) "
    for ch in channels:
        header += f"  ADC{ch}(V)  "
    print(header)
    print("-" * len(header))
    
    start_time = time.time()
    scan_interval = 1.0 / scan_rate
    
    try:
        while True:
            elapsed = time.time() - start_time
            
            # Read all channels
            voltages = []
            for ch in channels:
                voltage = jumperless.adc_get(ch)
                voltages.append(voltage)
            
            # Display results
            line = f"{elapsed:6.1f}  "
            for voltage in voltages:
                line += f"{voltage:+8.3f}  "
            print(line)
            
            time.sleep(scan_interval)
            
    except KeyboardInterrupt:
        print("\nScanning stopped")

# Scan channels 0, 1, 2 at 1 Hz
multi_channel_scanner([0, 1, 2], 1.0)
```

---

## Signal Generation

### Sine Wave Generator
Generate smooth sine waves using DAC.

```python
import jumperless
import math
import time

def sine_wave_generator(channel=0, frequency=1.0, amplitude=2.0, offset=0.0, duration=10.0):
    """Generate sine wave on DAC output"""
    print(f"Sine Wave Generator")
    print(f"Channel: DAC{channel}")
    print(f"Frequency: {frequency} Hz")
    print(f"Amplitude: ±{amplitude} V")
    print(f"Offset: {offset} V")
    print(f"Duration: {duration} s")
    
    # Calculate timing
    samples_per_cycle = 50  # Smooth wave
    sample_rate = frequency * samples_per_cycle
    sample_interval = 1.0 / sample_rate
    total_samples = int(duration * sample_rate)
    
    print(f"Sample rate: {sample_rate:.1f} Sa/s")
    print("Generating wave...")
    
    start_time = time.time()
    
    for i in range(total_samples):
        # Calculate sine value
        angle = 2 * math.pi * i / samples_per_cycle
        sine_value = math.sin(angle)
        voltage = offset + amplitude * sine_value
        
        # Clamp to safe range
        voltage = max(-8.0, min(8.0, voltage))
        
        # Set DAC
        jumperless.dac_set(channel, voltage)
        
        # Maintain timing
        next_time = start_time + (i + 1) * sample_interval
        while time.time() < next_time:
            pass  # Busy wait for precision
    
    # Return to offset
    jumperless.dac_set(channel, offset)
    print("Wave generation complete")

# Generate 2Hz sine wave for 5 seconds
sine_wave_generator(0, 2.0, 1.5, 0.0, 5.0)
```

### PWM Examples
Various PWM applications.

```python
import jumperless
import time

def led_fade_demo(pin=1):
    """Smooth LED fade using PWM"""
    print(f"LED Fade Demo on GPIO {pin}")
    
    # Setup
    jumperless.gpio_set_dir(pin, True)
    jumperless.connect(f"GPIO_{pin}", "10")
    
    print("Fading LED up and down...")
    
    # Fade up
    for brightness in range(0, 101, 2):
        duty = brightness / 100.0
        jumperless.pwm(pin, 1000, duty)
        time.sleep(0.05)
    
    # Fade down
    for brightness in range(100, -1, -2):
        duty = brightness / 100.0
        jumperless.pwm(pin, 1000, duty)
        time.sleep(0.05)
    
    jumperless.pwm_stop(pin)
    print("Fade complete")

def servo_demo(pin=2):
    """Servo motor control demonstration"""
    print(f"Servo Demo on GPIO {pin}")
    
    def set_servo_angle(angle):
        # Convert angle (0-180°) to pulse width (1-2ms)
        pulse_width = 1.0 + (angle / 180.0)  # 1.0-2.0ms
        duty_cycle = pulse_width / 20.0      # 20ms period
        jumperless.pwm(pin, 50, duty_cycle)  # 50Hz
    
    jumperless.connect(f"GPIO_{pin}", "15")
    
    angles = [0, 45, 90, 135, 180, 90, 45, 0]
    
    for angle in angles:
        print(f"Moving to {angle}°")
        set_servo_angle(angle)
        time.sleep(1)
    
    jumperless.pwm_stop(pin)
    print("Servo demo complete")

def tone_generator(pin=3, frequencies=[440, 523, 659, 784]):
    """Generate audio tones"""
    print(f"Tone Generator on GPIO {pin}")
    
    jumperless.connect(f"GPIO_{pin}", "20")
    
    for freq in frequencies:
        print(f"Playing {freq} Hz")
        jumperless.pwm(pin, freq, 0.5)  # 50% duty cycle
        time.sleep(0.5)
    
    jumperless.pwm_stop(pin)
    print("Tone generation complete")

# Run PWM demos
led_fade_demo(1)
time.sleep(1)
servo_demo(2)
time.sleep(1)
tone_generator(3, [262, 294, 330, 349, 392, 440, 494, 523])  # C major scale
```

### Arbitrary Waveform Generator
Generate custom waveforms from data.

```python
import jumperless
import time
import math

def triangle_wave(samples=50):
    """Generate triangle wave data"""
    wave = []
    for i in range(samples):
        if i < samples // 2:
            # Rising edge
            value = 2.0 * i / (samples // 2) - 1.0
        else:
            # Falling edge
            value = 1.0 - 2.0 * (i - samples // 2) / (samples // 2)
        wave.append(value)
    return wave

def sawtooth_wave(samples=50):
    """Generate sawtooth wave data"""
    wave = []
    for i in range(samples):
        value = 2.0 * i / samples - 1.0
        wave.append(value)
    return wave

def square_wave(samples=50, duty_cycle=0.5):
    """Generate square wave data"""
    wave = []
    high_samples = int(samples * duty_cycle)
    for i in range(samples):
        value = 1.0 if i < high_samples else -1.0
        wave.append(value)
    return wave

def arbitrary_waveform_generator(channel=0, waveform_data=None, frequency=1.0, 
                                amplitude=2.0, offset=0.0, cycles=5):
    """Generate arbitrary waveform from data array"""
    if waveform_data is None:
        waveform_data = [math.sin(2 * math.pi * i / 50) for i in range(50)]
    
    print(f"Arbitrary Waveform Generator")
    print(f"Channel: DAC{channel}")
    print(f"Waveform samples: {len(waveform_data)}")
    print(f"Frequency: {frequency} Hz")
    print(f"Amplitude: ±{amplitude} V")
    print(f"Offset: {offset} V")
    print(f"Cycles: {cycles}")
    
    # Calculate timing
    samples_per_cycle = len(waveform_data)
    sample_rate = frequency * samples_per_cycle
    sample_interval = 1.0 / sample_rate
    total_samples = cycles * samples_per_cycle
    
    print("Generating waveform...")
    
    start_time = time.time()
    
    for i in range(total_samples):
        # Get waveform value (-1.0 to +1.0)
        wave_index = i % samples_per_cycle
        wave_value = waveform_data[wave_index]
        
        # Scale and offset
        voltage = offset + amplitude * wave_value
        voltage = max(-8.0, min(8.0, voltage))  # Clamp
        
        # Set DAC
        jumperless.dac_set(channel, voltage)
        
        # Maintain timing
        next_time = start_time + (i + 1) * sample_interval
        while time.time() < next_time:
            pass
    
    jumperless.dac_set(channel, offset)
    print("Waveform generation complete")

# Generate different waveforms
print("Triangle wave:")
arbitrary_waveform_generator(0, triangle_wave(), 1.0, 1.5, 0.0, 3)

time.sleep(2)

print("\nSawtooth wave:")
arbitrary_waveform_generator(0, sawtooth_wave(), 2.0, 1.0, 0.0, 3)

time.sleep(2)

print("\nSquare wave (25% duty):")
arbitrary_waveform_generator(0, square_wave(50, 0.25), 1.5, 2.0, 0.0, 3)
```

---

## Interactive Applications

### Voltmeter with Probe
Interactive voltmeter using the probe interface.

```python
import jumperless
import time

def probe_voltmeter():
    """Interactive voltmeter using probe"""
    print("Probe Voltmeter")
    print("Touch any pad with the probe to measure voltage")
    print("Press Ctrl+C to exit")
    print()
    
    # Storage for measurements
    measurements = {}
    
    try:
        while True:
            print("Waiting for probe touch...")
            
            # Wait for probe touch
            pad = jumperless.probe_read()
            print(f"Probe touched: {pad}")
            
            # Temporarily connect to ADC
            jumperless.connect(str(pad), "ADC0")
            time.sleep(0.1)  # Allow settling
            
            # Take measurement
            voltage = jumperless.adc_get(0)
            measurements[str(pad)] = voltage
            
            print(f"Voltage at {pad}: {voltage:+7.3f}V")
            
            # Disconnect
            jumperless.disconnect(str(pad), "ADC0")
            
            # Show summary
            print("\nMeasurement Summary:")
            for pad_name, volt in measurements.items():
                print(f"  {pad_name:>10}: {volt:+7.3f}V")
            print()
            
    except KeyboardInterrupt:
        print("\nVoltmeter stopped")
        
        # Final summary
        if measurements:
            print("\nFinal Results:")
            for pad_name, volt in measurements.items():
                print(f"  {pad_name:>10}: {volt:+7.3f}V")

probe_voltmeter()
```

### Circuit Builder
Interactive circuit building with probe.

```python
import jumperless
import time

def interactive_circuit_builder():
    """Build circuits interactively with the probe"""
    print("Interactive Circuit Builder")
    print("Use the probe to create connections")
    print("Commands:")
    print("  - Touch two pads to connect them")
    print("  - Press CONNECT button to confirm connection")
    print("  - Press REMOVE button to remove last connection")
    print("  - Press Ctrl+C to exit")
    print()
    
    connections = []
    pending_connection = None
    
    try:
        while True:
            # Check for probe touch
            pad = jumperless.probe_read(blocking=False)
            if pad:
                if pending_connection is None:
                    pending_connection = str(pad)
                    print(f"First point: {pad}")
                    print("Touch second point...")
                else:
                    second_pad = str(pad)
                    print(f"Second point: {pad}")
                    print(f"Ready to connect {pending_connection} to {second_pad}")
                    print("Press CONNECT button to confirm, or touch new pad to cancel")
                    
                    # Store pending connection
                    pending_connection = (pending_connection, second_pad)
            
            # Check for button press
            button = jumperless.probe_button(blocking=False)
            if button == "CONNECT" and pending_connection:
                if isinstance(pending_connection, tuple):
                    node1, node2 = pending_connection
                    try:
                        jumperless.connect(node1, node2)
                        connections.append((node1, node2))
                        print(f"✓ Connected {node1} to {node2}")
                    except Exception as e:
                        print(f"✗ Connection failed: {e}")
                else:
                    print("Need two points to connect")
                
                pending_connection = None
                
            elif button == "REMOVE" and connections:
                node1, node2 = connections.pop()
                jumperless.disconnect(node1, node2)
                print(f"✗ Removed connection {node1} to {node2}")
            
            # Show current connections
            if connections:
                print(f"\nActive connections ({len(connections)}):")
                for i, (n1, n2) in enumerate(connections, 1):
                    print(f"  {i}: {n1} <-> {n2}")
                print()
            
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\nCircuit builder stopped")
        print(f"Final circuit has {len(connections)} connections")

interactive_circuit_builder()
```

### Logic Analyzer
Simple logic analyzer for digital signals.

```python
import jumperless
import time

def simple_logic_analyzer(pins=[1, 2, 3], sample_rate=100, duration=5):
    """Simple logic analyzer for GPIO pins"""
    print(f"Simple Logic Analyzer")
    print(f"Pins: GPIO {pins}")
    print(f"Sample rate: {sample_rate} Hz")
    print(f"Duration: {duration} seconds")
    print()
    
    # Setup pins as inputs
    for pin in pins:
        jumperless.gpio_set_dir(pin, False)  # Input
        jumperless.gpio_set_pull(pin, 0)     # No pull resistor
    
    # Calculate timing
    sample_interval = 1.0 / sample_rate
    total_samples = int(duration * sample_rate)
    
    print("Starting capture...")
    print("Time(s)  " + "  ".join([f"GPIO{p}" for p in pins]))
    print("-" * (10 + len(pins) * 7))
    
    start_time = time.time()
    samples = []
    
    for i in range(total_samples):
        elapsed = time.time() - start_time
        
        # Read all pins
        states = []
        for pin in pins:
            state = jumperless.gpio_get(pin)
            states.append("1" if state == "HIGH" else "0")
        
        samples.append((elapsed, states))
        
        # Display every 10th sample
        if i % (sample_rate // 10) == 0:
            print(f"{elapsed:6.2f}   " + "    ".join(states))
        
        # Wait for next sample
        next_time = start_time + (i + 1) * sample_interval
        while time.time() < next_time:
            pass
    
    print("\nCapture complete!")
    
    # Analyze transitions
    print("\nTransition Analysis:")
    for pin_idx, pin in enumerate(pins):
        transitions = 0
        last_state = samples[0][1][pin_idx]
        
        for _, states in samples[1:]:
            if states[pin_idx] != last_state:
                transitions += 1
                last_state = states[pin_idx]
        
        print(f"GPIO {pin}: {transitions} transitions")
    
    return samples

# Analyze GPIO 1, 2, 3 for 10 seconds at 50 Hz
data = simple_logic_analyzer([1, 2, 3], 50, 10)
```

---

## Circuit Templates

### Arduino Development Setup
Standard connections for Arduino development.

```python
import jumperless

def setup_arduino_development():
    """Setup standard Arduino development connections"""
    print("Setting up Arduino Development Environment")
    
    # Clear existing connections
    jumperless.nodes_clear()
    
    # Power distribution
    print("Setting up power rails...")
    jumperless.dac_set(2, 5.0)   # TOP_RAIL to 5V
    jumperless.dac_set(3, 0.0)   # BOTTOM_RAIL to GND
    
    # Connect power to breadboard
    jumperless.connect("TOP_RAIL", "1")     # 5V to hole 1
    jumperless.connect("BOTTOM_RAIL", "30") # GND to hole 30
    jumperless.connect("GND", "31")         # Additional GND
    
    # I2C connections (standard pins)
    print("Setting up I2C...")
    jumperless.connect("A4", "20")  # SDA
    jumperless.connect("A5", "21")  # SCL
    
    # SPI connections
    print("Setting up SPI...")
    jumperless.connect("D10", "10") # SS
    jumperless.connect("D11", "11") # MOSI
    jumperless.connect("D12", "12") # MISO
    jumperless.connect("D13", "13") # SCK
    
    # UART connections (if using software serial)
    print("Setting up UART...")
    jumperless.connect("D2", "25")  # Software Serial RX
    jumperless.connect("D3", "26")  # Software Serial TX
    
    # Analog connections
    print("Setting up analog pins...")
    jumperless.connect("A0", "40")
    jumperless.connect("A1", "41")
    jumperless.connect("A2", "42")
    jumperless.connect("A3", "43")
    
    print("Arduino development setup complete!")
    print("Available connections:")
    print("  Power: 5V on hole 1, GND on holes 30-31")
    print("  I2C: SDA on hole 20, SCL on hole 21")
    print("  SPI: SS=10, MOSI=11, MISO=12, SCK=13")
    print("  UART: RX on hole 25, TX on hole 26")
    print("  Analog: A0-A3 on holes 40-43")

setup_arduino_development()
```

### Sensor Testing Platform
Setup for testing various sensors.

```python
import jumperless

def setup_sensor_platform():
    """Setup platform for sensor testing"""
    print("Setting up Sensor Testing Platform")
    
    # Clear existing connections
    jumperless.nodes_clear()
    
    # Multi-voltage power supply
    print("Setting up power supplies...")
    jumperless.dac_set(0, 3.3)   # 3.3V supply
    jumperless.dac_set(1, 5.0)   # 5V supply
    jumperless.dac_set(2, 3.3)   # TOP_RAIL 3.3V
    jumperless.dac_set(3, 0.0)   # BOTTOM_RAIL GND
    
    # Power distribution
    jumperless.connect("DAC0", "1")     # 3.3V on hole 1
    jumperless.connect("DAC1", "2")     # 5.0V on hole 2
    jumperless.connect("GND", "30")     # GND on hole 30
    
    # I2C sensor bus
    print("Setting up I2C sensor bus...")
    jumperless.connect("A4", "20")      # SDA
    jumperless.connect("A5", "21")      # SCL
    jumperless.connect("DAC0", "22")    # 3.3V pullup power
    
    # SPI sensor bus
    print("Setting up SPI sensor bus...")
    jumperless.connect("D11", "10")     # MOSI
    jumperless.connect("D12", "11")     # MISO
    jumperless.connect("D13", "12")     # SCK
    jumperless.connect("D10", "13")     # CS1
    jumperless.connect("D9", "14")      # CS2
    
    # Analog sensor inputs
    print("Setting up analog inputs...")
    jumperless.connect("A0", "40")      # Analog sensor 1
    jumperless.connect("A1", "41")      # Analog sensor 2
    jumperless.connect("A2", "42")      # Analog sensor 3
    
    # Digital sensor inputs
    print("Setting up digital inputs...")
    jumperless.connect("D2", "50")      # Digital sensor 1
    jumperless.connect("D3", "51")      # Digital sensor 2
    jumperless.connect("D4", "52")      # Digital sensor 3
    
    # Reference voltages for sensor testing
    print("Setting up reference voltages...")
    jumperless.connect("ADC0", "45")    # Voltage measurement point 1
    jumperless.connect("ADC1", "46")    # Voltage measurement point 2
    
    print("Sensor testing platform ready!")
    print("Power supplies:")
    print("  3.3V: hole 1, hole 22 (I2C pullup)")
    print("  5.0V: hole 2")
    print("  GND:  hole 30")
    print("I2C bus: SDA=20, SCL=21")
    print("SPI bus: MOSI=10, MISO=11, SCK=12, CS1=13, CS2=14")
    print("Analog inputs: A0=40, A1=41, A2=42")
    print("Digital inputs: D2=50, D3=51, D4=52")
    print("Voltage monitoring: ADC0=45, ADC1=46")

setup_sensor_platform()
```

### Signal Generator Setup
Multi-channel signal generation platform.

```python
import jumperless

def setup_signal_generator():
    """Setup multi-channel signal generator"""
    print("Setting up Signal Generator Platform")
    
    # Clear existing connections
    jumperless.nodes_clear()
    
    # Analog outputs
    print("Setting up analog outputs...")
    jumperless.connect("DAC0", "10")    # Analog out 1
    jumperless.connect("DAC1", "11")    # Analog out 2
    
    # Digital/PWM outputs
    print("Setting up digital/PWM outputs...")
    jumperless.connect("GPIO_1", "20")  # PWM/Digital out 1
    jumperless.connect("GPIO_2", "21")  # PWM/Digital out 2
    jumperless.connect("GPIO_3", "22")  # PWM/Digital out 3
    jumperless.connect("GPIO_4", "23")  # PWM/Digital out 4
    
    # Reference and monitoring
    print("Setting up monitoring...")
    jumperless.connect("ADC0", "15")    # Monitor point 1
    jumperless.connect("ADC1", "16")    # Monitor point 2
    jumperless.connect("GND", "30")     # Ground reference
    
    # Initial setup - all outputs off
    jumperless.dac_set(0, 0.0)
    jumperless.dac_set(1, 0.0)
    
    for gpio in range(1, 5):
        jumperless.gpio_set_dir(gpio, True)  # Set as output
        jumperless.gpio_set(gpio, False)     # Set low
    
    print("Signal generator platform ready!")
    print("Analog outputs: DAC0=10, DAC1=11")
    print("PWM/Digital outputs: GPIO1=20, GPIO2=21, GPIO3=22, GPIO4=23")
    print("Monitor points: ADC0=15, ADC1=16")
    print("Ground reference: hole 30")

def demo_signal_generator():
    """Demonstrate signal generator capabilities"""
    print("\nSignal Generator Demo")
    
    # Test analog outputs
    print("Testing analog outputs...")
    for voltage in [1.0, 2.0, 3.3, 0.0]:
        jumperless.dac_set(0, voltage)
        jumperless.dac_set(1, -voltage if voltage > 0 else 0)
        print(f"DAC0: {voltage}V, DAC1: {-voltage if voltage > 0 else 0}V")
        time.sleep(1)
    
    # Test PWM outputs
    print("Testing PWM outputs...")
    for gpio in range(1, 5):
        frequency = 1000 * gpio  # Different frequency for each pin
        jumperless.pwm(gpio, frequency, 0.5)
        print(f"GPIO{gpio}: {frequency}Hz PWM")
        time.sleep(1)
        jumperless.pwm_stop(gpio)
    
    print("Demo complete!")

setup_signal_generator()
demo_signal_generator()
```

---

## Advanced Applications

### Automated Test Equipment
Framework for automated component testing.

```python
import jumperless
import time

class AutomatedTester:
    def __init__(self):
        self.results = []
        self.setup_test_environment()
    
    def setup_test_environment(self):
        """Setup connections for testing"""
        jumperless.nodes_clear()
        
        # Power supplies
        jumperless.dac_set(0, 3.3)  # 3.3V supply
        jumperless.dac_set(1, 5.0)  # 5V supply
        jumperless.connect("DAC0", "1")
        jumperless.connect("DAC1", "2")
        jumperless.connect("GND", "30")
        
        # Test points
        jumperless.connect("ADC0", "10")  # Voltage measurement
        jumperless.connect("ADC1", "11")  # Current measurement
        
        print("Test environment ready")
    
    def test_resistor(self, test_voltage=3.3, expected_resistance=1000):
        """Test resistor value"""
        print(f"Testing resistor (expected: {expected_resistance}Ω)")
        
        # Apply test voltage across resistor
        # Assumes resistor connected between holes 1 and 10
        jumperless.dac_set(0, test_voltage)
        time.sleep(0.1)  # Settle
        
        # Measure voltage across resistor
        measured_voltage = jumperless.adc_get(0)
        
        # Calculate current (assuming known series resistor)
        # This is simplified - real implementation would use current sensor
        voltage_drop = test_voltage - measured_voltage
        current = voltage_drop / 1000  # Assuming 1kΩ series resistor
        
        if current > 0:
            measured_resistance = measured_voltage / current
        else:
            measured_resistance = float('inf')
        
        error_percent = abs(measured_resistance - expected_resistance) / expected_resistance * 100
        
        result = {
            'component': 'resistor',
            'expected': expected_resistance,
            'measured': measured_resistance,
            'error_percent': error_percent,
            'pass': error_percent < 10  # 10% tolerance
        }
        
        self.results.append(result)
        
        print(f"  Expected: {expected_resistance}Ω")
        print(f"  Measured: {measured_resistance:.1f}Ω")
        print(f"  Error: {error_percent:.1f}%")
        print(f"  Result: {'PASS' if result['pass'] else 'FAIL'}")
        
        return result
    
    def test_capacitor(self, test_voltage=3.3, expected_capacitance=1e-6):
        """Test capacitor value using charge/discharge timing"""
        print(f"Testing capacitor (expected: {expected_capacitance*1e6:.1f}µF)")
        
        # Discharge capacitor
        jumperless.dac_set(0, 0.0)
        time.sleep(0.5)
        
        # Start charging
        start_time = time.time()
        jumperless.dac_set(0, test_voltage)
        
        # Measure time to reach 63.2% of final voltage (1 time constant)
        target_voltage = test_voltage * 0.632
        
        while True:
            voltage = jumperless.adc_get(0)
            if voltage >= target_voltage:
                charge_time = time.time() - start_time
                break
            
            if time.time() - start_time > 5:  # Timeout
                charge_time = float('inf')
                break
            
            time.sleep(0.001)
        
        # Calculate capacitance (C = t / R, assuming known resistance)
        series_resistance = 1000  # Assume 1kΩ series resistor
        if charge_time < float('inf'):
            measured_capacitance = charge_time / series_resistance
        else:
            measured_capacitance = float('inf')
        
        error_percent = abs(measured_capacitance - expected_capacitance) / expected_capacitance * 100
        
        result = {
            'component': 'capacitor',
            'expected': expected_capacitance,
            'measured': measured_capacitance,
            'error_percent': error_percent,
            'pass': error_percent < 20  # 20% tolerance
        }
        
        self.results.append(result)
        
        print(f"  Expected: {expected_capacitance*1e6:.1f}µF")
        print(f"  Measured: {measured_capacitance*1e6:.1f}µF")
        print(f"  Charge time: {charge_time*1000:.1f}ms")
        print(f"  Error: {error_percent:.1f}%")
        print(f"  Result: {'PASS' if result['pass'] else 'FAIL'}")
        
        return result
    
    def test_diode(self, test_current=0.001):
        """Test diode forward voltage"""
        print(f"Testing diode forward voltage")
        
        # Apply forward bias
        jumperless.dac_set(0, 2.0)  # Should be enough for forward bias
        time.sleep(0.1)
        
        # Measure forward voltage
        forward_voltage = jumperless.adc_get(0)
        
        # Check if voltage is in expected range for silicon diode
        expected_min = 0.5
        expected_max = 0.8
        
        result = {
            'component': 'diode',
            'measured_vf': forward_voltage,
            'pass': expected_min <= forward_voltage <= expected_max
        }
        
        self.results.append(result)
        
        print(f"  Forward voltage: {forward_voltage:.3f}V")
        print(f"  Expected range: {expected_min}-{expected_max}V")
        print(f"  Result: {'PASS' if result['pass'] else 'FAIL'}")
        
        return result
    
    def generate_report(self):
        """Generate test report"""
        print("\n" + "="*50)
        print("AUTOMATED TEST REPORT")
        print("="*50)
        
        passed = sum(1 for r in self.results if r['pass'])
        total = len(self.results)
        
        print(f"Tests completed: {total}")
        print(f"Tests passed: {passed}")
        print(f"Tests failed: {total - passed}")
        print(f"Pass rate: {passed/total*100:.1f}%" if total > 0 else "No tests")
        
        print("\nDetailed Results:")
        for i, result in enumerate(self.results, 1):
            status = "PASS" if result['pass'] else "FAIL"
            print(f"  {i}. {result['component'].title()}: {status}")
        
        return self.results

# Example usage
def run_component_tests():
    tester = AutomatedTester()
    
    print("Insert component between holes 1 and 10, then press Enter")
    input()
    
    print("What component are you testing?")
    print("1. Resistor")
    print("2. Capacitor") 
    print("3. Diode")
    
    choice = input("Enter choice (1-3): ")
    
    if choice == "1":
        expected = float(input("Expected resistance (Ω): "))
        tester.test_resistor(expected_resistance=expected)
    elif choice == "2":
        expected = float(input("Expected capacitance (µF): ")) * 1e-6
        tester.test_capacitor(expected_capacitance=expected)
    elif choice == "3":
        tester.test_diode()
    
    tester.generate_report()

# Uncomment to run interactive test
# run_component_tests()
```

---

## Utility Scripts

### Connection Backup and Restore
Save and restore connection configurations.

```python
import jumperless

def save_connections(filename="connections.txt"):
    """Save current connections to file"""
    print(f"Saving connections to {filename}")
    
    # This is a simplified version - real implementation would 
    # use the actual connection state from hardware
    connections = [
        ("D2", "A0"),
        ("GND", "30"),
        ("GPIO_1", "10")
    ]  # Example connections
    
    try:
        with open(filename, 'w') as f:
            f.write("# Jumperless Connection Backup\n")
            f.write(f"# Saved at: {time.time()}\n")
            for node1, node2 in connections:
                f.write(f"{node1},{node2}\n")
        
        print(f"✓ Saved {len(connections)} connections")
        return True
    except Exception as e:
        print(f"✗ Save failed: {e}")
        return False

def load_connections(filename="connections.txt"):
    """Load connections from file"""
    print(f"Loading connections from {filename}")
    
    try:
        connections = []
        with open(filename, 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    parts = line.split(',')
                    if len(parts) == 2:
                        connections.append((parts[0], parts[1]))
        
        # Clear existing connections
        jumperless.nodes_clear()
        
        # Create new connections
        for node1, node2 in connections:
            try:
                jumperless.connect(node1, node2)
                print(f"✓ Connected {node1} to {node2}")
            except Exception as e:
                print(f"✗ Failed to connect {node1} to {node2}: {e}")
        
        print(f"Loaded {len(connections)} connections")
        return True
    except Exception as e:
        print(f"✗ Load failed: {e}")
        return False

def backup_manager():
    """Interactive backup manager"""
    print("Connection Backup Manager")
    print("1. Save current connections")
    print("2. Load connections from file")
    print("3. List backup files")
    
    choice = input("Choice (1-3): ")
    
    if choice == "1":
        filename = input("Filename (or Enter for default): ").strip()
        if not filename:
            filename = "connections.txt"
        save_connections(filename)
    
    elif choice == "2":
        filename = input("Filename (or Enter for default): ").strip()
        if not filename:
            filename = "connections.txt"
        load_connections(filename)
    
    elif choice == "3":
        print("Available backup files:")
        # In real implementation, would list actual files
        print("  connections.txt")
        print("  test_setup.txt")
        print("  sensor_platform.txt")

# backup_manager()
```

### Performance Benchmark
Benchmark system performance.

```python
import jumperless
import time

def benchmark_dac_speed():
    """Benchmark DAC update speed"""
    print("DAC Speed Benchmark")
    
    iterations = 100
    start_time = time.time()
    
    for i in range(iterations):
        voltage = 3.3 * (i % 2)  # Alternate between 0 and 3.3V
        jumperless.dac_set(0, voltage)
    
    end_time = time.time()
    total_time = end_time - start_time
    rate = iterations / total_time
    
    print(f"Completed {iterations} DAC updates in {total_time:.3f}s")
    print(f"Update rate: {rate:.1f} updates/second")
    print(f"Time per update: {total_time/iterations*1000:.2f}ms")

def benchmark_adc_speed():
    """Benchmark ADC read speed"""
    print("ADC Speed Benchmark")
    
    iterations = 100
    start_time = time.time()
    
    voltages = []
    for i in range(iterations):
        voltage = jumperless.adc_get(0)
        voltages.append(voltage)
    
    end_time = time.time()
    total_time = end_time - start_time
    rate = iterations / total_time
    
    print(f"Completed {iterations} ADC reads in {total_time:.3f}s")
    print(f"Read rate: {rate:.1f} reads/second")
    print(f"Time per read: {total_time/iterations*1000:.2f}ms")
    print(f"Voltage range: {min(voltages):.3f}V to {max(voltages):.3f}V")

def benchmark_gpio_speed():
    """Benchmark GPIO toggle speed"""
    print("GPIO Speed Benchmark")
    
    pin = 1
    jumperless.gpio_set_dir(pin, True)  # Set as output
    iterations = 100
    
    start_time = time.time()
    
    for i in range(iterations):
        state = bool(i % 2)
        jumperless.gpio_set(pin, state)
    
    end_time = time.time()
    total_time = end_time - start_time
    rate = iterations / total_time
    
    print(f"Completed {iterations} GPIO toggles in {total_time:.3f}s")
    print(f"Toggle rate: {rate:.1f} toggles/second")
    print(f"Time per toggle: {total_time/iterations*1000:.2f}ms")

def benchmark_connection_speed():
    """Benchmark connection switching speed"""
    print("Connection Speed Benchmark")
    
    connections = [
        ("1", "2"),
        ("3", "4"),
        ("5", "6"),
        ("7", "8")
    ]
    
    iterations = 20  # Connection switching is slower
    start_time = time.time()
    
    for i in range(iterations):
        node1, node2 = connections[i % len(connections)]
        jumperless.connect(node1, node2)
        jumperless.disconnect(node1, node2)
    
    end_time = time.time()
    total_time = end_time - start_time
    rate = (iterations * 2) / total_time  # Connect + disconnect
    
    print(f"Completed {iterations} connect/disconnect cycles in {total_time:.3f}s")
    print(f"Operation rate: {rate:.1f} operations/second")
    print(f"Time per operation: {total_time/(iterations*2)*1000:.1f}ms")

def run_all_benchmarks():
    """Run complete benchmark suite"""
    print("Jumperless Performance Benchmark Suite")
    print("="*40)
    
    benchmark_dac_speed()
    print()
    benchmark_adc_speed()
    print()
    benchmark_gpio_speed()
    print()
    benchmark_connection_speed()
    
    print("Benchmark suite complete!")

run_all_benchmarks()
```

---

## File Management Examples

Since all these scripts are designed to be saved and reused, here's how to manage them effectively:

### Save Script Template
```python
# Template for saving scripts in Jumperless
def save_script_to_file(script_content, filename):
    """Save script content to file"""
    try:
        with open(f"/python_scripts/{filename}", 'w') as f:
            f.write(script_content)
        print(f"✓ Script saved as {filename}")
    except Exception as e:
        print(f"✗ Save failed: {e}")

# Example usage:
# save_script_to_file('''
# import jumperless
# jumperless.oled_print("Hello World!")
# ''', "hello_world.py")
```

### Script Launcher
```python
def list_and_run_scripts():
    """List available scripts and run selected one"""
    try:
        files = jumperless.fs_listdir("/python_scripts")
        python_files = [f for f in files.split(',') if f.endswith('.py')]
        
        print("Available Scripts:")
        for i, filename in enumerate(python_files, 1):
            print(f"  {i}. {filename}")
        
        choice = int(input("Select script (number): ")) - 1
        if 0 <= choice < len(python_files):
            filename = python_files[choice]
            print(f"Running {filename}...")
            exec(open(f"/python_scripts/{filename}").read())
        else:
            print("Invalid selection")
            
    except Exception as e:
        print(f"Error: {e}")

# Uncomment to use:
# list_and_run_scripts()
```

These examples provide a comprehensive foundation for working with Jumperless. They can be used as-is, modified for specific needs, or combined to create more complex applications. Each script is designed to be educational, practical, and immediately usable. 