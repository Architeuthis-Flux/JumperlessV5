# Demonstration of synchronous Jumperless hardware control
# This shows how to use return values from hardware commands in Python code

import JythonModule as jl

print("=== Jumperless Synchronous Demo ===")

# Test 1: Read ADC and use the value
print("\n1. Reading ADC voltage:")
voltage = jl.adc.get(0)
print("ADC Channel 0:", voltage, "V")

if voltage > 2.5:
    print("Voltage is high!")
    jl.oled.print("High: " + str(voltage) + "V")
else:
    print("Voltage is low")
    jl.oled.print("Low: " + str(voltage) + "V")

# Test 2: GPIO state checking
print("\n2. GPIO state checking:")
for pin in range(1, 6):
    state = jl.gpio.get(pin)
    print("GPIO", pin, "is", "HIGH" if state else "LOW")
    
    # Set LED or do something based on state
    if state:
        print("  Pin", pin, "is active!")

# Test 3: INA current monitoring
print("\n3. Current monitoring:")
try:
    current = jl.ina.get_current(0)
    power = jl.ina.get_power(0)
    print("INA0 - Current:", current, "mA, Power:", power, "mW")
    
    if current > 100:
        print("High current detected!")
        jl.oled.print("HIGH CURRENT!")
    else:
        jl.oled.print("Current OK")
except Exception as e:
    print("INA read error:", e)

# Test 4: Conditional node connections
print("\n4. Conditional connections:")
pin_state = jl.gpio.get(1)
if pin_state:
    print("Pin 1 is HIGH - connecting nodes 1 and 5")
    jl.nodes.connect(1, 5)
else:
    print("Pin 1 is LOW - connecting nodes 2 and 6")
    jl.nodes.connect(2, 6)

# Test 5: Voltage-based DAC control
print("\n5. Feedback control:")
input_voltage = jl.adc.get(0)
if input_voltage < 2.0:
    jl.dac.set(0, 2.5)
    print("Input low, set DAC to 2.5V")
elif input_voltage > 4.0:
    jl.dac.set(0, 1.0)
    print("Input high, set DAC to 1.0V")
else:
    jl.dac.set(0, input_voltage)
    print("Input OK, DAC follows input:", input_voltage, "V")


print("Controlling real OLED...")
#jl.oled.connect()
jl.oled.print("Hello Sync!")
print("OLED updated!")
run


print("\n=== Demo Complete ===")
print("You can now write Python code that uses actual sensor values!")
print("Example: if jl.gpio.get(1): jl.oled.print('Button pressed!')") 