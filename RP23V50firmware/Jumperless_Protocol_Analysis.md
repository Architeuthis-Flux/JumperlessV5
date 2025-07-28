# Jumperless Mixed-Signal Logic Analyzer Protocol Analysis

## Overview

This document analyzes the communication protocol between the libsigrok driver (`jumperless-mixed-signal`) and the Jumperless firmware, explaining what each side sends and receives and what they expect.

## Protocol Mismatch Issues

Based on the logs and code analysis, there are several critical mismatches between what the driver expects and what the firmware provides:

### 1. Channel Count Mismatch

**Driver Expectations:**
- `JUMPERLESS_MAX_DIGITAL_CHANNELS = 8` (defined in protocol.h)
- `JUMPERLESS_MAX_ANALOG_CHANNELS = 5` (defined in protocol.h)
- Driver creates 8 digital channels (GPIO 1-8) and 5 analog channels (ADC 0-4)

**Firmware Reality:**
- Firmware reports `max_digital_channels = 2` in header
- Firmware reports `max_analog_channels = 0` in header
- Firmware actually supports 8 digital channels and 14 analog channels

**Impact:** Driver disables all analog channels because firmware reports 0 analog channels available.

### 2. Header Structure Analysis

**Driver Expects (88 bytes):**
```c
typedef struct {
    char magic[8];                      // "$JLDATA\0"
    uint8_t version;                    // Protocol version (2)
    uint8_t capture_mode;               // Current mode (0=digital, 1=mixed, 2=analog)
    uint8_t max_digital_channels;       // Maximum digital channels (8)
    uint8_t max_analog_channels;        // Maximum analog channels (5)
    uint32_t sample_rate;               // Current sample rate (Hz)
    uint32_t sample_count;              // Number of samples to capture
    uint32_t digital_channel_mask;      // Available digital channels (0xFF)
    uint32_t analog_channel_mask;       // Enabled analog channels
    uint8_t bytes_per_sample;           // Total bytes per sample
    uint8_t digital_bytes_per_sample;   // Digital bytes per sample (1)
    uint8_t analog_bytes_per_sample;    // Analog bytes per sample (N*2)
    uint8_t adc_resolution_bits;        // ADC resolution (12 bits)
    uint32_t trigger_channel_mask;      // Trigger channels (future)
    uint32_t trigger_pattern;           // Trigger pattern (future)
    uint32_t trigger_edge_mask;         // Edge trigger mask (future)
    uint32_t pre_trigger_samples;       // Pre-trigger samples (future)
    float analog_voltage_range;         // ADC voltage range (18.28V)
    uint64_t max_sample_rate;           // Maximum sample rate (50MHz)
    uint64_t max_memory_depth;          // Maximum memory depth
    uint8_t supports_triggers;          // Supports triggers (boolean)
    uint8_t supports_compression;       // Supports compression (boolean)
    uint8_t supported_modes;            // Supported modes bitmask
    char firmware_version[16];          // Firmware version string
    char device_id[16];                 // Device identifier
    uint32_t checksum;                  // Header checksum (XOR)
} __attribute__((packed)) jumperless_header;
```

**Firmware Code Sets (107 bytes):**
```c
struct {
    char magic[8];
    uint8_t version;
    uint8_t capture_mode;
    uint8_t max_digital_channels;       // **SETS: 8** (correct)
    uint8_t max_analog_channels;        // **SETS: 14** (wrong - driver expects 5)
    uint32_t sample_rate;
    uint32_t sample_count;
    uint32_t digital_channel_mask;
    uint32_t analog_channel_mask;
    uint8_t bytes_per_sample;
    uint8_t digital_bytes_per_sample;
    uint8_t analog_bytes_per_sample;
    uint8_t adc_resolution_bits;
    uint32_t trigger_channel_mask;
    uint32_t trigger_pattern;
    uint32_t trigger_edge_mask;
    uint32_t pre_trigger_samples;
    float analog_voltage_range;
    uint64_t max_sample_rate;
    uint64_t max_memory_depth;
    uint8_t supports_triggers;
    uint8_t supports_compression;
    uint8_t supported_modes;
    char firmware_version[16];
    char device_id[16];
    uint32_t checksum;
} __attribute__((packed)) header;
```

**Firmware Logs Show (88 bytes received):**
```
sr: jumperless-mixed-signal: max_digital_channels: 2 (was: 2)
sr: jumperless-mixed-signal: max_analog_channels: 0 (was: 0)
```

