# Jumperless MicroPython API Reference

This document provides a comprehensive reference for the `jumperless` MicroPython module, which allows for direct control over the Jumperless hardware.

## A Note on Usage

All functions and constants from the `jumperless` module are automatically imported into the global namespace. This means you can call them directly (e.g., `connect(1, 5)`) without needing the `jumperless.` prefix.

There are three primary ways to specify nodes in functions:
1.  **By Number**: Use the integer corresponding to the breadboard row (1-60).
2.  **By String Name**: Use a case-insensitive string for any named node (e.g., `"d13"`, `"TOP_RAIL"`).
3.  **By Constant**: Use the predefined, case-sensitive constant for a node (e.g., `D13`, `TOP_RAIL`).

---

## Node Connections

These functions manage the connections between nodes on the breadboard and special function pins.

### `connect(node1, node2, [save=True])`
Creates a bridge between two nodes.

*   `node1`, `node2`: The nodes to connect. Can be integers, strings, or constants.
*   `save` (optional): If `True` (default), the connection is saved to the current slot's node file. If `False`, it's a temporary connection for the current session.

**Example:**
```python
# Connect breadboard row 1 to row 30
connect(1, 30)

# Connect Arduino D13 to the top power rail
connect(D13, TOP_RAIL)

# Connect GPIO 1 to ADC 0 using strings
connect("GPIO_1", "ADC0")
```

### `disconnect(node1, node2)`
Removes a specific bridge between two nodes.

*   `node1`, `node2`: The two nodes to disconnect.
*   To remove all connections from a single node, set `node2` to `-1`.

**Example:**
```python
# Remove the bridge between rows 1 and 30
disconnect(1, 30)

# Remove all connections from GPIO_1
disconnect(GPIO_1, -1)
```

### `is_connected(node1, node2)`
Checks if a direct or indirect connection exists between two nodes.

*   Returns a custom `ConnectionState` object which evaluates to `True` if connected (`CONNECTED`) and `False` if not (`DISCONNECTED`).

**Example:**
```python
if is_connected(D13, TOP_RAIL):
    print("D13 is connected to the top rail.")

state = is_connected(1, 2)
print(state)  # Prints "CONNECTED" or "DISCONNECTED"
```

### `nodes_clear()`
Removes all connections from the board.

**Example:**
```python
nodes_clear()
print("All connections cleared.")
```

### `node(name_or_id)`
Creates a node object from a string name or integer ID. This is useful for storing a node reference in a variable.

**Example:**
```python
my_pin = node("D7")
led_pin = node(15)

connect(my_pin, led_pin)
oled_print(my_pin) # Displays 'D7' on the OLED
```

---

## DAC (Digital-to-Analog Converter)

Functions for controlling the analog voltage outputs.

### `dac_set(channel, voltage, [save=True])`
Sets the output voltage for a specific DAC channel.

*   `channel`: The DAC channel to set. Can be an integer (0-3) or a node constant (`DAC0`, `DAC1`, `TOP_RAIL`, `BOTTOM_RAIL`).
*   `voltage`: The desired voltage (from -8.0V to 8.0V).
*   `save` (optional): If `True` (default), the setting is saved to the config file.
*   **Aliases**: `set_dac()`

**Channels:**
*   `0` or `DAC0`: The 5V tolerant DAC output.
*   `1` or `DAC1`: The 8V tolerant DAC output.
*   `2` or `TOP_RAIL`: The top power rail.
*   `3` or `BOTTOM_RAIL`: The bottom power rail.

**Example:**
```python
# Set the top rail to 5V
dac_set(TOP_RAIL, 5.0)

# Set DAC0 to 1.25V
set_dac(DAC0, 1.25)
```

### `dac_get(channel)`
Reads the currently set voltage for a DAC channel.

*   `channel`: The DAC channel to read.
*   Returns a float.
*   **Aliases**: `get_dac()`

**Example:**
```python
voltage = dac_get(TOP_RAIL)
print("Top Rail voltage: " + str(voltage))
```

---

## ADC (Analog-to-Digital Converter)

Functions for measuring analog voltages.

### `adc_get(channel)`
Reads the voltage from a specific ADC channel.

