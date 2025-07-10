"""
INA (Current/Power Monitor) Reference
====================================

Complete reference for all INA functions in the Jumperless MicroPython module.
This file demonstrates every current and power monitoring function with practical examples.

Functions demonstrated:
- ina_get_current(sensor) - Read current in Amps
- ina_get_voltage(sensor) - Read shunt voltage in V
- ina_get_bus_voltage(sensor) - Read bus voltage in V
- ina_get_power(sensor) - Read power in Watts
- get_current() - Alias for ina_get_current()
- get_voltage() - Alias for ina_get_voltage()
- get_bus_voltage() - Alias for ina_get_bus_voltage()
- get_power() - Alias for ina_get_power()

INA Sensors:
- Sensor 0: INA sensor 0
- Sensor 1: INA sensor 1
- Current range: Depends on shunt resistor configuration
- Voltage range: Typically 0-26V bus voltage

Usage:
  exec(open('micropython_examples/ina_reference.py').read())
"""

import time

def ina_basic_operations():
    """Demonstrate basic INA operations"""
    
    print("╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                           INA BASIC OPERATIONS                              │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("INA Basic Demo")
    
    print("\n☺ Reading all INA sensors:")
    
    # Read all measurements from both sensors
    for sensor in range(2):
        print("\n  INA Sensor " + str(sensor) + ":")
        
        # Current measurement
        current = ina_get_current(sensor)
        print("    ina_get_current(" + str(sensor) + "): " + str(current) + "A")
        
        # Shunt voltage measurement
        shunt_voltage = ina_get_voltage(sensor)
        print("    ina_get_voltage(" + str(sensor) + "): " + str(shunt_voltage) + "V")
        
        # Bus voltage measurement
        bus_voltage = ina_get_bus_voltage(sensor)
        print("    ina_get_bus_voltage(" + str(sensor) + "): " + str(bus_voltage) + "V")
        
        # Power measurement
        power = ina_get_power(sensor)
        print("    ina_get_power(" + str(sensor) + "): " + str(power) + "W")
        
        # Display on OLED
        oled_clear()
        oled_print("INA" + str(sensor) + ": " + str(current * 1000) + "mA")
        time.sleep(1.5)
    
    print("\n☺ Using alias functions:")
    
    # Test alias functions
    current = get_current(0)
    voltage = get_voltage(0)
    bus_voltage = get_bus_voltage(0)
    power = get_power(0)
    
    print("  get_current(0): " + str(current) + "A (alias)")
    print("  get_voltage(0): " + str(voltage) + "V (alias)")
    print("  get_bus_voltage(0): " + str(bus_voltage) + "V (alias)")
    print("  get_power(0): " + str(power) + "W (alias)")
    
    oled_clear()
    oled_print("Aliases: " + str(current * 1000) + "mA")
    time.sleep(1)
    
    print("✓ Basic INA operations complete")

def ina_continuous_monitoring():
    """Demonstrate continuous current and power monitoring"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                        CONTINUOUS MONITORING                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("INA Monitoring")
    
    print("\n☺ Continuous monitoring (30 samples):")
    print("  Sample | INA0 Current | INA0 Power | INA1 Current | INA1 Power")
    print("  -------|--------------|------------|--------------|------------")
    
    for sample in range(30):
        # Read both sensors
        current0 = get_current(0)
        power0 = get_power(0)
        current1 = get_current(1)
        power1 = get_power(1)
        
        # Format for display
        current0_str = str(round(current0 * 1000, 3)) + "mA"
        power0_str = str(round(power0 * 1000, 3)) + "mW"
        current1_str = str(round(current1 * 1000, 3)) + "mA"
        power1_str = str(round(power1 * 1000, 3)) + "mW"
        
        print("  " + str(sample + 1).rjust(6) + " | " + current0_str.rjust(12) + " | " + 
              power0_str.rjust(10) + " | " + current1_str.rjust(12) + " | " + power1_str.rjust(10))
        
        # Show current sample on OLED
        oled_clear()
        if abs(current0) > 0.001:  # > 1mA
            oled_print("INA0: " + str(round(current0 * 1000, 1)) + "mA")
        else:
            oled_print("INA0: " + str(round(current0 * 1000000, 0)) + "µA")
        
        time.sleep(0.2)
    
    print("✓ Continuous monitoring complete")

def ina_load_testing():
    """Test INA sensors with controlled loads"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                              LOAD TESTING                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Load Testing")
    
    print("\n☺ Setting up controlled load testing:")
    print("  This test will vary load and measure current consumption")
    
    # Clear connections for clean setup
    nodes_clear()
    
    # Set up test circuit through current sensor
    print("  Setting up test circuit:")
    connect(DAC0, ISENSE_PLUS)    # Power source through current sensor
    connect(ISENSE_MINUS, 30)     # Current sensor output to load
    print("    DAC0 -> ISENSE_PLUS -> ISENSE_MINUS -> Load (hole 30)")
    print("    Connect variable load between hole 30 and GND")
    
    # Test different voltage levels
    test_voltages = [0.5, 1.0, 1.5, 2.0, 2.5, 3.0]
    
    print("\n  Testing at different voltage levels:")
    print("  Voltage | Current   | Power     | Bus V")
    print("  --------|-----------|-----------|--------")
    
    for voltage in test_voltages:
        # Set test voltage
        dac_set(DAC0, voltage)
        time.sleep(0.5)  # Allow settling
        
        # Read measurements
        current = get_current(0)
        power = get_power(0)
        bus_voltage = get_bus_voltage(0)
        
        # Format measurements
        current_str = str(round(current * 1000, 2)) + "mA"
        power_str = str(round(power * 1000, 2)) + "mW"
        bus_str = str(round(bus_voltage, 3)) + "V"
        
        print("  " + str(voltage) + "V     | " + current_str.rjust(9) + " | " + 
              power_str.rjust(9) + " | " + bus_str)
        
        oled_clear()
        oled_print(str(voltage) + "V: " + current_str)
        time.sleep(1)
    
    # Reset voltage
    dac_set(DAC0, 0.0)
    
    print("✓ Load testing complete")

