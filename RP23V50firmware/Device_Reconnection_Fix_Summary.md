# Device Reconnection and Crash Fix Summary

## Problem Description
PulseView was crashing when Jumperless devices were disconnected during operation, with the following error sequence:
1. Device gets disconnected during use
2. `jlms_send_command()` fails with "Device not configured" error
3. PulseView tries to close the device connection
4. `sigrok::Device::close()` throws a C++ exception
5. Application crashes with `SIGABRT`

## Root Cause
The crash occurred because:
1. **LibSigrok Driver**: No USB disconnection detection or reconnection logic
2. **Device Close Operations**: No graceful handling of already-disconnected devices
3. **Exception Propagation**: C++ exceptions from libsigrok were not caught by PulseView
4. **No Recovery Mechanism**: Failed operations resulted in immediate crashes instead of recovery attempts

## Solution Overview
Implemented a comprehensive 4-layer protection system:

### Layer 1: USB Device Detection and Reconnection (LibSigrok Driver)

**Files Modified:**
- `libsigrok-falaj/src/hardware/jumperless-mixed-signal/protocol.h`
- `libsigrok-falaj/src/hardware/jumperless-mixed-signal/protocol.c`
- `libsigrok-falaj/src/hardware/jumperless-mixed-signal/api.c`

**Key Functions Added:**
```c
// USB device management
SR_PRIV gboolean jlms_check_usb_device_present(const char *device_path);
SR_PRIV int jlms_attempt_device_reconnection(struct sr_dev_inst *sdi);
SR_PRIV gboolean jlms_is_device_error_recoverable(int error_code);
```

**Features:**
- **Device Presence Checking**: Verifies USB device file exists and is accessible
- **Error Classification**: Identifies recoverable vs non-recoverable errors
- **Automatic Reconnection**: Attempts reconnection with exponential backoff (5 retries, 1s intervals)
- **State Reset**: Properly resets device context during reconnection
- **Device Re-identification**: Ensures device is properly identified after reconnection

### Layer 2: Enhanced Command Error Handling

**Modified Functions:**
- `jlms_send_command()`: Detects USB disconnection and returns `SR_ERR_IO`
- `jlms_configure_timing()`: Attempts reconnection when timing configuration fails

**Error Recovery Flow:**
```
Command Fails → Check if USB Disconnection → Attempt Reconnection → Retry Command → Return Result
```

### Layer 3: Graceful Device Close Operations

**Enhanced `dev_close()` Function:**
- **Null Pointer Safety**: Handles cases where device context or connection is NULL
- **USB Presence Check**: Verifies device is present before sending close commands  
- **Graceful Cleanup**: Continues cleanup even if individual operations fail
- **No Fatal Errors**: Device close operations never fail fatally

**Protection Against:**
- Attempting to close already-disconnected devices
- Sending commands to non-existent devices
- Serial port errors during cleanup
- Resource leaks during failed close operations

### Layer 4: PulseView Exception Handling

**Files Modified:**
- `julseview/pv/session.cpp`
- `julseview/pv/toolbars/mainbar.cpp`

**Exception Handling Added:**
```cpp
// In Session::set_device()
try {
    device_->close();
} catch (const std::exception& e) {
    qWarning() << "Exception while closing device (device may be disconnected):" << e.what();
} catch (...) {
    qWarning() << "Unknown exception while closing device (device may be disconnected)";
}

// In Session::select_device() and MainBar::on_actionConnect_triggered()
try {
    session_.select_device(dlg.get_selected_device());
} catch (const std::exception &e) {
    QMessageBox::warning(this, tr("Device Connection Error"), ...);
} catch (...) {
    QMessageBox::warning(this, tr("Device Connection Error"), ...);
}
```

## User Experience Improvements

### Before Fix:
- ❌ **Immediate Crash**: Application terminated with SIGABRT
- ❌ **Data Loss**: All unsaved work lost
- ❌ **No Recovery**: Required application restart
- ❌ **No User Feedback**: Cryptic crash reports only

### After Fix:
- ✅ **Graceful Handling**: User-friendly error messages
- ✅ **Automatic Recovery**: Device reconnection attempts
- ✅ **Application Stability**: No crashes from device disconnection
- ✅ **Clear Feedback**: Informative messages about connection status
- ✅ **Continued Operation**: Can reconnect devices without restart

## Error Messages and User Feedback

**LibSigrok Level:**
```
sr: jumperless-mixed-signal: Device appears to be disconnected
sr: jumperless-mixed-signal: Attempting to reconnect to device /dev/cu.usbmodemJLV5port5...
sr: jumperless-mixed-signal: Device successfully reconnected after 2 attempts
```

**PulseView Level:**
```
Dialog: "Device Connection Error"
Message: "Failed to connect to device. Device may be disconnected or busy."
```

## Testing Scenarios Covered

1. **Device Disconnection During Operation**: Device unplugged while running
2. **Device Disconnection During Connection**: Device unplugged during connect dialog
3. **Device Disconnection Before Close**: Device already gone when PulseView tries to close
4. **Repeated Disconnection/Reconnection**: Multiple disconnect/reconnect cycles
5. **Connection Dialog Errors**: Robust handling of connection failures

## Technical Implementation Details

### USB Device Detection Algorithm:
1. Check if device file exists using `stat()`
2. Attempt to open device file with `O_RDWR | O_NONBLOCK`
3. Immediately close if successful (non-destructive test)
4. Return presence status

### Reconnection Strategy:
1. **Immediate Detection**: Detect disconnection on first failed command
2. **Clean Slate**: Reset all device context and close existing connections
3. **Retry Loop**: Up to 5 attempts with 1-second intervals
4. **Full Verification**: Re-identify device after reconnection
5. **Graceful Failure**: Clear error messages if reconnection fails

### Exception Safety:
- **RAII Principles**: Proper resource cleanup in all error paths
- **No Throw Guarantee**: Critical cleanup operations never throw
- **Exception Neutrality**: Exceptions caught at appropriate boundaries
- **User Experience**: All exceptions converted to user-friendly messages

## Compilation and Integration

The solution maintains full backward compatibility and requires no changes to:
- Build configuration
- External dependencies  
- User workflows
- Existing device drivers

**Forward Compatibility:**
- New USB management functions can be used by other drivers
- Exception handling patterns can be applied to other device types
- Reconnection logic is generic and reusable

## Verification and Validation

**Verified Scenarios:**
- ✅ Normal operation with connected device
- ✅ Device disconnection during acquisition
- ✅ Device disconnection during configuration  
- ✅ Device reconnection after disconnection
- ✅ Multiple rapid disconnect/reconnect cycles
- ✅ Connection attempts to non-existent devices
- ✅ Application shutdown with disconnected devices

This comprehensive solution transforms device disconnection from a fatal crash into a recoverable error condition, significantly improving the robustness and user experience of PulseView with Jumperless devices. 