*   `channel`: The ADC channel to read (0-4).
*   Returns a float.
*   **Aliases**: `get_adc()`

**Channels:**
*   `0-3`: 8V tolerant ADC inputs.
*   `4`: 5V tolerant ADC input.

**Example:**
```python
voltage = adc_get(0)
print("ADC0 voltage: " + str(voltage))
```

---

## GPIO (General Purpose Input/Output)

Functions for controlling the digital I/O pins.

### `gpio_set(pin, value)`
Sets the output state of a GPIO pin.

*   `pin`: The GPIO pin number (1-10).
*   `value`: `True` for HIGH, `False` for LOW.
*   **Aliases**: `set_gpio()`

### `gpio_get(pin)`
Reads the state of a GPIO pin.

*   `pin`: The GPIO pin number (1-10).
*   Returns a `GPIOState` object (`HIGH`, `LOW`, or `FLOATING`).
*   **Aliases**: `get_gpio()`

### `gpio_set_dir(pin, direction)`
Sets the direction of a GPIO pin.

*   `pin`: The GPIO pin number (1-10).
*   `direction`: `True` for OUTPUT, `False` for INPUT.
*   **Aliases**: `set_gpio_dir()`

### `gpio_get_dir(pin)`
Reads the direction of a GPIO pin.

*   `pin`: The GPIO pin number (1-10).
*   Returns a `GPIODirection` object (`INPUT` or `OUTPUT`).
*   **Aliases**: `get_gpio_dir()`

### `gpio_set_pull(pin, pull)`
Configures the internal pull resistor for a GPIO pin.

*   `pin`: The GPIO pin number (1-10).
*   `pull`: `1` for PULLUP, `-1` for PULLDOWN, `0` for NONE.
*   **Aliases**: `set_gpio_pull()`

### `gpio_get_pull(pin)`
Reads the pull resistor configuration of a GPIO pin.

*   `pin`: The GPIO pin number (1-10).
*   Returns a `GPIOPull` object (`PULLUP`, `PULLDOWN`, or `NONE`).
*   **Aliases**: `get_gpio_pull()`

**Pinout:**
*   `1-8`: Routable GPIO pins `GPIO_1` to `GPIO_8`.
*   `9`: `UART_TX`.
*   `10`: `UART_RX`.

**Example:**
```python
# Set GPIO 1 as an output and turn it on
gpio_set_dir(1, True)
gpio_set(1, True)

# Set GPIO 2 as an input with a pull-up
gpio_set_dir(2, False)
gpio_set_pull(2, 1)

# Read the state of GPIO 2
state = gpio_get(2)
if state == HIGH:
    print("GPIO 2 is HIGH")
```

---

## PWM (Pulse-Width Modulation)

Functions for generating PWM signals on GPIO pins.

### `pwm(pin, [frequency], [duty_cycle])`
Sets up and starts a PWM signal on a GPIO pin.

*   `pin`: The GPIO pin to use (1-8).
*   `frequency` (optional): The PWM frequency in Hz (10.0 to 62,500,000.0). Defaults to 1000.
*   `duty_cycle` (optional): The duty cycle from 0.0 to 1.0. Defaults to 0.5.
*   **Aliases**: `set_pwm()`

### `pwm_set_duty_cycle(pin, duty_cycle)`
Changes the duty cycle of an existing PWM signal.

*   `pin`: The GPIO pin number (1-8).
*   `duty_cycle`: The new duty cycle (0.0 to 1.0).
*   **Aliases**: `set_pwm_duty_cycle()`

### `pwm_set_frequency(pin, frequency)`
Changes the frequency of an existing PWM signal.

*   `pin`: The GPIO pin number (1-8).
*   `frequency`: The new frequency in Hz.
*   **Aliases**: `set_pwm_frequency()`

### `pwm_stop(pin)`
Stops the PWM signal on a GPIO pin.

*   `pin`: The GPIO pin number (1-8).
*   **Aliases**: `stop_pwm()`

**Example:**
```python
# Start a 1kHz, 25% duty cycle PWM on GPIO_1
pwm(GPIO_1, 1000, 0.25)

# Change the duty cycle to 75%
pwm_set_duty_cycle(GPIO_1, 0.75)

# Stop the PWM signal
pwm_stop(GPIO_1)
```