def ina_power_profiling():
    """Profile power consumption over time"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                            POWER PROFILING                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Power Profiling")
    
    print("\n☺ Power consumption profiling:")
    print("  Collecting power data with statistics")
    
    # Data collection arrays
    current_readings = []
    power_readings = []
    voltage_readings = []
    
    num_samples = 100
    print("\n  Collecting " + str(num_samples) + " samples...")
    
    # Collect data
    max_current = 0
    min_current = float('inf')
    
    for sample in range(num_samples):
        current = get_current(0)
        power = get_power(0)
        voltage = get_bus_voltage(0)
        
        current_readings.append(current)
        power_readings.append(power)
        voltage_readings.append(voltage)
        
        # Track extremes
        if current > max_current:
            max_current = current
        if current < min_current:
            min_current = current
        
        # Show progress
        if sample % 10 == 0:
            oled_clear()
            oled_print("Sample " + str(sample + 1) + "/" + str(num_samples))
        
        time.sleep(0.05)
    
    # Calculate statistics
    avg_current = sum(current_readings) / len(current_readings)
    avg_power = sum(power_readings) / len(power_readings)
    avg_voltage = sum(voltage_readings) / len(voltage_readings)
    
    # Calculate standard deviation for current
    current_variance = sum((x - avg_current) ** 2 for x in current_readings) / len(current_readings)
    current_std = current_variance ** 0.5
    
    # Calculate peak-to-peak noise
    current_pp = max_current - min_current
    
    print("\n  Power Profile Results:")
    print("    Samples collected: " + str(num_samples))
    print("    Average current: " + str(round(avg_current * 1000, 3)) + " mA")
    print("    Average power: " + str(round(avg_power * 1000, 3)) + " mW")
    print("    Average bus voltage: " + str(round(avg_voltage, 3)) + " V")
    print("    Current std dev: " + str(round(current_std * 1000, 3)) + " mA")
    print("    Current range: " + str(round(min_current * 1000, 3)) + " to " + str(round(max_current * 1000, 3)) + " mA")
    print("    Current p-p noise: " + str(round(current_pp * 1000, 3)) + " mA")
    
    # Show distribution (simplified histogram)
    print("\n  Current distribution (simplified):")
    
    # Create bins
    bins = 5
    bin_width = current_pp / bins
    histogram = [0] * bins
    
    for reading in current_readings:
        bin_index = int((reading - min_current) / bin_width)
        if bin_index >= bins:
            bin_index = bins - 1
        histogram[bin_index] += 1
    
    for i, count in enumerate(histogram):
        bin_current = min_current + i * bin_width
        bar = "█" * (count // 4)  # Scale for display
        print("    " + str(round(bin_current * 1000, 2)) + "mA: " + bar + " (" + str(count) + ")")
    
    # Show final stats on OLED
    oled_clear()
    oled_print("Avg: " + str(round(avg_current * 1000, 1)) + "mA")
    time.sleep(2)
    
    print("✓ Power profiling complete")

def ina_efficiency_measurement():
    """Measure power efficiency"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                          EFFICIENCY MEASUREMENT                             │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Efficiency Test")
    
    print("\n☺ Power efficiency measurement:")
    print("  Using both INA sensors to measure input and output power")
    
    # Setup for efficiency testing
    print("  Setting up power efficiency test circuit:")
    print("    INA0: Input power measurement")
    print("    INA1: Output power measurement")
    
    # Test circuit configuration
    nodes_clear()
    
    # Input power through INA0
    connect(DAC0, ISENSE_PLUS)  # Using DAC0 as power source
    connect(ISENSE_MINUS, 35)   # INA0 measures input
    
    print("    DAC0 -> INA0 -> Converter Input (hole 35)")
    print("    Connect converter/regulator between hole 35 and output")
    print("    Connect INA1 to measure output power")
    
    # Test at different input voltages
    input_voltages = [2.0, 2.5, 3.0, 3.5, 4.0]
    
    print("\n  Efficiency measurements:")
    print("  Input V | Input P | Output P | Efficiency")
    print("  --------|---------|----------|------------")
    
    for input_voltage in input_voltages:
        # Set input voltage
        dac_set(DAC0, input_voltage)
        time.sleep(0.5)  # Allow settling
        
        # Measure input power (INA0)
        input_current = get_current(0)
        input_power = get_power(0)
        input_bus_v = get_bus_voltage(0)
        
        # Measure output power (INA1)
        output_current = get_current(1)
        output_power = get_power(1)
        output_bus_v = get_bus_voltage(1)
        
        # Calculate efficiency
        if input_power > 0:
            efficiency = (output_power / input_power) * 100
        else:
            efficiency = 0
        
        # Format for display
        input_p_str = str(round(input_power * 1000, 2)) + "mW"
        output_p_str = str(round(output_power * 1000, 2)) + "mW"
        eff_str = str(round(efficiency, 1)) + "%"
        
        print("  " + str(input_voltage) + "V    | " + input_p_str.rjust(7) + " | " + 
              output_p_str.rjust(8) + " | " + eff_str.rjust(10))
        
        oled_clear()
        oled_print("Eff: " + eff_str)
        time.sleep(1)
    
    # Reset
    dac_set(DAC0, 0.0)
    
    print("\n  Additional measurements:")
    print("    Input current range: " + str(round(min([get_current(0) for _ in range(5)]) * 1000, 3)) + 
          " to " + str(round(max([get_current(0) for _ in range(5)]) * 1000, 3)) + " mA")
    print("    Output current range: " + str(round(min([get_current(1) for _ in range(5)]) * 1000, 3)) + 
          " to " + str(round(max([get_current(1) for _ in range(5)]) * 1000, 3)) + " mA")
    
    print("✓ Efficiency measurement complete")

