# Driver and Firmware Updates Summary

## Overview
This document summarizes the changes made to align the libsigrok driver and Jumperless firmware for proper communication.

## Changes Made

### 1. Driver Updates (libsigrok-falaj)

#### Protocol Header (protocol.h)
- **Updated channel limits:**
  - `JUMPERLESS_MAX_DIGITAL_CHANNELS`: 8 → 16
  - `JUMPERLESS_MAX_ANALOG_CHANNELS`: 5 → 14
- **Added default channel constants:**
  - `JUMPERLESS_DEFAULT_DIGITAL_CHANNELS`: 8
  - `JUMPERLESS_DEFAULT_ANALOG_CHANNELS`: 5
- **Updated command offsets:**
  - All commands offset by 0xA0 to avoid conflicts
  - `JUMPERLESS_CMD_RESET`: 0x00 → 0xA0
  - `JUMPERLESS_CMD_RUN`: 0x01 → 0xA1
  - `JUMPERLESS_CMD_ID`: 0x02 → 0xA2
  - `JUMPERLESS_CMD_GET_HEADER`: 0x03 → 0xA3
  - `JUMPERLESS_CMD_SET_CHANNELS`: 0x04 → 0xA4
  - `JUMPERLESS_CMD_ARM`: 0x05 → 0xA5
  - `JUMPERLESS_CMD_GET_STATUS`: 0x06 → 0xA6
  - `JUMPERLESS_CMD_CONFIGURE`: 0x07 → 0xA7
  - `JUMPERLESS_CMD_SET_SAMPLES`: 0x08 → 0xA8
  - `JUMPERLESS_CMD_SET_MODE`: 0x09 → 0xA9
  - `JUMPERLESS_CMD_SET_TRIGGER`: 0x0A → 0xAA
  - `JUMPERLESS_CMD_CLEAR_TRIGGER`: 0x0B → 0xAB
- **Updated header structure:**
  - Size: 88 bytes → 107 bytes (matches firmware)
  - `max_digital_channels`: 8 → 16
  - `max_analog_channels`: 5 → 14

#### API Configuration (api.c)
- **Updated channel creation:**
  - Uses `JUMPERLESS_DEFAULT_DIGITAL_CHANNELS` (8) instead of max
  - Uses `JUMPERLESS_DEFAULT_ANALOG_CHANNELS` (5) instead of max
  - Creates 8 digital channels (GPIO 1-8) by default
  - Creates 5 analog channels (ADC 0-4) by default

#### Protocol Handling (protocol.c)
- **Updated header parsing:**
  - Expects 107 bytes instead of 88 bytes
  - Handles larger header structure correctly
- **Updated hardcoded command values:**
  - Changed `\x02` to `\xA2` for ID command
  - Changed `\x03` to `\xA3` for GET_HEADER command
- **Added auto-RUN functionality:**
  - When no triggers are configured, automatically starts acquisition after ARM
  - Checks `trigger_channel_mask` and `trigger_mask` for trigger configuration

### 2. Firmware Updates (RP23V50firmware)

#### Command Definitions (LogicAnalyzer.h)
- **Updated command offsets:**
  - All commands offset by 0xA0 to match driver
  - `JL_CMD_RESET`: 0x00 → 0xA0
  - `JL_CMD_RUN`: 0x01 → 0xA1
  - `JL_CMD_ID`: 0x02 → 0xA2
  - `JL_CMD_GET_HEADER`: 0x03 → 0xA3
  - `JL_CMD_SET_CHANNELS`: 0x04 → 0xA4
  - `JL_CMD_ARM`: 0x05 → 0xA5
  - `JL_CMD_GET_STATUS`: 0x06 → 0xA6
  - `JL_CMD_CONFIGURE`: 0x07 → 0xA7
  - `JL_CMD_SET_SAMPLES`: 0x08 → 0xA8
  - `JL_CMD_SET_MODE`: 0x09 → 0xA9
  - `JL_CMD_SET_TRIGGER`: 0x0A → 0xAA
  - `JL_CMD_CLEAR_TRIGGER`: 0x0B → 0xAB

#### Command Processing (LogicAnalyzer.cpp)
- **Updated command handling:**
  - Uses `JL_CMD_` constants instead of `JUMPERLESS_CMD_`
  - All command cases updated to new values
- **Updated command definitions:**
  - All `JUMPERLESS_CMD_*` constants updated to 0xA0-0xAB range
  - Updated command range checks to use 0xA0-0xAB
  - Updated debug messages to reflect new command values
