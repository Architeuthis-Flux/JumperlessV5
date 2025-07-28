# UI Refresh Implementation for Channel Configuration Changes

## Overview

This implementation adds automatic UI refresh functionality when users enable or disable channels in the Jumperless device. The system now properly notifies the UI about device capability changes (like maximum sample count) that occur when the firmware recalculates buffer sizes based on the active channel configuration.

## Problem Solved

Previously, when users enabled or disabled channels, the firmware would recalculate the optimal buffer size and maximum sample count, but the UI would not refresh to show these updated capabilities. Users had to manually reconnect or restart the application to see the new limits.

## Solution Components

### 1. Firmware Changes (`LogicAnalyzer.cpp`)

**Critical Race Condition Fix:**
- Fixed `JUMPERLESS_CMD_SET_MODE` command to process configuration completely before responding
- Added `updateBufferConfiguration()` call before sending status response
- Ensures firmware sends updated header after all processing is complete
- Prevents race condition where UI briefly sees changes then reverts

**Enhanced Debug Output:**
- Added max sample count to debug messages for better visibility
- Shows calculated limits for both digital-only and mixed-signal modes

**Improved Documentation:**
- Added comments explaining that updated headers trigger UI refresh via `sr_session_send_meta()`

### 2. UI Widget Changes (`jumperlessconfig.cpp`)

**Channel Change Detection:**
- Added tracking of previous channel configuration (`last_digital_enabled_`, `last_analog_enabled_`)
- Enhanced `update_channel_status()` to detect actual configuration changes
- Implemented automatic header request when changes are detected

**Header Request Mechanism:**
- Added `request_device_header_update()` function
- Triggers device mode re-configuration to force channel update
- Uses standard libsigrok configuration mechanism to communicate with driver

### 3. Driver Changes (`protocol.c`)

**Dynamic Capability Notification:**
- Added capability change detection after receiving updated device headers
- Implemented `sr_session_send_meta()` calls to notify UI about:
  - Maximum memory depth changes (`SR_CONF_LIMIT_SAMPLES`)
  - Maximum sample rate changes (`SR_CONF_SAMPLERATE`)
  - Current sample limit adjustments when they exceed new maximums

**Key Implementation:**
```c
/* Store old values for comparison */
uint64_t old_max_memory_depth = devc->max_memory_depth;
uint64_t old_max_sample_rate = devc->max_sample_rate;

/* Process the updated header */
ret = jlms_receive_header(sdi);
if (ret == SR_OK) {
    /* Notify UI about capability changes if they occurred */
    if (old_max_memory_depth != devc->max_memory_depth) {
        sr_session_send_meta(sdi, SR_CONF_LIMIT_SAMPLES,
                            g_variant_new_uint64(devc->max_memory_depth));
    }
    
    /* Auto-adjust current limits if they exceed new maximum */
    if (devc->limit_samples > devc->max_memory_depth) {
        devc->limit_samples = devc->max_memory_depth;
        sr_session_send_meta(sdi, SR_CONF_LIMIT_SAMPLES,
                            g_variant_new_uint64(devc->limit_samples));
    }
}
```

### 4. API Changes (`api.c`)

**Dynamic Limit Validation:**
- Updated `config_set()` to use device-reported maximums instead of static constants
- Updated `config_list()` to report dynamic maximums when device context is available

**Before:**
```c
if (tmp_u64 < MIN_NUM_SAMPLES || tmp_u64 > MAX_NUM_SAMPLES)
    return SR_ERR_ARG;
```

**After:**
```c
uint64_t max_samples = (devc->max_memory_depth > 0) ? devc->max_memory_depth : MAX_NUM_SAMPLES;
if (tmp_u64 < MIN_NUM_SAMPLES || tmp_u64 > max_samples)
    return SR_ERR_ARG;
```

## How It Works

### Complete End-to-End Flow:

1. **User Action:** User enables/disables channels in PulseView channel configuration

