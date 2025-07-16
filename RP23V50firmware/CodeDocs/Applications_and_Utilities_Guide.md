# Applications and Utilities Guide

The Jumperless firmware includes several built-in applications and utilities to aid in development and debugging. These can be accessed via terminal commands from the main menu.

## File Manager

The File Manager provides a visual interface for managing files and directories on the Jumperless's internal flash storage.

*   **Launch Command**: `/`

### Features
*   Navigate directories using the **arrow keys** or **clickwheel**.
*   View file contents.
*   Create, edit, and delete files and directories.
*   Color-coded file type indicators for easy identification.
*   Integrated with the **eKilo** text editor.

### File Manager Commands
*   `Enter` / **Clickwheel Press**: Open a file or directory.
*   `v`: View the contents of the selected file.
*   `e`: Edit the selected file using the eKilo editor.
*   `n`: Create a new file in the current directory.
*   `d`: Create a new directory.
*   `x`: Delete the selected file or directory (will ask for confirmation).
*   `r`: Rename the selected file or directory.
*   `i`: Show information about the selected file (size, etc.).
*   `h`: Display help for the file manager.
*   `q` / **Ctrl+Q**: Quit the file manager and return to the main menu.
*   `/`: Go to the root directory.
*   `.`: Go up to the parent directory.
*   `u`: Show memory usage statistics.
*   `m`: Manually initialize the MicroPython example scripts.

### File Type Indicators
The file manager uses unicode symbols and colors to indicate file types:
*   `⌘` **Directory** (Blue)
*   `𓆚` **Python File** (Green)
*   `⍺` **Text File** (Gray)
*   `⚙` **Config File** (Yellow)
*   `☊` **Node File** (Cyan)
*   `⎃` **Color File** (Rainbow)

## eKilo Text Editor

eKilo is a lightweight, built-in text editor for creating and modifying files directly on the Jumperless.

*   **Launch**: Automatically launched when creating a new file (`n`) or editing an existing one (`e`) in the File Manager.

### Features
*   Syntax highlighting for Python and other file types.
*   Basic editing functionality (typing, backspace, etc.).
*   Cursor movement via the **clickwheel**.
*   Save and exit functionality.

### Editor Commands
*   **Ctrl+S**: Save the file.
*   **Ctrl+P**: Save the file and load its content into the MicroPython REPL.
*   **Ctrl+Q**: Quit the editor without saving.
*   **Clickwheel Scroll**: Move the cursor up and down.
*   **Clickwheel Press**: Toggles character selection mode.

## I2C Scanner

A utility to scan the I2C bus for connected devices.

*   **Launch Command**: `@`

### Usage
*   `@`: Interactive mode, prompts for SDA and SCL rows.
*   `@<sda_row>,<scl_row>`: Scans using specific breadboard rows for SDA and SCL (e.g., `@20,21`).
*   `@<row>`: Automatically tries all four possible SDA/SCL combinations around the specified row.

## DAC Calibration

A utility to calibrate the Digital-to-Analog Converters for improved accuracy.

*   **Launch Command**: `$`

This process is typically only needed once or if the hardware configuration changes.

## MicroPython REPL

A full-featured MicroPython Read-Eval-Print Loop for interactive coding.

*   **Launch Command**: `p`

See the [MicroPython API Reference](MicroPython_API_Reference.md) for more details on using the REPL.

## USB Mass Storage

This utility allows the Jumperless to appear as a USB flash drive when connected to a computer, providing direct access to the internal filesystem.

*   **Enable Command**: `U`
*   **Disable Command**: `u`

When enabled, you can drag and drop files to and from the Jumperless, making it easy to manage scripts and data. 