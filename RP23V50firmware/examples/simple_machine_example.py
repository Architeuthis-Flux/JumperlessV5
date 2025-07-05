# Simple Machine Module Example for Jumperless
# Demonstrates Pin, ADC, and PWM classes with Jumperless integration

print("=== Jumperless Machine Module Example ===")

# Test basic machine module functionality
try:
    from machine import Pin, ADC, PWM
    print("✓ Machine module imported")
    
    # Create a pin on breadboard node 1
    pin1 = Pin(1, Pin.OUT)
    print("✓ Created Pin(1)")
    
    # Control the pin
    pin1.on()
    print("✓ Pin set HIGH")
    
    pin1.off()
    print("✓ Pin set LOW")
    
    # Read pin state
    state = pin1.value()
    print(f"✓ Pin state: {state}")
    
    # Test ADC
    adc = ADC(26)  # ADC channel 0
    reading = adc.read_u16()
    voltage = (reading / 65535) * 3.3
    print(f"✓ ADC reading: {voltage:.3f}V")
    
    # Test PWM for DAC
    dac = PWM(Pin(16))  # TOP_RAIL
    dac.duty_u16(32768)  # Set to mid-range
    print("✓ DAC set via PWM")
    
except ImportError as e:
    print(f"❌ Machine module error: {e}")

# Test Jumperless functions still work
try:
    result = connect(1, 5)
    print(f"✓ connect(1, 5): {result}")
    
    voltage = adc_get(0)
    print(f"✓ adc_get(0): {voltage:.3f}V")
    
except NameError:
    print("⚠ Jumperless functions not in global scope")

print("=== Example complete ===")

# Usage instructions
print("\nTo use this:")
print("1. Copy to python_scripts/ on Jumperless")
print("2. In REPL: exec(open('python_scripts/simple_machine_example.py').read())") 