# Jumperless V5 Control Channels Guide

## Overview

The Jumperless V5 driver now supports **Control Channels** - a new class of traces that can monitor and control internal firmware signals. Unlike traditional analog and digital channels that read external signals, control channels provide access to internal firmware state and control signals.

## Features

### Enhanced Channel Support
- **Analog Channels**: Up to 8 channels (ADC_0-ADC_7)
- **Digital Channels**: Up to 8 channels (GPIO_0-GPIO_7) 
- **Control Channels**: Up to 16 channels (C0-C15)
- **Default Configuration**: 8 Digital + 5 Analog channels

### Channel Organization

All channels are organized into logical groups for better PulseView display:

#### Analog Channels
- **Group**: "ADC Channels"
- **Channels**: ADC_0 through ADC_7 (all 8 channels in one group)
- **Display**: Horizontal layout with checkboxes
- **Default Names**: ADC_0, ADC_1, ADC_2, ADC_3, ADC_4 (0-5V), Pad_Sense, Supply_Monitor, Probe_Measure

#### Digital Channels
- **Group**: "GPIO Channels"
- **Channels**: GPIO_1 through GPIO_8 (all 8 channels in one group)
- **Display**: Horizontal layout with checkboxes
- **Default Names**: GPIO_1, GPIO_2, GPIO_3, GPIO_4, GPIO_5, GPIO_6, GPIO_7, GPIO_8

#### Control Channels
- **Group 1**: "Digital Control Channels" (C0-C7)
  - C0: UART_TX
  - C1: UART_RX
  - C2: Python_D1
  - C3: Python_D2
  - C4: Python_D3
  - C5: Python_D4
  - C6: RP_6
  - C7: RP_7

- **Group 2**: "Analog Control Channels" (C8-C15)
  - C8: DAC_0
  - C9: DAC_1
  - C10: Python_A1
  - C11: Python_A2
  - C12: Python_A3
  - C13: Python_A4
  - C14: Top_Rail
  - C15: Bottom_Rail

### Control Channel Types

Control channels can be used to monitor and control:

1. **DAC Output Values** (C0, C1)
   - Real-time monitoring of DAC0 and DAC1 output values
   - Useful for closed-loop control applications

2. **I2C Communication Data** (C2)
   - Monitor I2C bus activity and data
   - Debug I2C communication issues

3. **MicroPython Triggers** (C3)
   - Trigger captures from MicroPython code
   - Synchronize firmware events with data capture

4. **Internal Firmware State** (C4-C15)
   - Monitor internal variables and state
   - Debug firmware behavior

## Usage

### Enabling Control Channels

#### Via Python API
```python
import sigrok.core as sr

# Open device
dev = ctx.scan()[0]  # Get first Jumperless device
dev.open()

# Enable 4 control channels
dev.config_set(sr.ConfigKey.CONTROL_CHANNELS, 4)

# Enable specific control channels
for ch in dev.channels:
    if ch.name.startswith('C'):
        ch.enabled = True
```

#### Via PulseView
1. Open PulseView
2. Connect to Jumperless device
3. In device settings, set "Control Channels" to desired number (0-16)
4. Enable individual control channels in the channel list

#### Via sigrok-cli
```bash
# Enable 4 control channels
sigrok-cli -d jumperless --config control_channels=4

# Start capture with control channels
sigrok-cli -d jumperless --config control_channels=4 -c samplerate=10000 -c limit_samples=1000
```

### Control Channel Commands

The firmware supports these control channel commands:

- `E<n>` - Enable n control channels (0-16)
- `C<enable><channel>` - Enable/disable specific control channel
  - Example: `C10` enables C0, `C01` disables C1
- `N<channel><name>` - Update control channel name from firmware
  - Example: `N0MySensor` renames C0 to "MySensor"

### Dynamic Channel Naming

All channel names can be updated dynamically from the Jumperless firmware:

#### From MicroPython
```python
import jumperless as jl

# Update control channel names
jl.update_channel_name(0, "MySensor")    # Rename C0
jl.update_channel_name(8, "VoltageRail") # Rename C8

# Update analog channel names  
jl.update_channel_name(0, "Temperature") # Rename ADC_0
jl.update_channel_name(1, "Pressure")    # Rename ADC_1

# Update digital channel names
jl.update_channel_name(0, "Button1")     # Rename GPIO_0
jl.update_channel_name(1, "LED1")        # Rename GPIO_1
```

#### From Firmware Commands
```bash
# Update control channel names via serial commands
N0MySensor     # Rename C0 to "MySensor"
N8VoltageRail  # Rename C8 to "VoltageRail"
N0Temperature  # Rename ADC_0 to "Temperature"
N0Button1      # Rename GPIO_0 to "Button1"
```

### Data Format

Control channels use the same data format as analog channels:
- 12-bit ADC resolution
- 16-bit DMA transfers
- Values range from 0-4095 (12-bit)

### Example Applications

#### 1. DAC Monitoring
```python
# Monitor DAC output values
dev.config_set(sr.ConfigKey.CONTROL_CHANNELS, 2)  # Enable C0, C1
# C0 will show DAC0 value, C1 will show DAC1 value
```

#### 2. I2C Debugging
```python
# Monitor I2C communication
dev.config_set(sr.ConfigKey.CONTROL_CHANNELS, 1)  # Enable C2
# C2 will show I2C data activity
```

#### 3. MicroPython Integration
```python
# In MicroPython firmware
import jumperless as jl

# Set trigger value that will appear on C3
jl.set_control_channel(3, 2048)  # Set C3 to mid-scale

# In Python host code
dev.config_set(sr.ConfigKey.CONTROL_CHANNELS, 4)  # Enable C0-C3
# C3 will show the trigger value from MicroPython
```

## Technical Details

### Driver Changes
- Added `MAX_CONTROL_CHANNELS = 16` constant
- Extended `dev_context` structure with control channel support
- Added `SR_CONF_CONTROL_CHANNELS` configuration option
- Implemented control channel enable/disable logic

### Firmware Integration
- Control channels use existing ADC infrastructure
- Data transmitted alongside analog/digital channels
- No impact on existing channel performance

### Performance Considerations
- Control channels add minimal overhead
- Data rate depends on enabled channel count
- Recommended for debugging and monitoring, not high-speed applications

## Future Enhancements

Planned control channel features:
- **Digital Control Channels**: Boolean state monitoring
- **Custom Data Types**: Support for different data formats
- **Bidirectional Control**: Write values to control channels
- **Event Logging**: Timestamped control channel events

## Troubleshooting

### Common Issues

1. **Control channels not appearing**
   - Ensure firmware supports control channels
   - Check that `SR_CONF_CONTROL_CHANNELS` is set correctly

2. **No data on control channels**
   - Verify firmware is sending control channel data
   - Check channel enable masks

3. **Performance issues**
   - Reduce number of enabled control channels
   - Lower sample rate if needed

### Debug Commands
```bash
# Check driver version and capabilities
sigrok-cli --show

# List available configuration options
sigrok-cli -d jumperless --show

# Test control channel configuration
sigrok-cli -d jumperless --config control_channels=4 --test
```

## Examples

See `control_channels_demo.py` for a complete working example of control channel usage. 