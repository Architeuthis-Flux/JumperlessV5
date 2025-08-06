# JulseView Decimation Mode Implementation

## Overview

This document describes the implementation of high-speed digital capture with decimated analog sampling in JulseView. This feature allows the device to capture digital signals at rates >5MHz while keeping analog sampling within the 200kHz hardware limit.

## Problem Statement

The original JulseView implementation had these limitations:
- ADC hardware limited to ~200kHz total sample rate (across all channels)
- Digital PIO can capture >5MHz but was constrained by analog rate
- Both digital and analog had to use the same sample count per buffer
- USB transfer rate limited ping-pong buffer switching

## Solution: Asymmetric Buffer Allocation with Decimation

### Key Features

1. **Automatic Decimation Detection**: When sample rate > 200kHz total, decimation mode is automatically enabled
2. **Asymmetric Buffer Layout**: Digital buffers increased to 16KB each (from 1KB), analog buffers remain 32KB each
3. **Separate Sample Counts**: Digital and analog can use different sample counts per buffer
4. **Sample Duplication**: Analog samples are duplicated to maintain timing alignment with digital samples

### Buffer Layout

```
Total Buffer: 96KB
Layout: [abuf0 32KB][abuf1 32KB][dbuf0 16KB][dbuf1 16KB]

Buffer Sizes:
- Analog buffers:  32KB each (64KB total)
- Digital buffers: 16KB each (32KB total) - 16x increase from original

Sample Capacity (with 5 analog channels):
- Analog:  32KB / (5 channels × 2 bytes) = 3,276 samples per buffer
- Digital: 16KB / 1 byte = 16,384 samples per buffer
```

## Implementation Details

### New Constants (JulseView.h)

```cpp
#define JULSEVIEW_ADC_MAX_RATE 200000  // Maximum safe ADC sample rate
#define JULSEVIEW_DECIMATION_MIN_FACTOR 1  // Minimum decimation factor
#define JULSEVIEW_DECIMATION_MAX_FACTOR 100  // Maximum decimation factor
#define JULSEVIEW_DIGITAL_BUF_SIZE 32768   // 32KB for digital (16KB each for ping-pong)
#define JULSEVIEW_DMA_BUF_SIZE JULSEVIEW_ANALOG_BUF_SIZE + JULSEVIEW_DIGITAL_BUF_SIZE  // Dynamic total buffer size
```

### New Member Variables (JulseView.h)

```cpp
// --- Decimation Configuration ---
bool use_decimation_mode;              // Whether we're in high-speed decimation mode
uint32_t analog_decimation_factor;     // How many digital samples per analog sample
uint32_t digital_samples_per_half;     // Digital samples per buffer (larger)
uint32_t analog_samples_per_half;      // Analog samples per buffer (smaller)
uint32_t last_analog_sample_index;     // Index of last analog sample for duplication
```

### New Methods (JulseView.cpp)

1. **`configure_decimation_mode()`**: Determines if decimation is needed and calculates factor
2. **`calculate_asymmetric_buffer_layout()`**: Sets up buffer layout for decimation mode
3. **`send_slices_analog_decimated()`**: Sends data with decimation logic
4. **`duplicate_analog_sample()`**: Duplicates analog sample data for timing alignment

### Integration Points

#### arm() Function Modifications

1. **Decimation Configuration**: Added call to `configure_decimation_mode()` after sample rate validation
2. **Buffer Layout**: Replaced fixed buffer layout with call to `calculate_asymmetric_buffer_layout()`
3. **Sample Count Calculation**: Updated to use asymmetric sample counts in decimation mode
4. **DMA Transfer Counts**: Modified to use separate digital and analog transfer counts
5. **ADC Rate Calculation**: Updated to use decimated rate when in decimation mode

#### send_slices_analog() Function Modifications

- Added check at beginning to call `send_slices_analog_decimated()` when in decimation mode
- Maintains backward compatibility for normal mode

## Usage Examples

### Normal Mode (≤200kHz)
```
Sample Rate: 100kHz
Behavior: Normal mode, no decimation
Digital: 100kHz
Analog: 100kHz
```

### High-Speed Mode (>200kHz)
```
Sample Rate: 500kHz
Behavior: Decimation mode enabled
Digital: 500kHz (full rate)
Analog: 166kHz (decimated by factor of 3)
Result: Each analog sample duplicated 3 times
```

### Ultra-High Mode (>>200kHz)
```
Sample Rate: 2MHz
Behavior: Decimation mode enabled
Digital: 2MHz (full rate)
Analog: 200kHz (decimated by factor of 10)
Result: Each analog sample duplicated 10 times
```

## Testing

### Test Scenarios

1. **Normal Mode Test**: Sample rate ≤ 200kHz
   - Verify no decimation occurs
   - Verify digital and analog use same sample count

2. **Decimation Mode Test**: Sample rate > 200kHz
   - Verify decimation mode is enabled
   - Verify digital samples at full rate
   - Verify analog samples at reduced rate
   - Verify sample duplication occurs

3. **Buffer Layout Test**: Verify asymmetric buffer allocation
   - Check digital buffer size increase
   - Verify analog buffer size unchanged
   - Confirm total buffer size remains 72KB

### Test Commands

```bash
# Build the firmware
pio run

# Test with serial terminal
# Send these commands to test decimation mode:
R10000   # Normal mode test
R500000  # High-speed mode test
R2000000 # Ultra-high mode test
```

## Future Enhancements

### Protocol Optimization
- Add protocol flag for decimation mode
- Implement driver-side sample duplication
- Reduce USB bandwidth usage

### Buffer Size Optimization
- Increase total buffer size to 96KB or 128KB
- Allow larger digital buffers for higher decimation factors
- Implement dynamic buffer allocation based on sample rate

### Advanced Decimation
- Implement interpolation instead of simple duplication
- Add configurable decimation factors
- Support different decimation factors per analog channel

## Debug Output

The implementation includes comprehensive debug output:

```
=== CONFIGURING DECIMATION MODE ===
DECIMATION ENABLED: sample_rate=500000 Hz > 200000 Hz limit
DECIMATION FACTOR: 3 (each analog sample duplicated 3 times)
ACTUAL ADC RATE: 166666 Hz (digital at 500000 Hz)

=== CALCULATING ASYMMETRIC BUFFER LAYOUT ===
DECIMATION BUFFER LAYOUT:
  abuf0: 0x00000000 - 0x00007FFF (32768 bytes)
  abuf1: 0x00008000 - 0x0000FFFF (32768 bytes)
  dbuf0: 0x00010000 - 0x00010FFF (4096 bytes)
  dbuf1: 0x00011000 - 0x00011FFF (4096 bytes)

=== SENDING DECIMATED DATA ===
Digital samples: 4096, Analog samples: 3276, Decimation factor: 3
Sample 0: NEW analog data (idx=0)
Sample 1: DUPLICATED analog data (from idx=0)
Sample 2: DUPLICATED analog data (from idx=0)
Sample 3: NEW analog data (idx=1)
```

## Conclusion

The decimation mode implementation successfully addresses the original limitations by:

1. **Enabling High-Speed Digital Capture**: Digital signals can now be captured at >5MHz
2. **Maintaining Analog Compatibility**: Analog sampling stays within 200kHz hardware limits
3. **Preserving Timing Relationships**: Sample duplication maintains proper timing alignment
4. **Ensuring Backward Compatibility**: Normal mode behavior unchanged for existing users

This implementation provides a solid foundation for high-speed logic analysis while maintaining the mixed-signal capabilities that make JulseView unique. 