**🔍 KEY DISCOVERY:** The firmware **CODE** sets correct values (8 digital, 14 analog), but the **LOGS** show wrong values (2 digital, 0 analog). This suggests a **parsing or transmission issue** rather than a code issue.

**Size Mismatch:** Driver expects 88 bytes, firmware sends 107 bytes (19 bytes larger).

### 3. Detailed Header Comparison

| Field | Driver Expects | Firmware Sets | Driver Receives | Status |
|-------|---------------|---------------|-----------------|---------|
| **Structure Size** | 88 bytes | 107 bytes | 88 bytes | ❌ Size mismatch |
| **max_digital_channels** | 8 | 8 | 2 | ❌ Parsing error |
| **max_analog_channels** | 5 | 14 | 0 | ❌ Both wrong |
| **version** | 2 | 2 | 65 | ❌ ASCII vs binary |
| **magic** | "$JLDATA\0" | "$JLDATA" | Unknown | ❓ Not logged |

**🔍 CRITICAL INSIGHTS:**
1. **Firmware code is correct** - it sets the right values
2. **Driver parsing is wrong** - it reads wrong values
3. **19-byte size difference** suggests byte offset misalignment
4. **Version 65** suggests ASCII 'A' instead of binary 2
5. **Channel counts** suggest wrong byte positions being read

### 3. Command/Response Protocol

## Enhanced Protocol Commands

### Command Format
All commands follow this format:
```
[Command Byte] [Data Length] [Data...]
```

### Command Definitions

| Command | Value | Description | Data Format |
|---------|-------|-------------|-------------|
| `JUMPERLESS_CMD_RESET` | 0x00 | Reset device | None |
| `JUMPERLESS_CMD_RUN` | 0x01 | Start capture | None |
| `JUMPERLESS_CMD_ID` | 0x02 | Get device ID | None |
| `JUMPERLESS_CMD_GET_HEADER` | 0x03 | Get device capabilities | None |
| `JUMPERLESS_CMD_SET_CHANNELS` | 0x04 | Configure channels | 8 bytes (4 digital + 4 analog mask) |
| `JUMPERLESS_CMD_ARM` | 0x05 | Arm device for trigger | None |
| `JUMPERLESS_CMD_GET_STATUS` | 0x06 | Get device status | None |
| `JUMPERLESS_CMD_CONFIGURE` | 0x07 | Configure timing | 8 bytes (rate + samples) |
| `JUMPERLESS_CMD_SET_SAMPLES` | 0x08 | Set sample count | 4 bytes |
| `JUMPERLESS_CMD_SET_MODE` | 0x09 | Set capture mode | 1 byte |

### Response Format
All responses follow this format:
```
[Response Type] [Length] [Data...]
```

### Response Types

| Response | Value | Description | Data Format |
|----------|-------|-------------|-------------|
| `JUMPERLESS_RESP_ID` | 0x7F | Device ID response | "1SLO" |
| `JUMPERLESS_RESP_HEADER` | 0x80 | Device capabilities | Header structure |
| `JUMPERLESS_RESP_DATA` | 0x81 | Sample data | Sample data |
| `JUMPERLESS_RESP_STATUS` | 0x82 | Status response | 1 byte status |
| `JUMPERLESS_RESP_ERROR` | 0x83 | Error response | 1 byte error code |

## Communication Flow

### 1. Device Detection
**Driver → Firmware:**
- Sends `JUMPERLESS_CMD_ID` (0x02)

**Firmware → Driver:**
- Responds with `JUMPERLESS_RESP_ID` (0x7F) + "1SLO"

### 2. Device Identification
**Driver → Firmware:**
- Sends `JUMPERLESS_CMD_GET_HEADER` (0x03)

**Firmware → Driver:**
- Responds with `JUMPERLESS_RESP_HEADER` (0x80) + Header structure
- **ISSUE:** Firmware reports wrong channel counts (2 digital, 0 analog)

### 3. Channel Configuration
**Driver → Firmware:**
- Sends `JUMPERLESS_CMD_SET_CHANNELS` (0x04) + 8 bytes:
  - Bytes 0-3: Digital channel mask (little-endian)
  - Bytes 4-7: Analog channel mask (little-endian)

**Firmware → Driver:**
- Responds with `JUMPERLESS_RESP_STATUS` (0x82) + status byte
- **ISSUE:** Firmware returns status 0x01 (error) instead of 0x00 (OK)