2. **UI Widget Detection:** 
   - JumperlessConfig widget detects channel changes every 500ms via `update_channel_status()`
   - Compares current channel configuration with previous state
   - When changes detected, calls `request_device_header_update()`

3. **UI Trigger:** 
   - Widget re-sends current device mode via `sr_dev->config_set(ConfigKey::DEVICE_MODE)`
   - This triggers the driver to process channel configuration changes

4. **Driver Communication:** 
   - Driver sends `JUMPERLESS_CMD_SET_CHANNELS` to firmware
   - Firmware is informed of new channel configuration

5. **Firmware Processing:**
   - Firmware recalculates optimal buffer size based on active channels
   - Updates `jl_la_max_samples` and other capabilities  
   - Sends status response + updated header with new capabilities

6. **Driver Processing:**
   - Driver receives updated header with new `max_memory_depth`
   - Compares with previous values to detect changes
   - Sends `sr_session_send_meta()` notifications to UI for changed capabilities
   - Automatically adjusts current limits if they exceed new maximums

7. **UI Update:** 
   - PulseView receives meta updates and refreshes capability displays
   - Sample limit fields automatically update to show new maximums
   - User sees real-time feedback without reconnecting device

## Testing

Two comprehensive test scripts have been provided:

### **Basic Channel Configuration Test:**
- **File:** `examples/test_ui_refresh_on_channel_change.py`
- **Purpose:** Tests libsigrok channel enable/disable and capability reporting
- **Usage:** `python examples/test_ui_refresh_on_channel_change.py`

### **Complete Integration Test:**
- **File:** `examples/test_ui_channel_refresh_integration.py` 
- **Purpose:** Tests complete end-to-end UI refresh flow including meta updates
- **Usage:** `python examples/test_ui_channel_refresh_integration.py`

### **Race Condition Fix Test:**
- **File:** `examples/test_mode_command_fix.py`
- **Purpose:** Verifies that SET_MODE command processes completely before responding
- **Usage:** `python examples/test_mode_command_fix.py`

**The integration test demonstrates:**
- UI widget channel change detection (simulated)
- Automatic header requests triggered by channel changes
- Firmware buffer recalculation and capability updates
- Driver meta notification system (`sr_session_send_meta()`)
- Real-time UI refresh without device reconnection

**Test scenarios covered:**
- Digital-only mode (highest sample count)
- Mixed-signal mode (medium sample count) 
- Analog-only mode (varies based on channel count)
- Selective analog channel enabling
- Meta update monitoring and validation

## Expected Behavior

### Channel Configuration Impact on Sample Limits:

1. **Digital-Only Mode:** Highest maximum sample count (~262K samples)
   - Only 1 byte per sample needed during capture
   - Maximum buffer utilization

2. **Mixed-Signal Mode:** Medium maximum sample count
   - Varies based on number of enabled analog channels
   - More analog channels = lower max sample count

3. **Analog-Only Mode:** Variable maximum sample count
   - Depends on specific analog channels enabled
   - Generally lower than digital-only

### UI Behavior:
- Sample limit fields automatically update when channels change
- Invalid sample counts are automatically adjusted downward
- User sees real-time feedback of available memory/samples
- No need to reconnect device or restart application

## Benefits

1. **Improved User Experience:** Immediate feedback on capability changes
2. **Accurate Limits:** UI always shows current device capabilities
3. **Automatic Adjustment:** Invalid settings are corrected automatically
4. **Transparent Operation:** Changes happen seamlessly in the background

## Implementation Notes

- Uses standard libsigrok meta notification system (`sr_session_send_meta()`)
- Backward compatible with existing UI applications
- Graceful fallback to static limits when device context unavailable
- Comprehensive logging for debugging capability changes

This implementation ensures that users always see accurate, up-to-date device capabilities that reflect the current channel configuration, providing a much more intuitive and responsive user experience. 