---

## INA (Current/Power Monitor)

Functions for reading data from the INA219 current sensors.

### `ina_get_current(sensor)`
Reads the current in Amps.
*   `sensor`: The sensor to read (0 or 1).
*   **Aliases**: `get_current()`

### `ina_get_voltage(sensor)`
Reads the shunt voltage in Volts.
*   `sensor`: The sensor to read (0 or 1).
*   **Aliases**: `get_voltage()`

### `ina_get_bus_voltage(sensor)`
Reads the bus voltage in Volts.
*   `sensor`: The sensor to read (0 or 1).
*   **Aliases**: `get_bus_voltage()`

### `ina_get_power(sensor)`
Reads the power in Watts.
*   `sensor`: The sensor to read (0 or 1).
*   **Aliases**: `get_power()`

**Example:**
```python
current_mA = ina_get_current(0) * 1000
print("Current: " + str(current_mA) + " mA")
```

---

## OLED Display

Functions for controlling the onboard OLED display.

### `oled_print(text, [size=2])`
Displays text on the OLED screen. It can print strings, numbers, and custom Jumperless types.

*   `text`: The content to display.
*   `size` (optional): The font size (1 or 2). Defaults to 2.

### `oled_clear()`
Clears the OLED display.

### `oled_show()`
Refreshes the OLED display to show the latest changes. (Note: Often not needed as `oled_print` handles this).

### `oled_connect()`
Connects the I2C lines to the OLED display.

### `oled_disconnect()`
Disconnects the I2C lines from the OLED display.

**Example:**
```python
oled_connect()
oled_print("Hello!")
time.sleep(2)
oled_clear()
oled_disconnect()
```

---

## Probe

Functions for interacting with the physical probe.

### `probe_read([blocking=True])`
Reads the pad currently being touched by the probe.

*   `blocking` (optional): If `True` (default), the function will wait until a pad is touched. If `False`, it returns immediately.
*   Returns a `ProbePad` object (e.g., `25`, `D13_PAD`, `NO_PAD`).
*   **Aliases**: `read_probe()`, `probe_read_blocking()`, `probe_read_nonblocking()`, `probe_wait()`, `wait_probe()`, `probe_touch()`, `wait_touch()`

### `probe_button([blocking=True])`
Reads the state of the buttons on the probe.

*   `blocking` (optional): If `True` (default), waits for a button press. If `False`, returns the current state immediately.
*   Returns a `ProbeButton` object (`CONNECT_BUTTON`, `REMOVE_BUTTON`, or `BUTTON_NONE`).
*   **Aliases**: `get_button()`, `button_read()`, `read_button()`, `probe_button_blocking()`, `probe_button_nonblocking()`, `check_button()`, `button_check()`

**Example:**
```python
print("Touch a pad...")
pad = probe_read()
print("You touched: " + str(pad))

if pad == D13_PAD:
    print("That's the Arduino LED pin!")

print("Press a probe button...")
button = get_button()
if button == CONNECT_BUTTON:
    print("Connect button pressed.")
```

---

<!-- ## Clickwheel

Functions for simulating clickwheel actions.

### `clickwheel_up([clicks=1])`
Simulates scrolling the clickwheel up.

### `clickwheel_down([clicks=1])`
Simulates scrolling the clickwheel down.

### `clickwheel_press()`
Simulates pressing the clickwheel button.

--- -->

## System Functions

### `arduino_reset()`
Resets the connected Arduino Nano.

### `run_app(appName)`
Launches a built-in Jumperless application.

*   `appName`: The name of the app to run (e.g., "File Manager", "I2C Scan").

---

## Status Functions

These functions print detailed status information to the serial console.

*   `print_bridges()`: Prints all active bridges.
*   `print_paths()`: Prints all resolved paths between nodes.
*   `print_crossbars()`: Prints the raw state of the crossbar matrix.
*   `print_nets()`: Prints the current net list.
*   `print_chip_status()`: Prints the status of the CH446Q chips.

---

## Help Functions

### `help()`
Displays a comprehensive list of all available functions and constants in the `jumperless` module.

### `nodes_help()`
Displays a detailed reference for all available node names and their aliases. 