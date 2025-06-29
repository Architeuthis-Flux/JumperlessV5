# USB Mass Storage with QSPI Flash - Jumperless V5

This document describes the USB Mass Storage implementation for Jumperless V5, which uses the onboard 16MB QSPI flash chip to provide USB drive functionality.

## Overview

The Jumperless V5 can present itself as a USB Mass Storage device (USB drive) to allow easy file transfer and management. This functionality uses the onboard QSPI flash memory and is based on:

- **Adafruit SPIFlash Library**: For QSPI flash access
- **SdFat Library**: For FAT filesystem support  
- **TinyUSB MSC**: For USB Mass Storage Class implementation
- **Custom Integration**: Seamless switching between normal operation and USB mode

## Features

- ✅ **Real Flash Storage**: Uses actual 16MB QSPI flash (not just emulated)
- ✅ **FAT Filesystem**: Standard FAT filesystem for compatibility
- ✅ **Proper USB Protocol**: Full SCSI/MSC compliance for broad OS support
- ✅ **Safe Mode Switching**: Device becomes safely unresponsive during USB mode
- ✅ **Automatic Detection**: Proper USB enumeration and device identification
- ✅ **File Monitoring**: Real-time monitoring of flash contents
- ✅ **Error Handling**: Comprehensive error reporting and recovery

## Hardware Requirements

- **Jumperless V5 Board** with RP2350B microcontroller
- **16MB QSPI Flash** on default QSPI pins (automatically detected)
- **USB Connection** to host computer

## Usage

### 1. Basic Commands

```cpp
// Test the flash integration
testUSBFlashIntegration();

// Enter USB Mass Storage mode
if (enterUSBFilesystemMode()) {
    // Device is now a USB drive - becomes unresponsive
    while (handleUSBFilesystemMode()) {
        // Handle USB communications
        // Will exit when drive is ejected
    }
    exitUSBFilesystemMode(); // Resume normal operation
}

// Check current status
printFlashStatus();
printUSBFilesystemStatus();
```

### 2. Command Line Interface

If integrated with the Jumperless command system:

```
help usb              # Show USB-related commands
usbfs                 # Enter USB filesystem mode
usbfs test            # Test flash integration
usbfs status          # Show current status
```

### 3. First Time Setup

When you first use the USB Mass Storage functionality:

1. **Connect to Computer**: The device will enumerate as "Jumperless V5"
2. **Format if Needed**: If the flash isn't formatted, your OS may prompt to format it
3. **Choose FAT32**: Select FAT32 for best compatibility
4. **Use Normally**: Copy files to/from the drive like any USB storage device

## Technical Implementation

### Flash Transport Layer

```cpp
// RP2350B uses same flash that stores the firmware
Adafruit_FlashTransport_RP2040 flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
```

### File System Integration

- **SdFat Library**: Handles FAT filesystem operations
- **Sector-Level Access**: Direct 512-byte sector read/write
- **Cache Management**: Automatic sector caching for performance
- **Mount/Unmount**: Proper filesystem state management

### USB Mass Storage Callbacks

All required MSC callbacks are implemented:

- `tud_msc_inquiry_cb()`: Device identification
- `tud_msc_test_unit_ready_cb()`: Ready status checking  
- `tud_msc_capacity_cb()`: Flash size reporting
- `tud_msc_read10_cb()`: Sector reading
- `tud_msc_write10_cb()`: Sector writing
- `tud_msc_scsi_cb()`: SCSI command handling

## Configuration

### PlatformIO Dependencies

```ini
lib_deps = 
    adafruit/Adafruit SPIFlash
    adafruit/SdFat - Adafruit Fork
```

### USB Interface Configuration

In `include/usb_interface_config.h`:

```cpp
#define USB_MSC_ENABLE           1    // Enable Mass Storage
#define USB_CDC_ENABLE_COUNT     3    // Keep other interfaces
```

### Build Flags

The implementation automatically uses the correct flash transport for RP2350B.

## Safety and Data Integrity

### Mode Switching

- **Exclusive Access**: Only one mode can access flash at a time
- **Proper Unmounting**: Filesystem is properly closed before USB access
- **State Tracking**: Full state management prevents conflicts
- **Ejection Handling**: Proper ejection detection and cleanup