### 4. Timing Configuration
**Driver → Firmware:**
- Sends `JUMPERLESS_CMD_CONFIGURE` (0x07) + 8 bytes:
  - Bytes 0-3: Sample rate (little-endian)
  - Bytes 4-7: Sample count (little-endian)

**Firmware → Driver:**
- Responds with `JUMPERLESS_RESP_STATUS` (0x82) + status byte
- **ISSUE:** Firmware returns status 0x01 (error) instead of 0x00 (OK)

### 5. Device Arming
**Driver → Firmware:**
- Sends `JUMPERLESS_CMD_ARM` (0x05)

**Firmware → Driver:**
- Responds with `JUMPERLESS_RESP_STATUS` (0x82) + status byte
- **ISSUE:** Firmware returns status 0x01 (error) instead of 0x00 (OK)

### 6. Data Acquisition
**Driver → Firmware:**
- Sends `JUMPERLESS_CMD_RUN` (0x01)

**Firmware → Driver:**
- Responds with `JUMPERLESS_RESP_DATA` (0x81) + sample data
- **ISSUE:** Never reaches this point due to arming failure

## Data Format Issues

### Sample Data Format
**Driver Expects:**
- Digital-only: 3 bytes per sample
- Mixed-signal: 32 bytes per sample (always)
- Format: `[GPIO][UART][Marker][Analog Data...][EOF]`

**Firmware Sends:**
- Digital-only: 3 bytes per sample
- Mixed-signal: 32 bytes per sample (always)
- Format: `[GPIO][UART][0xDA][14×ADC][2×DAC][2×INA0][2×INA1][0xA0]`

**Issue:** Driver expects 5 analog channels, firmware provides 14 channels.

## Status Code Issues

### Expected Status Codes
- `0x00`: Success
- `0x01`: Insufficient data
- `0x02`: Invalid state
- `0xFF`: Unknown command

### Actual Firmware Responses
- Firmware consistently returns `0x01` for all commands
- This indicates "Insufficient data" error
- Suggests firmware expects different data format or timing

## Root Cause Analysis

### 1. Header Size Mismatch
The firmware header structure is 19 bytes larger than expected, causing parsing issues.

### 2. Channel Count Reporting - CRITICAL DISCOVERY
**The firmware CODE sets correct values, but LOGS show wrong values:**

**Firmware Code Sets:**
- `max_digital_channels = 8` ✅ (correct)
- `max_analog_channels = 14` ❌ (driver expects 5)

**Driver Logs Show:**
- `max_digital_channels = 2` ❌ (wrong)
- `max_analog_channels = 0` ❌ (wrong)

**This indicates a parsing or transmission issue, not a code issue.**

### 3. Status Response Issues
Firmware returns error status (0x01) for all commands, indicating:
- Data format mismatch
- Timing issues
- State machine problems

### 4. Protocol Version Mismatch
- Driver expects protocol version 2
- Firmware sends version 65 (ASCII 'A')
- This suggests firmware may be sending ASCII data instead of binary

### 5. Header Parsing Issue
The 19-byte size difference suggests the driver is parsing the header incorrectly, possibly:
- Reading wrong byte offsets
- Misinterpreting data types
- Buffer overflow/underflow

## Recommendations

### 1. Fix Firmware Header
- Correct `max_digital_channels` to 8
- Correct `max_analog_channels` to 5
- Ensure header size matches driver expectations (88 bytes)
- Fix protocol version to 2

### 2. Fix Status Responses
- Return `0x00` (success) for successful commands
- Implement proper error handling and status codes

### 3. Fix Data Format
- Ensure sample data format matches driver expectations
- Limit analog channels to 5 as expected by driver
- Verify byte ordering (little-endian)

### 4. Add Protocol Validation
- Add checksum validation
- Add magic number validation
- Add response timeout handling

### 5. Improve Error Handling
- Add detailed error reporting
- Add state machine validation
- Add command validation

## Conclusion

The primary issues are:
1. **Header size mismatch** (107 vs 88 bytes)
2. **Incorrect channel count reporting** (2/0 vs 8/5)
3. **Consistent error status responses** (0x01 for all commands)
4. **Protocol version mismatch** (65 vs 2)

These issues prevent the driver from properly configuring the device and starting acquisition. The firmware needs to be updated to match the driver's expectations for successful communication. 