def ina_threshold_monitoring():
    """Monitor for threshold violations"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                         THRESHOLD MONITORING                                │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Threshold Monitor")
    
    print("\n☺ Current threshold monitoring:")
    print("  Monitoring for current threshold violations")
    
    # Set monitoring thresholds
    current_threshold_low = 0.005   # 5mA
    current_threshold_high = 0.100  # 100mA
    power_threshold = 0.250         # 250mW
    
    print("  Thresholds:")
    print("    Current low: " + str(current_threshold_low * 1000) + "mA")
    print("    Current high: " + str(current_threshold_high * 1000) + "mA")
    print("    Power: " + str(power_threshold * 1000) + "mW")
    
    # Monitoring loop
    print("\n  Monitoring (press Ctrl+C to stop):")
    print("  Time | Current | Power | Status")
    print("  -----|---------|-------|--------")
    
    start_time = time.time()
    violations = 0
    
    try:
        while True:
            current_time = time.time() - start_time
            
            # Read measurements
            current = get_current(0)
            power = get_power(0)
            voltage = get_bus_voltage(0)
            
            # Check thresholds
            status = "OK"
            if current < current_threshold_low:
                status = "LOW_CURRENT"
                violations += 1
            elif current > current_threshold_high:
                status = "HIGH_CURRENT"
                violations += 1
            elif power > power_threshold:
                status = "HIGH_POWER"
                violations += 1
            
            # Format display
            time_str = str(round(current_time, 1)) + "s"
            current_str = str(round(current * 1000, 1)) + "mA"
            power_str = str(round(power * 1000, 1)) + "mW"
            
            print("  " + time_str.rjust(4) + " | " + current_str.rjust(7) + " | " + 
                  power_str.rjust(5) + " | " + status)
            
            # Show status on OLED
            if status == "OK":
                oled_clear()
                oled_print("Monitor: " + current_str)
            else:
                oled_clear()
                oled_print("ALERT: " + status)
            
            time.sleep(1)
            
            # Auto-stop after 30 readings for demo
            if current_time > 30:
                break
                
    except KeyboardInterrupt:
        print("\n  Monitoring stopped by user")
    
    print("\n  Monitoring summary:")
    print("    Total violations: " + str(violations))
    print("    Monitoring time: " + str(round(time.time() - start_time, 1)) + "s")
    
    oled_clear()
    oled_print("Monitor Done")
    time.sleep(1)
    
    print("✓ Threshold monitoring complete")

def ina_sensor_comparison():
    """Compare readings between INA sensors"""
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                          SENSOR COMPARISON                                  │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    
    oled_clear()
    oled_print("Sensor Comparison")
    
    print("\n☺ Comparing INA sensor readings:")
    print("  Reading both sensors simultaneously")
    
    print("\n  Sample | INA0 Current | INA0 Power | INA1 Current | INA1 Power | Difference")
    print("  -------|--------------|------------|--------------|------------|------------")
    
    differences = []
    
    for sample in range(20):
        # Read both sensors at the same time
        current0 = get_current(0)
        power0 = get_power(0)
        voltage0 = get_bus_voltage(0)
        
        current1 = get_current(1)
        power1 = get_power(1)
        voltage1 = get_bus_voltage(1)
        
        # Calculate difference
        current_diff = abs(current0 - current1)
        power_diff = abs(power0 - power1)
        differences.append(current_diff)
        
        # Format for display
        c0_str = str(round(current0 * 1000, 2)) + "mA"
        p0_str = str(round(power0 * 1000, 2)) + "mW"
        c1_str = str(round(current1 * 1000, 2)) + "mA"
        p1_str = str(round(power1 * 1000, 2)) + "mW"
        diff_str = str(round(current_diff * 1000, 2)) + "mA"
        
        print("  " + str(sample + 1).rjust(6) + " | " + c0_str.rjust(12) + " | " + 
              p0_str.rjust(10) + " | " + c1_str.rjust(12) + " | " + p1_str.rjust(10) + 
              " | " + diff_str.rjust(10))
        
        # Show comparison on OLED
        oled_clear()
        oled_print("INA0:" + str(round(current0 * 1000, 1)) + " INA1:" + str(round(current1 * 1000, 1)))
        time.sleep(0.5)
    
    # Calculate comparison statistics
    avg_diff = sum(differences) / len(differences)
    max_diff = max(differences)
    min_diff = min(differences)
    
    print("\n  Sensor comparison statistics:")
    print("    Average difference: " + str(round(avg_diff * 1000, 3)) + "mA")
    print("    Maximum difference: " + str(round(max_diff * 1000, 3)) + "mA")
    print("    Minimum difference: " + str(round(min_diff * 1000, 3)) + "mA")
    
    # Show sensor characteristics
    print("\n  Individual sensor characteristics:")
    for sensor in range(2):
        readings = [get_current(sensor) for _ in range(10)]
        avg_reading = sum(readings) / len(readings)
        std_reading = (sum((x - avg_reading) ** 2 for x in readings) / len(readings)) ** 0.5
        
        print("    INA" + str(sensor) + " - Avg: " + str(round(avg_reading * 1000, 3)) + 
              "mA, Std: " + str(round(std_reading * 1000, 3)) + "mA")
    
    print("✓ Sensor comparison complete")

def run_all_ina_demos():
    """Run all INA demonstration functions"""
    
    print("🚀 Starting Complete INA Reference Demonstration")
    print("═" * 75)
    
    demos = [
        ("Basic Operations", ina_basic_operations),
        ("Continuous Monitoring", ina_continuous_monitoring),
        ("Load Testing", ina_load_testing),
        ("Power Profiling", ina_power_profiling),
        ("Efficiency Measurement", ina_efficiency_measurement),
        ("Threshold Monitoring", ina_threshold_monitoring),
        ("Sensor Comparison", ina_sensor_comparison)
    ]
    
    for name, demo_func in demos:
        print("\n📍 Running: " + name)
        print("─" * 50)
        try:
            demo_func()
            print("✓ " + name + " completed successfully")
        except Exception as e:
            print("❌ " + name + " failed: " + str(e))
        
        time.sleep(2)
    
    oled_clear()
    oled_print("INA Reference Complete!")
    print("\n🎉 All INA demonstrations complete!")
    print("═" * 75)

def ina_quick_test():
    """Quick test of INA functions"""
    
    print("⚡ Quick INA Test")
    print("─" * 20)
    
    # Test both sensors briefly
    for sensor in range(2):
        current = get_current(sensor)
        power = get_power(sensor)
        voltage = get_bus_voltage(sensor)
        
        print("INA" + str(sensor) + " - Current: " + str(round(current * 1000, 3)) + "mA, " +
              "Power: " + str(round(power * 1000, 3)) + "mW, " +
              "Voltage: " + str(round(voltage, 3)) + "V")
    
    print("✓ Quick INA test complete")

# Run the demonstration
if __name__ == "__main__":
    run_all_ina_demos() 