### Data Protection

- **Sector Validation**: All sector operations are bounds-checked
- **Write Protection**: Read-only mode when not in USB Mass Storage
- **Error Handling**: Comprehensive error detection and reporting
- **Cache Synchronization**: Automatic cache flushing

## Troubleshooting

### Flash Not Detected

```
◇ Failed to initialize QSPI flash!
```

**Solutions:**
- Check QSPI connections
- Verify flash chip compatibility  
- Check power supply stability

### Filesystem Not Formatted

```
△ No FAT filesystem found - flash may need formatting
```

**Solutions:**
- Connect to computer and format as FAT32
- Use disk utility to check partition table
- Try `testUSBFlashIntegration()` for detailed diagnostics

### USB Not Enumerating

```
◇ Failed to prepare filesystem for USB mode
```

**Solutions:**
- Check USB cable connection
- Verify USB configuration in `usb_interface_config.h`
- Try different USB port or computer
- Check serial output for detailed error messages

### Performance Issues

- **Large Files**: May be slow due to flash limitations
- **Many Small Files**: Consider archiving for better performance
- **Wear Leveling**: Flash has limited write cycles

## API Reference

### Flash Management Functions

```cpp
bool initializeUSBFlash(void);              // Initialize flash hardware
void deinitializeUSBFlash(void);            // Cleanup flash resources
bool isUSBFlashInitialized(void);           // Check initialization status
bool isUSBFileSystemFormatted(void);        // Check filesystem status
void printFlashStatus(void);                // Print detailed status
```

### USB Mode Functions

```cpp
bool enterUSBFilesystemMode(void);          // Enter USB MSC mode
bool handleUSBFilesystemMode(void);         // Handle USB communications  
void exitUSBFilesystemMode(void);           // Exit USB MSC mode
bool isInUSBFilesystemMode(void);           // Check current mode
```

### Monitoring Functions

```cpp
void monitorUSBFlashContents(void);         // List flash contents
void markUSBFileSystemChanged(void);        // Force content refresh
void testUSBFlashIntegration(void);         // Comprehensive test
```

### Low-Level Flash Functions

```cpp
int32_t readFlashBlocks(uint32_t lba, void* buffer, uint32_t bufsize);
int32_t writeFlashBlocks(uint32_t lba, const uint8_t* buffer, uint32_t bufsize);
bool syncFlashBlocks(void);
bool getFlashCapacity(uint32_t* block_count, uint16_t* block_size);
```

## Integration Notes

### With Existing Jumperless Features

- **Command System**: Can be integrated with existing menu system
- **Display Updates**: Shows USB mode status on OLED
- **Serial Output**: Comprehensive logging and debugging
- **State Management**: Properly coordinates with other subsystems

### Memory Usage

- **Flash Transport**: ~1KB RAM for transport layer
- **SdFat Library**: ~4KB RAM for filesystem buffers
- **USB Buffers**: ~2KB RAM for USB packet handling
- **Total Overhead**: ~7KB RAM (acceptable for RP2350B)

## Example Usage Patterns

### File Transfer Workflow

```cpp
// 1. Enter USB mode
Serial.println("Connecting as USB drive...");
if (enterUSBFilesystemMode()) {
    Serial.println("USB drive active - copy files now");
    
    // 2. Handle USB until ejected
    while (handleUSBFilesystemMode()) {
        // Device is unresponsive, USB handles everything
    }
    
    // 3. Exit USB mode
    exitUSBFilesystemMode();
    Serial.println("USB drive disconnected - resuming normal operation");
    
    // 4. Check what was added/changed
    monitorUSBFlashContents();
}
```

### Development/Testing Workflow

```cpp
// Test everything before using
testUSBFlashIntegration();

// Check status anytime
printFlashStatus();
printUSBFilesystemStatus();

// Manual flash operations (if needed)
uint8_t buffer[512];
if (readFlashBlocks(0, buffer, 512) > 0) {
    // Process boot sector data
}
```

This implementation provides a robust, production-ready USB Mass Storage solution for your Jumperless V5 device! 