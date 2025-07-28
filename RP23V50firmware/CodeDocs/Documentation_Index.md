# Jumperless Documentation Index

Welcome to the complete documentation for the Jumperless breadboard automation system. This documentation covers all aspects of using, programming, and extending the Jumperless platform.

## 1. Getting Started

### [Getting Started Guide](Getting_Started_Guide.md)
**Essential reading for new users.** This guide covers hardware setup, first connections, basic Python scripting, and troubleshooting. **Start here if you're new to Jumperless.**

### [UI Style Guide](../examples/Jumperless_UI_Style_Guide.md)
**The official style guide.** Details the specific icons, colors, and formatting conventions that must be used for all UI elements to ensure a consistent user experience.

---

## 2. User Guides

### [Hardware Interface Guide](Hardware_Interface_Guide.md)
An overview of the Jumperless hardware components, including power rails, DACs, ADCs, and GPIO pins that are controllable via the MicroPython API.

### [Terminal Commands Reference](Terminal_Commands_Reference.md)
A complete reference for all built-in terminal commands available from the main menu, covering everything from connection management to debugging.

### [Applications and Utilities Guide](Applications_and_Utilities_Guide.md)
A guide to the built-in applications and utilities, such as the File Manager, eKilo Text Editor, and I2C Scanner.

### [File System Guide](File_System_Guide.md)
Explains how to work with the Jumperless file system, including file and directory operations and accessing the device as a USB Mass Storage device.

---

## 3. MicroPython Programming

### [MicroPython API Reference](Jumperless_API_Reference.md)
**The primary programming reference.** A comprehensive guide to the `jumperless` MicroPython module, with detailed explanations and examples for all functions and constants.

### [JFS Module Documentation](JFS_Module_Documentation.md)
Detailed documentation for the `jfs` module, which provides a comprehensive, Pythonic interface for filesystem operations.

### [Example Scripts Reference](Example_Scripts_Reference.md)
A collection of ready-to-use Python scripts demonstrating various Jumperless features and common use cases. A great starting point for your own projects.

---

## 4. Logic Analyzer (Current Implementation)

### [Complete Protocol Alignment Fix](Complete_Protocol_Alignment_Fix.md)
**Current logic analyzer implementation.** The definitive guide to the hybrid Jumperless/SUMP protocol implementation, libsigrok driver compatibility, and PulseView integration. **Use this for all logic analyzer work.**

### [Large Sample Count Fix](Large_Sample_Count_Fix.md)
Technical details on handling large sample counts (50K+ samples) with proper data transmission and timeout management.

### [Forced Analog Channels Fix](Forced_Analog_Channels_Fix.md)
**Current debugging approach.** How the firmware now always enables all analog channels regardless of driver configuration, ensuring analog data is always captured for mixed-signal testing.

### [Driver-Firmware Synchronization Fix](Driver_Firmware_Sync_Fix.md)
**Complete solution.** The definitive fix that synchronizes both the libsigrok driver and Jumperless firmware to correctly handle mixed-signal data. **Use this for current mixed-signal captures.**

---

## 5. Development & Advanced Topics

### [MicroPython Native Module](MicroPython_Native_Module.md)
A technical deep-dive into the C implementation of the Jumperless MicroPython module, explaining its architecture and performance benefits.

### [Building Native Module](Building_Native_Module.md)
Instructions for building and integrating custom MicroPython modules with the Jumperless platform.

### [OLED Async Implementation](OLED_Async_Implementation.md)
A technical guide to the asynchronous I2C implementation for non-blocking control of the OLED display.

---

## 6. Testing & Validation

### Logic Analyzer Tests
- [`test_sump_protocol.py`](../examples/test_sump_protocol.py) - SUMP protocol compatibility test
- [`test_large_capture.py`](../examples/test_large_capture.py) - Large sample count validation
- [`test_command_mapping.py`](../examples/test_command_mapping.py) - Command recognition test
- [`test_forced_analog_channels.py`](../examples/test_forced_analog_channels.py) - Verify forced analog channel configuration
- [`test_driver_firmware_sync.py`](../examples/test_driver_firmware_sync.py) - Complete driver-firmware synchronization validation

### MicroPython Examples
- [`micropython_examples/`](../examples/micropython_examples/) - Working MicroPython code examples
- [`jfs_demo.py`](../examples/micropython_examples/jfs_demo.py) - File system operations demo

---

## 📝 Documentation Maintenance

This documentation index reflects the current, cleaned-up state of the Jumperless documentation. **Obsolete documentation has been removed** to maintain clarity and ensure all listed files are current and functional.

**Last Updated:** December 2024 - Complete protocol alignment implementation 