- **Updated header reporting:**
  - `max_digital_channels`: 8 → 16 (matches driver expectations)
  - `max_analog_channels`: 14 (matches actual hardware capability)
- **Added auto-RUN functionality:**
  - When `trigger_mode == TRIGGER_NONE`, automatically starts capture after ARM
  - Calls `setupCapture()` and `startCapture()` immediately
  - Provides debug output for auto-capture status

#### Additional Firmware Files Updated
- **LogicAnalyzernew.cpp:** Updated command definitions and range checks
- **LogicAnalyzerCurrent.cpp:** Updated command range checks

## Key Improvements

### 1. Protocol Compatibility
- **Command offsets** prevent conflicts with other Jumperless commands
- **Header size matching** ensures proper parsing
- **Channel count alignment** between driver and firmware

### 2. User Experience
- **Auto-RUN functionality** eliminates need for separate RUN command when no triggers are set
- **Default channel configuration** provides sensible defaults (8 digital, 5 analog)
- **Maximum capability reporting** allows future expansion

### 3. Error Handling
- **Proper status responses** for all commands
- **Debug output** for troubleshooting
- **State validation** before operations

## Expected Behavior

### 1. Device Detection
1. Driver sends `0xA2` (ID command)
2. Firmware responds with "1SLO"
3. Driver sends `0xA3` (GET_HEADER command)
4. Firmware sends 107-byte header with correct channel counts

### 2. Channel Configuration
1. Driver sends `0xA4` (SET_CHANNELS) with 8 bytes (digital + analog masks)
2. Firmware responds with `0x82` + `0x00` (success)
3. Firmware sends updated header

### 3. Acquisition Start
1. Driver sends `0xA5` (ARM command)
2. Firmware responds with `0x82` + `0x00` (success)
3. If no triggers: Firmware automatically starts capture
4. If triggers: Firmware waits for trigger condition

### 4. Command Range
- **Enhanced Jumperless Commands:** 0xA0-0xAB
- **SUMP Commands:** 0x80-0x82 (unchanged)
- **Other Commands:** 0xC0-0xDF (unchanged)

### 5. Data Format Standardization
- **ALWAYS use 32-byte mixed-signal format** (never digital-only)
- **Header framing:** SOH (0xAF) + 107-byte header + EOH (0xBF)
- **Sample format:** 3 bytes digital + 29 bytes analog per sample
- **Eliminates parsing errors** from format switching and misalignment

## Testing Recommendations

### 1. Basic Communication
- Verify device detection and header reception
- Check channel configuration with different masks
- Test status response handling

### 2. Auto-RUN Functionality
- Test ARM command with no triggers (should auto-start)
- Test ARM command with triggers (should wait)
- Verify capture data transmission

### 3. Error Conditions
- Test invalid commands
- Test insufficient data scenarios
- Test state validation

## Notes

- The driver now creates 8 digital and 5 analog channels by default
- The firmware reports 16 digital and 14 analog channels as maximum capability
- All commands are offset by 0xA0 to avoid conflicts
- Auto-RUN eliminates the need for a separate RUN command in most cases
- Header size is now 107 bytes on both sides

## Latest Updates (2024-12-19)

### Standardized 32-byte Mixed-Signal Format
- **Driver**: Modified `api.c` to always set 32 bytes per sample and force mixed-signal mode
- **Firmware**: Modified `LogicAnalyzer.cpp` to always use `LA_MODE_MIXED_SIGNAL` and send 32-byte samples
- **Result**: Both sides now consistently use the same 32-byte format regardless of channel configuration

### Header Framing with SOH/EOH
- **Driver**: Updated `protocol.h` to define `JUMPERLESS_HEADER_SOH 0xAF` and `JUMPERLESS_HEADER_EOH 0xBF`
- **Driver**: Modified `jlms_receive_header` to expect and validate SOH/EOH framing
- **Firmware**: Modified `sendJumperlessHeader` to send SOH + header + EOH sequence
- **Result**: Reliable header parsing with proper byte alignment

### Fixed Analog Channel Index Calculation
- **Driver**: Updated `protocol.c` to use name-based lookup with `jumperless_analog_names` array instead of index arithmetic
- **Driver**: Fixed analog channel mask to always enable 5 analog channels (0x1F) for mixed-signal format
- **Firmware**: Modified buffer allocation to handle zero analog channels gracefully
- **Result**: Proper analog channel detection and buffer allocation 