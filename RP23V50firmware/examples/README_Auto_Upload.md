# Jumperless Auto-Upload Script (PlatformIO)

This script automatically detects when a Jumperless device is plugged in and uploads firmware using PlatformIO.

## Features

- **Automatic Detection**: Monitors for new serial devices and identifies Jumperless boards
- **PlatformIO Integration**: Uses the official PlatformIO build and upload system
- **Cross-Platform**: Works on Windows, macOS, and Linux
- **Watch Mode**: Continuously monitors for new devices
- **Robust Uploads**: Uses PlatformIO's proven upload mechanisms
- **Environment Support**: Supports different PlatformIO environments

## Requirements

Install PlatformIO and required Python packages:

```bash
# Install PlatformIO
pip install platformio

# Install required Python package for device detection
pip install pyserial
```

## Usage

### Basic Usage (One-time scan and upload)

```bash
# Scan for connected Jumperless devices and upload firmware
python3 auto_upload.py

# Upload to a specific environment
python3 auto_upload.py --environment jumperless_v5

# Upload from a different project directory
python3 auto_upload.py --project-dir ../RP23V50firmware
```

### Watch Mode (Continuous monitoring)

```bash
# Monitor for new devices and auto-upload when detected
python3 auto_upload.py --watch

# Watch mode with specific environment
python3 auto_upload.py --watch --environment jumperless_v5
```

### Command Line Options

```bash
python3 auto_upload.py [OPTIONS]

Options:
  --watch, -w               Watch mode - continuously monitor for new devices
  --environment, -e ENV     PlatformIO environment (default: jumperless_v5)
  --project-dir, -d DIR     PlatformIO project directory (default: current directory)
  --help, -h               Show help message
```

## Examples

### Example 1: Quick Firmware Upload
```bash
python3 auto_upload.py
```
This scans for any connected Jumperless devices and uploads the current firmware using PlatformIO.

### Example 2: Development Workflow
```bash
python3 auto_upload.py --watch
```
Perfect for development! Leave this running and every time you plug in a Jumperless device, it will automatically get the latest firmware uploaded.

### Example 3: Different Project Directory
```bash
python3 auto_upload.py --project-dir ../RP23V50firmware --environment jumperless_v5
```
Upload firmware from a different directory with a specific environment.

### Example 4: Multiple Devices
```bash
python3 auto_upload.py --watch
```
If multiple Jumperless devices are connected, PlatformIO will handle port selection automatically.

## How It Works

1. **Port Scanning**: The script scans all available serial ports
2. **Device Filtering**: Identifies potential Jumperless devices by USB/serial patterns  
3. **PlatformIO Upload**: Uses `platformio run --target upload` to build and upload firmware
4. **Automatic Port Selection**: PlatformIO handles device communication and upload protocols

## Supported Device Patterns

The script looks for devices matching these patterns:
- USB serial devices (ttyUSB*, COM*)
- ACM devices (ttyACM*)
- Arduino-like devices
- RP2040/Pico devices
- Devices with "Serial" in the description

## PlatformIO Commands Used

The script essentially runs this command for you:

```bash
platformio run --target upload --environment jumperless_v5 --upload-port /dev/ttyUSB0
```

Or without a specific port (auto-detection):

```bash
platformio run --target upload --environment jumperless_v5
```

## Project Structure Requirements

Your project directory must contain:
- `platformio.ini` - PlatformIO configuration file
- `src/` directory with source code
- Any required libraries and dependencies

## Troubleshooting

### "PlatformIO not found"
Install PlatformIO:
```bash
pip install platformio
# OR
curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py -o get-platformio.py
python3 get-platformio.py
```

### "No platformio.ini found"
Make sure you're running the script from a PlatformIO project directory, or use `--project-dir` to specify the correct path.

### "No potential Jumperless devices found"
- Make sure the device is properly connected
- Check that drivers are installed
- Try different USB cables/ports
- Verify the device appears in device manager (Windows) or `ls /dev/tty*` (Linux/macOS)

### Build/Upload Errors
- Check your PlatformIO configuration in `platformio.ini`
- Ensure all dependencies are installed
- Verify the environment name is correct
- Check for compilation errors in your source code

### Permission Errors (Linux/macOS)
```bash
# Add your user to the dialout group (Linux)
sudo usermod -a -G dialout $USER

# Or use sudo to run the script
sudo python3 auto_upload.py
```

## Advanced Usage

### Custom Environments
You can define custom environments in your `platformio.ini`:

```ini
[env:jumperless_debug]
board = jumperless_v5
build_type = debug
build_flags = -DDEBUG_MODE

[env:jumperless_release]
board = jumperless_v5  
build_type = release
```

Then use them with:
```bash
python3 auto_upload.py --environment jumperless_debug
```

### Integration with IDEs
Set up your IDE to run the auto-upload script:

**VS Code**: Add to tasks.json:
```json
{
    "label": "Auto-upload to Jumperless",
    "type": "shell",
    "command": "python3",
    "args": ["examples/auto_upload.py"],
    "group": "build",
    "options": {
        "cwd": "${workspaceFolder}"
    }
}
```

**VS Code with Watch Mode**: 
```json
{
    "label": "Watch for Jumperless devices",
    "type": "shell",
    "command": "python3", 
    "args": ["examples/auto_upload.py", "--watch"],
    "group": "build",
    "isBackground": true
}
```

### Batch Processing
Upload to multiple environments:
```bash
#!/bin/bash
for env in jumperless_v5 jumperless_debug; do
    python3 auto_upload.py --environment "$env"
    sleep 2
done
```

## Advantages Over Manual Upload

1. **Automatic Device Detection**: No need to manually specify ports
2. **Robust Build System**: Uses PlatformIO's proven build and upload pipeline
3. **Better Error Handling**: PlatformIO provides detailed error messages
4. **Dependency Management**: Automatically handles library dependencies
5. **Cross-Platform**: Works consistently across operating systems
6. **Future-Proof**: Uses official PlatformIO APIs

## Safety Notes

- The script automatically uploads firmware when devices are detected
- Always test your firmware before using auto-upload in production
- Use watch mode carefully - it will upload to ANY newly detected Jumperless device
- Test with the one-time mode first before using watch mode
- Make sure your `platformio.ini` configuration is correct

## Comparison with Serial Upload

| Feature | PlatformIO Auto-Upload | Manual Serial Upload |
|---------|----------------------|---------------------|
| Reliability | High (official tools) | Medium (custom implementation) |
| Error Handling | Excellent | Basic |
| Build System | Full PlatformIO build | None (script only) |
| Dependencies | Automatic | Manual |
| Cross-Platform | Excellent | Good |
| Setup Complexity | Low | Medium |

## Contributing

Feel free to improve the script by:
- Adding better device detection patterns
- Improving error handling
- Adding support for different PlatformIO features
- Enhancing the progress display
- Adding more environment configurations

## License

This script is part of the Jumperless project and follows the same license terms. 