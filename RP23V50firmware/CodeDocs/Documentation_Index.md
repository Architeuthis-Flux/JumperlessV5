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

## 4. Advanced Topics

### [MicroPython Native Module](MicroPython_Native_Module.md)
A technical deep-dive into the C implementation of the Jumperless MicroPython module, explaining its architecture and performance benefits.

### [OLED Async Implementation](OLED_Async_Implementation.md)
A technical guide to the asynchronous I2C implementation for non-blocking control of the OLED display. 