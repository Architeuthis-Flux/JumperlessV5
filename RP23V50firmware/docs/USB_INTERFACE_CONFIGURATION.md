# USB Interface Configuration Guide

## Quick Start - Choose Your Configuration

Edit `include/usb_interface_config.h` and set the values below based on what you want to test:

### **Recommended Configurations for Testing:**

## Option 1: Minimal (2 CDC Interfaces) - **RECOMMENDED FIRST**
```c
#define USB_CDC_ENABLE_COUNT     2    // Main + Arduino
#define USB_MSC_ENABLE           0    // Disabled
#define USB_HID_ENABLE_COUNT     0    // Disabled
#define USB_MIDI_ENABLE          0    // Disabled
#define USB_VENDOR_ENABLE        0    // Disabled
```
**Result:** Main Serial + Arduino Serial (what you already have working)

## Option 2: Add Mass Storage (3 total interfaces)
```c
#define USB_CDC_ENABLE_COUNT     2    // Main + Arduino
#define USB_MSC_ENABLE           1    // Add mass storage
#define USB_HID_ENABLE_COUNT     0    // Disabled
#define USB_MIDI_ENABLE          0    // Disabled
#define USB_VENDOR_ENABLE        0    // Disabled
```
**Result:** Main Serial + Arduino Serial + Mass Storage

## Option 3: Full CDC (4 CDC interfaces)
```c
#define USB_CDC_ENABLE_COUNT     4    // Main + Arduino + Routable + Debug
#define USB_MSC_ENABLE           0    // Disabled
#define USB_HID_ENABLE_COUNT     0    // Disabled
#define USB_MIDI_ENABLE          0    // Disabled
#define USB_VENDOR_ENABLE        0    // Disabled
```
**Result:** 4 Serial interfaces

## Option 4: Everything (Maximum)
```c
#define USB_CDC_ENABLE_COUNT     4    // All CDC interfaces
#define USB_MSC_ENABLE           1    // Mass storage
#define USB_HID_ENABLE_COUNT     2    // 2 HID devices
#define USB_MIDI_ENABLE          1    // MIDI device
#define USB_VENDOR_ENABLE        1    // Custom vendor interface
```
**Result:** All possible interfaces (may have stability issues)

---

## How It Works

### Interface Types Explained:

- **CDC (Serial)**: Communication Device Class - appears as `/dev/cu.usbmodem*` on Mac
  - **Interface 0**: Main Serial (always enabled)
  - **Interface 1**: Arduino Serial (USBSer1) 
  - **Interface 2**: Routable Serial (USBSer2)
  - **Interface 3**: Debug Serial (USBSer3)

- **MSC (Mass Storage)**: Appears as removable drive for file storage

- **HID (Human Interface Device)**: For mouse/keyboard/custom HID functionality

- **MIDI**: For MIDI device functionality

- **Vendor**: Custom protocol interface

### Expected USB Device Names:
- **Interface 0**: "JL Main Serial"
- **Interface 1**: "JL Arduino Serial" 
- **Interface 2**: "JL Routable Serial"
- **Interface 3**: "JL Debug Serial"
- **MSC**: "JL Mass Storage"
- **HID**: "JL HID Device"
- **MIDI**: "JL MIDI Device"
- **Vendor**: "JL Vendor Device"

---

## Testing Process

1. **Start with Option 1** (2 CDC) to ensure the dynamic system works
2. **Increment gradually** - add one interface type at a time
3. **Test each configuration** before moving to the next
4. **Check enumeration** after each change: `ls /dev/cu.usbmodem*`

### After Each Configuration Change:
```bash
pio run --target clean && pio run --target upload
```

### Check Results on Mac:
```bash
# Check for multiple serial interfaces
ls /dev/cu.usbmodem*

# Check USB device information
system_profiler SPUSBDataType | grep -A 10 "Jumperless"
```

---

## Troubleshooting

**If device doesn't enumerate:**
- Try reducing interface count
- Check serial output for error messages
- Device may need manual BOOTSEL mode (hold BOOTSEL while connecting)

**If you get build errors:**
- Current code may need USBSer2/USBSer3 conditional compilation fixes
- Start with Option 1 (2 CDC) which should build cleanly

**If interfaces don't appear:**
- Check that custom descriptors are being called (debug messages)
- Try disconnect/reconnect USB cable
- Some interfaces may need specific drivers

---

## Current Status

◆ **Working**: 2 CDC interfaces with linker wrapping approach
⟐ **In Progress**: Dynamic configuration system
△ **Known Issue**: Code needs conditional compilation fixes for USBSer2/USBSer3 references 