# Jumperless Demo Script
# This script demonstrates how to control Jumperless hardware using Python
# Can be run from: MicroPython REPL, saved as .py file, or external Python

import JythonModule as jl
import time

def main():
    """Main demo function showing Jumperless capabilities"""
    print("=== Jumperless Python Demo ===")
    
    # Display demo info on OLED
    try:
        jl.oled.connect()
        jl.oled.print("Python Demo", 2)
        time.sleep(1)
    except:
        print("OLED not available, continuing...")
    
    # DAC and ADC demo
    print("\n1. DAC/ADC Demo")
    print("Setting DAC 0 to 2.5V...")
    jl.dac.set(0, 2.5)
    
    print("Reading ADC channels...")
    for channel in range(3):
        voltage = jl.adc.get(channel)
        print(f"  ADC {channel}: {voltage}")
    
    # GPIO demo
    print("\n2. GPIO Demo")
    print("Setting GPIO pins...")
    jl.gpio.set(1, jl.gpio.HIGH)
    jl.gpio.set(2, jl.gpio.LOW)
    
    print("Reading GPIO states...")
    for pin in range(1, 6):
        state = jl.gpio.get(pin)
        print(f"  GPIO {pin}: {state}")
    
    # Node connection demo
    print("\n3. Node Connection Demo")
    print("Connecting nodes 1 and 5...")
    jl.nodes.connect(1, 5, save=False)
    
    time.sleep(0.5)
    
    print("Disconnecting nodes 1 and 5...")
    jl.nodes.disconnect(1, 5)
    
    # OLED demo sequence
    print("\n4. OLED Demo")
    try:
        messages = [
            ("Hello!", 2),
            ("Jumperless", 1),
            ("Python", 2),
            ("Demo", 2)
        ]
        
        for msg, size in messages:
            jl.oled.print(msg, size)
            time.sleep(1)
            
        # Font cycling demo
        print("Cycling fonts...")
        for i in range(3):
            jl.oled.cycle_font()
            jl.oled.print(f"Font {i}", 2)
            time.sleep(1)
            
    except Exception as e:
        print(f"OLED error: {e}")
    
    # Convenience function demo
    print("\n5. Convenience Functions Demo")
    jl.set_voltage(1, 3.3)
    voltage = jl.read_voltage(1)
    print(f"Set DAC 1 to 3.3V, read: {voltage}")
    
    jl.connect_nodes(10, 15)
    jl.display("Done!", 2)
    
    print("\n=== Demo Complete ===")

def blink_demo():
    """Simple LED blinking demo using GPIO"""
    print("GPIO Blink Demo (Ctrl+C to stop)")
    
    try:
        while True:
            jl.gpio.set(5, jl.gpio.HIGH)
            jl.display("LED ON", 1)
            time.sleep(0.5)
            
            jl.gpio.set(5, jl.gpio.LOW) 
            jl.display("LED OFF", 1)
            time.sleep(0.5)
            
    except KeyboardInterrupt:
        print("\nBlink demo stopped")
        jl.gpio.set(5, jl.gpio.LOW)

def voltage_sweep():
    """Sweep DAC voltage and monitor with ADC"""
    print("Voltage Sweep Demo")
    
    jl.oled.print("Sweep", 2)
    
    # Connect DAC 0 to ADC 0 for monitoring
    jl.nodes.connect(60, 70)  # Assuming these are DAC0/ADC0 nodes
    
    for voltage in [0.0, 1.0, 2.0, 3.0, 3.3, 2.0, 1.0, 0.0]:
        print(f"Setting {voltage}V...")
        jl.dac.set(0, voltage)
        time.sleep(0.2)
        
        reading = jl.adc.get(0)
        print(f"  Read: {reading}")
        
        jl.oled.print(f"{voltage}V", 2)
        time.sleep(0.5)
    
    # Disconnect
    jl.nodes.disconnect(60, 70)
    jl.oled.print("Done", 2)

# Interactive mode functions
def interactive_help():
    """Print interactive help"""
    print("""
Jumperless Interactive Mode

Available demos:
  main()         - Full demo of all features
  blink_demo()   - GPIO LED blinking
  voltage_sweep() - DAC voltage sweep with ADC monitoring

Manual control examples:
  jl.dac.set(0, 2.5)         # Set DAC 0 to 2.5V
  jl.adc.get(0)              # Read ADC channel 0
  jl.gpio.set(5, jl.gpio.HIGH) # Set GPIO 5 HIGH
  jl.nodes.connect(1, 5)     # Connect nodes 1 and 5
  jl.oled.print("Hello!")    # Display on OLED
  jl.arduino.reset()         # Reset Arduino

For help on specific modules:
  help(jl.dac)    # DAC help
  help(jl.gpio)   # GPIO help
  help(jl.oled)   # OLED help
    """)

# Run main demo if script is executed directly
if __name__ == "__main__":
    main()
else:
    # If imported, show help
    interactive_help() 