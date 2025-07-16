# Hardware Interface Guide

This document provides an overview of the Jumperless hardware components that can be controlled and monitored through the MicroPython API.

## Power System

### Power Rails (`TOP_RAIL`, `BOTTOM_RAIL`)
The Jumperless board features two programmable power rails that run along the top and bottom of the breadboard.

*   **Voltage Range**: -8.0V to +8.0V
*   **Control**: Use the `dac_set()` function with the `TOP_RAIL` or `BOTTOM_RAIL` constants.
*   **Monitoring**: The rail voltages can be monitored via the ADCs if connected.

**Example:**
```python
# Set the top rail to 5V and the bottom rail to -5V
dac_set(TOP_RAIL, 5.0)
dac_set(BOTTOM_RAIL, -5.0)
```

### Onboard Supplies (`3V3`, `5V`)
The board provides fixed 3.3V and 5V power supplies.

*   `SUPPLY_3V3`: A fixed 3.3V supply.
*   `SUPPLY_5V`: A fixed 5V supply, typically from USB.
*   These are available as nodes for connection but their voltage is not programmable.

**Example:**
```python
# Connect a component to the 3.3V supply
connect(1, SUPPLY_3V3)
```

## Analog I/O

### DACs (Digital-to-Analog Converters)
Two general-purpose DACs are available for providing variable analog voltages.

*   `DAC0`: 5V tolerant DAC output.
*   `DAC1`: 8V tolerant DAC output.
*   **Control**: Use `dac_set(DAC0, voltage)` or `dac_set(DAC1, voltage)`.

**Example:**
```python
# Generate a 1.25V signal on DAC1
dac_set(DAC1, 1.25)
connect(DAC1, 15) # Connect to breadboard row 15
```

### ADCs (Analog-to-Digital Converters)
Multiple ADC channels are available for measuring analog voltages.

*   **Channels 0-3**: 8V tolerant inputs.
*   **Channel 4**: 5V tolerant input.
*   **Probe ADC (Channel 7)**: The ADC connected to the probe tip for measurements.
*   **Control**: Use `adc_get(channel)` to read the voltage.

**Example:**
```python
# Measure the voltage on ADC channel 2
voltage = adc_get(2)
print("Voltage at ADC2: " + str(voltage))
```

## Digital I/O (GPIO)

The Jumperless provides 8 routable GPIO pins and 2 pins dedicated to UART.

*   **Pins**: `GPIO_1` through `GPIO_8`.
*   **Functions**: Each pin can be configured as an input (with optional pull-up/pull-down resistors) or an output. They also support PWM.
*   **Control**: Use the `gpio_*` and `pwm_*` functions.

**Example:**
```python
# Blink an LED connected to GPIO_1
gpio_set_dir(GPIO_1, True) # Set as output
while True:
    gpio_set(GPIO_1, True)
    time.sleep(0.5)
    gpio_set(GPIO_1, False)
    time.sleep(0.5)
```

## UART (Serial Communication)

Two pins are available for serial communication.

*   `UART_TX`: Transmit pin.
*   `UART_RX`: Receive pin.
*   These can be used as standard GPIO pins or for serial communication with other devices.

**Example:**
```python
# Connect the Jumperless UART to an external device
connect(UART_TX, "some_rx_pin")
connect(UART_RX, "some_tx_pin")
```

## Current/Power Monitor (INA)

The INA219 sensors allow for monitoring current and power consumption.

*   **Sensor 0**: General purpose current/power monitor.
*   **Sensor 1**: Typically used for resistance measurement with DAC0.
*   **Control**: Use the `ina_*` functions to get current, voltage, and power readings.

**Example:**
```python
# Measure the current flowing through sensor 0
current = ina_get_current(0)
print("Current: " + str(current * 1000) + " mA")
```

## Routable Buffer

A routable buffer is available for isolating or strengthening signals.

*   `BUFFER_IN`: The input to the buffer.
*   `BUFFER_OUT`: The output of the buffer.

**Example:**
```python
# Pass a signal through the buffer
connect(some_signal_source, BUFFER_IN)
connect(BUFFER_OUT, some_destination)
```

## Arduino Nano Interface

The Jumperless board has headers that are compatible with an Arduino Nano, and all of the Nano's pins are available as nodes in MicroPython.

*   **Digital Pins**: `D0` - `D13`
*   **Analog Pins**: `A0` - `A7`
*   **Power Pins**: `VIN`, `NANO_5V`, `NANO_3V3`, `NANO_GND_0`, `NANO_GND_1`
*   **Other Pins**: `RESET`, `AREF`

**Example:**
```python
# Connect the Arduino's D13 pin to an LED on row 25
connect(D13, 25)
connect(NANO_GND_0, 26) # Provide ground for the LED
```
