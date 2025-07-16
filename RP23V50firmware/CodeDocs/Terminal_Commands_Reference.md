# Terminal Commands Reference

This document provides a reference for the Jumperless firmware's built-in terminal commands. These commands are available from the main menu when connected via a serial terminal.

## Command Syntax

Most commands are single characters. Some commands may take additional arguments or prompt for more information.

### Getting Help
*   `help`: Displays a list of command categories.
*   `help <category>`: Shows detailed help for a specific category (e.g., `help basics`).
*   `<command>?`: Shows help for a specific command (e.g., `n?`).

## Main Menu Commands

### Navigation & File Management
*   `m`: Show the main menu.
*   `/`: Open the File Manager.
*   `s`: Show all saved slot files.
*   `o`: Load a node file from a specific slot.
*   `f`: Load a node file from serial input.
*   `<`: Cycle to the previous slot.
*   `>`: Cycle to the next slot (Not implemented in `main.cpp` loop).

### Connection Management
*   `n`: Show the current net list.
*   `b`: Show the bridge array, paths, and chip status.
*   `c`: Show the raw crossbar switch status.
*   `x`: Clear all connections.
*   `+`: Add connections interactively.
*   `-`: Remove connections interactively.
*   `y`: Refresh connections from the currently loaded node file.

### Hardware Control
*   `^`: Set the voltage for the current DAC (alternates between DAC0 and DAC1).
*   `v`: Get readings from ADC channels.
*   `g`: Print the current state of all GPIO pins.
*.  `a`: Connect UART to Arduino pins D0/D1.
*   `A`: Disconnect UART from Arduino pins.
*   `r`: Reset the Arduino Nano. Can be followed by `t` (top) or `b` (bottom) reset button.

### System & Debug
*   `?`: Show the firmware version.
*   `'`: Replay the startup animation.
*   `d`: Open the menu to set debug flags.
*   `l`: Open the LED brightness and test menu.
*   `` ` ``: Edit the `config.txt` file in the eKilo editor.
*   `~`: Print the contents of `config.txt`.
*   `p`: Enter the MicroPython REPL.
*   `U`/`u`: Enable/disable USB Mass Storage mode.
*   `e`: Show/hide extra menu options.
*   `C`: Toggle terminal colors on/off.

### Extra Menu Commands (`e`)
*   `G`: Reload the `config.txt` file.
*   `P`: Print all connectable node names.
*   `F`: Cycle through available OLED fonts.
*   `@`: Scan for I2C devices. Can be used as `@` for interactive, `@<sda>,<scl>` for specific rows, or `@<row>` for auto-scan around a row.
*   `$`: Run DAC calibration.
*   `=`: Dump the OLED frame buffer to the terminal.
*   `k`: Toggle showing OLED content in the terminal.
*   `R`: Toggle showing the board's LED status in a continuous dump.
*   `%`: List all files and directories on the filesystem.

## Special Keys & Shortcuts

*   **Ctrl+Q**: Quit the current mode (e.g., file manager, editor, REPL).
*   **Clickwheel**: Used for navigation in menus, the file manager, and the editor.
    *   **Scroll**: Move selection up/down.
    *   **Short Press**: Select/Confirm.
    *   **Long Press**: Back/Cancel.
*   **Probe Buttons**: The two buttons on the probe are used for making (`CONNECT`) and breaking (`REMOVE`) connections in probe mode. 