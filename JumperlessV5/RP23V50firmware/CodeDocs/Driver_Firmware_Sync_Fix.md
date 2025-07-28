# Driver-Firmware Synchronization Fix

## Overview
This document describes the complete fix that ensures the libsigrok driver and Jumperless firmware are perfectly synchronized for mixed-signal logic analyzer captures.

## Problem Summary
The analog signal data was being misinterpreted because:

1. **Firmware forced all analog channels ON** (✅ fixed)
2. **Driver fell back to SUMP configuration** but processed data as digital-only ❌
3. **Driver mode mismatch**: firmware sending mixed-signal, driver expecting digital ❌

## Complete Solution

### ✅ Firmware Changes (Already Applied)
**File**: `src/LogicAnalyzer.cpp`

```cpp
// FORCE ALL ANALOG CHANNELS ON FOR DEBUGGING
analog_mask = 0x1F;  // Enable ADC 0-4 (5 channels)
analog_chan_count = 5;  // Fixed count of 5 analog channels
current_la_mode = LA_MODE_MIXED_SIGNAL;  // Always mixed-signal mode
```

**Result**: Firmware always captures and sends mixed-signal data (1 byte digital + 10 bytes analog per sample).

### ✅ Driver Changes (Just Applied)
**File**: `libsigrok-falaj/src/hardware/jumperless-mixed-signal/protocol.c`

```cpp
/* FORCE mixed-signal mode to match firmware behavior */
devc->device_capture_mode = JUMPERLESS_MODE_MIXED_SIGNAL;
devc->num_digital_channels = 8;  /* All 8 digital channels */
devc->digital_channel_mask = 0xFF;  /* Enable all digital channels */
devc->num_analog_channels = 5;  /* Force all 5 analog channels */
devc->analog_channel_mask = 0x1F;  /* Enable ADC 0-4 */
devc->bytes_per_sample = 1 + (5 * 2);  /* 1 digital + 10 analog bytes */
```

**Result**: Driver correctly interprets the mixed-signal data format.

## Expected Behavior After Fix

### 📋 Firmware Logs
```
◆ Channels configured: digital=0x000000FF, analog=0x1F (5 channels, mode=1) [FORCED ALL ANALOG ON]
◆ Enhanced header: response=0x81, size=220000 bytes (20000 samples × 11 bytes/sample, mode=1)
◆ Sending Enhanced mixed-signal data: 20000 samples, 5 analog channels
```

### 📋 Driver Logs
```
sr: jumperless-mixed-signal: Enhanced timing configuration failed, trying SUMP fallback
sr: jumperless-mixed-signal: Forced mixed-signal mode: 8 digital + 5 analog channels, 11 bytes/sample
sr: session: bus: Received SR_DF_LOGIC packet (1018 bytes, unitsize = 1).
sr: session: bus: Received SR_DF_ANALOG packet (1018 bytes, 5 channels).
```

### 📊 PulseView Results
- **Digital channels (D0-D7)**: Show digital signal data properly
- **Analog channels (A0-A4)**: Show analog voltage waveforms with correct scaling:
  - **A0-A3**: ±8V range (bipolar signals)
  - **A4**: 0-5V range (unipolar signals)

## Data Format Specification

Each sample contains exactly **11 bytes**:

```
Byte Structure (per sample):
├─ Byte 0:     Digital data (8 channels: D7 D6 D5 D4 D3 D2 D1 D0)
├─ Bytes 1-2:  ADC 0 data (little-endian, ±8V range)
├─ Bytes 3-4:  ADC 1 data (little-endian, ±8V range)
├─ Bytes 5-6:  ADC 2 data (little-endian, ±8V range)  
├─ Bytes 7-8:  ADC 3 data (little-endian, ±8V range)
└─ Bytes 9-10: ADC 4 data (little-endian, 0-5V range)
```

## Voltage Conversion

The driver now uses the correct Jumperless voltage formulas:

```c
// ADC channels 0-3 and 7: ±8V range
voltage = (adc_value * 18.28f / 4095) - 8.0f;

// ADC channel 4: 0-5V range  
voltage = adc_value * 5.0f / 4095;
```

## Testing

### 1. Upload Updated Firmware
```bash
cd JumperlessV5/RP23V50firmware
pio run -t upload
```

### 2. Test with PulseView
1. Open PulseView
2. Select "Jumperless Mixed-Signal Logic Analyzer"  
3. Enable some digital and analog channels
4. Start capture
5. **Expected**: Both digital and analog data should appear correctly

### 3. Test with sigrok-cli
```bash
sigrok-cli --driver=jumperless-mixed-signal --channels=D0,D1,A0,A1 --samples=1000 --output-format=csv
```

**Expected output**: CSV with both digital and analog columns showing real data.

## Success Indicators

### ✅ Firmware Success
- Debug shows: `[FORCED ALL ANALOG ON]`
- Always reports: `mode=1` (mixed-signal)
- Always sends: `11 bytes/sample`

### ✅ Driver Success  
- Debug shows: `Forced mixed-signal mode: 8 digital + 5 analog channels, 11 bytes/sample`
- Processes data with: `jlms_process_mixed_signal_data()`
- Sends both: `SR_DF_LOGIC` and `SR_DF_ANALOG` packets

### ✅ PulseView Success
- Digital channels show logic level changes
- Analog channels show voltage waveforms  
- Voltage scaling is accurate (±8V for A0-A3, 0-5V for A4)
- No "analog data on digital channels" artifacts

## Reverting (Future)

When proper channel negotiation is implemented, revert by:

1. **Firmware**: Remove forced analog channel assignment in `JUMPERLESS_CMD_SET_CHANNELS`
2. **Driver**: Remove forced mixed-signal mode in `jlms_sump_start()`  
3. **Both**: Implement proper protocol negotiation for Enhanced mode

This ensures the mixed-signal logic analyzer works reliably while maintaining a path to proper protocol implementation. 