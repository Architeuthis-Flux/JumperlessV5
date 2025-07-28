# Jumperless Unified Data Format Guide

## Overview

The new unified data format eliminates alignment issues and provides a self-describing, robust format for both digital and mixed-signal data transmission. This format is used by both firmware and driver implementations.

## Format Structure

### Digital-Only Mode
**3 bytes per sample:**
```
[byte 0] GPIO data (8 digital channels, bits 0-7)
[byte 1] UART data (currently unused, set to 0x00)
[byte 2] Format marker: 0xDD (DIGITAL_ONLY_MARKER)
```

### Mixed-Signal Mode
**32 bytes per sample:**
```
[bytes 0-2]  Digital header (same as digital-only, but marker = 0xDA)
[bytes 3-30] Analog data (14 channels × 2 bytes each, little-endian)
[byte 31]    EOF marker: 0xA0 (ANALOG_EOF_MARKER)
```

### Analog-Only Mode
**32 bytes per sample:**
```
[bytes 0-2]  Digital header (dummy data, marker = 0xAA)
[bytes 3-30] Analog data (14 channels × 2 bytes each, little-endian)
[byte 31]    EOF marker: 0xA0 (ANALOG_EOF_MARKER)
```

## Format Markers

| Marker | Value | Meaning | Next Bytes |
|--------|-------|---------|------------|
| `DIGITAL_ONLY_MARKER` | 0xDD | Digital-only sample | 0 (sample complete) |
| `MIXED_SIGNAL_MARKER` | 0xDA | Mixed-signal sample | 29 (28 analog + 1 EOF) |
| `ANALOG_ONLY_MARKER` | 0xAA | Analog-only sample | 29 (28 analog + 1 EOF) |
| `ANALOG_EOF_MARKER` | 0xA0 | End of analog data | N/A |

## Analog Channel Mapping

The 14 analog channels in mixed-signal/analog modes map as follows:

| Channel | Type | Description | Voltage Range |
|---------|------|-------------|---------------|
| 0-3, 7  | ADC | General purpose ADCs | ±8V |
| 4       | ADC | Special purpose ADC | 0-5V |
| 5-6     | ADC | Probe monitoring | ±8V |
| 8-9     | DAC | DAC outputs | ±8V |
| 10      | INA | INA219 #0 voltage | ±8V |
| 11      | INA | INA219 #0 current | ±3.3V |
| 12      | INA | INA219 #1 voltage | ±8V |
| 13      | INA | INA219 #1 current | ±3.3V |

## Voltage Conversion Formulas

### ADC Channels 0-3, 7 (±8V range):
```
voltage = ((adc_value * 18.28) / 4095.0) - 8.0
```

### ADC Channel 4 (0-5V range):
```
voltage = (adc_value * 5.0) / 4095.0
```

### INA Current Channels (example):
```
voltage = ((adc_value * 3.3) / 4095.0) - 1.65
```

## Parsing Algorithm

### Basic Parser Logic:
```c
while (remaining >= 3) {
    uint8_t gpio = data[0];
    uint8_t uart = data[1];
    uint8_t marker = data[2];
    
    switch (marker) {
        case DIGITAL_ONLY_MARKER:
            // Process 3-byte digital sample
            process_digital(gpio);
            data += 3;
            remaining -= 3;
            break;
            
        case MIXED_SIGNAL_MARKER:
        case ANALOG_ONLY_MARKER:
            // Process 32-byte mixed/analog sample
            if (remaining >= 32 && data[31] == ANALOG_EOF_MARKER) {
                process_mixed_signal(gpio, &data[3]);
                data += 32;
                remaining -= 32;
            } else {
                // Error: incomplete sample or missing EOF
                break;
            }
            break;
            
        default:
            // Unknown marker - stop parsing
            break;
    }
}
```

## Benefits

### 1. **Simplified Alignment**
- No complex byte counting required
- Read 3 bytes, check marker, conditionally read more
- Self-synchronizing format

### 2. **Robust Error Detection**
- Format markers prevent misinterpretation
- EOF markers validate complete transmission
- Unknown markers stop parsing safely

### 3. **Efficient Processing**
- Single unified parser for all modes
- No mode-specific processing paths
- Minimal overhead (3 bytes vs 1 byte for digital)

### 4. **Future-Proof**
- Extensible with new marker types
- Backward compatible parsing
- Clear upgrade path

## Implementation Files

### Firmware (RP23V50firmware/):
- `src/LogicAnalyzer.cpp` - Main implementation
- `src/LogicAnalyzer.h` - Header declarations

### Driver (libsigrok-falaj/):
- `src/hardware/jumperless-mixed-signal/protocol.c` - Parser implementation
- `src/hardware/jumperless-mixed-signal/protocol.h` - Function declarations

### Testing:
- `examples/test_new_data_format.py` - Validation script

## Migration Notes

### From Previous Format:
1. ✅ Eliminated variable `bytes_per_sample` calculations
2. ✅ Removed complex mode-specific processing
3. ✅ Added robust error detection and recovery
4. ✅ Simplified driver implementation

### Compatibility:
- ✅ Maintains all existing channel mappings
- ✅ Preserves voltage conversion formulas
- ✅ Supports all capture modes (digital, mixed, analog)
- ✅ Works with both SUMP and Enhanced protocols

## Testing Results

```
🧪 Testing new unified Jumperless data format
==================================================
Testing digital-only format...
  ✅ Digital-only format test passed!

Testing mixed-signal format...
  ✅ Mixed-signal format test passed!

Testing format validation...
  ✅ Format validation tests passed!

🎉 All tests passed! New format is working correctly.
```

## Summary

The unified data format successfully resolves previous alignment issues while maintaining full compatibility with existing channel configurations and voltage scaling. The self-describing nature of the format makes it robust against transmission errors and simplifies both firmware and driver implementations. 