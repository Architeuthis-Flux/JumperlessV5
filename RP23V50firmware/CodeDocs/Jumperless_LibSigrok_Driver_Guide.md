# Jumperless LibSigrok Driver Development Guide

This guide covers building and developing the Jumperless LibSigrok driver for mixed-signal analysis capabilities.

## Overview

The Jumperless LibSigrok driver enables the Jumperless V5 to function as a mixed-signal logic analyzer and oscilloscope in PulseView and other sigrok-based applications. The driver supports:

- **Digital Logic Analysis**: 8 channels via SUMP protocol 
- **Analog Oscilloscope**: 4 channels via custom protocol
- **Mixed-Signal Capture**: Simultaneous digital and analog acquisition
- **Hardware Triggers**: Basic trigger functionality
- **High-Speed Sampling**: Up to 50 MHz digital, configurable analog rates

## Architecture

The driver implements a hybrid approach:

- **Digital channels** use the standard SUMP/OLS protocol for compatibility
- **Analog channels** use a custom protocol for flexibility
- **Channel management** allows mixed-signal operation
- **Data processing** handles both logic and analog data streams

### Key Files

```
examples/libsigrok-master/src/hardware/jumperless/
├── api.c           # Driver API implementation
├── protocol.h      # Protocol definitions and structures  
├── protocol.c      # Protocol implementation and data processing
```

## Quick Start

### 1. Build Script Usage

The `build_jumperless_driver.sh` script automates the entire development workflow:

```bash
# Check current status
./build_jumperless_driver.sh status

# Verify patches are applied
./build_jumperless_driver.sh check-patches

# Build the driver (clean build)
./build_jumperless_driver.sh build

# Quick incremental rebuild after changes
./build_jumperless_driver.sh quick-build

# Test the driver
./build_jumperless_driver.sh test

# Install system-wide
./build_jumperless_driver.sh install

# Setup PulseView.app (macOS only)
./build_jumperless_driver.sh pulseview-app
```

### 2. macOS PulseView.app Installation

**Complete Installation Process:**

```bash
# 1. Install dependencies (if not already done)
brew install autoconf automake libtool pkg-config
brew install glib libserialport libusb libftdi hidapi

# 2. Build and install custom libsigrok
./build_jumperless_driver.sh build
./build_jumperless_driver.sh install

# 3. Update PulseView.app with Jumperless driver
./build_jumperless_driver.sh pulseview-app

# 4. Test with sigrok-cli (optional)
export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH
sigrok-cli -L | grep jumperless
```

**Using PulseView.app:**

1. Launch PulseView.app from Applications
2. Go to **File → Connect to Device**
3. Select **"Jumperless Mixed-Signal Logic Analyzer"** from the driver list
4. Configure connection settings (serial port, baud rate)
5. Set up your digital/analog channels
6. Start acquisition

### 3. Development Workflow

```bash
# Open driver files for editing
./build_jumperless_driver.sh dev-edit

# Make your changes...

# Quick rebuild and test
./build_jumperless_driver.sh quick-build
./build_jumperless_driver.sh pulseview-app  # Update PulseView.app
```

## Platform Setup

### Ubuntu/Debian

```bash
# Install build dependencies
sudo apt-get install build-essential autoconf automake libtool pkg-config
sudo apt-get install libglib2.0-dev libserialport-dev libusb-1.0-0-dev
sudo apt-get install libftdi1-dev libhidapi-dev

# Install sigrok suite for testing
sudo apt-get install sigrok-cli pulseview
```

### macOS

```bash
# Install Homebrew dependencies
brew install autoconf automake libtool pkg-config
brew install glib libserialport libusb libftdi hidapi

# Install sigrok (if available)
brew install sigrok-cli
# Note: PulseView may need to be built from source on macOS
```

### Fedora/RHEL

```bash
# Install build dependencies
sudo dnf install gcc autoconf automake libtool pkgconfig
sudo dnf install glib2-devel libserialport-devel libusb1-devel
sudo dnf install libftdi-devel hidapi-devel

# Install sigrok suite
sudo dnf install sigrok-cli pulseview
```

## Build Script Reference

### Commands

| Command | Description |
|---------|-------------|
| `build` | Clean build of libsigrok with Jumperless driver |
| `quick-build` | Incremental build (faster for development) |
| `configure` | Just run the configure step |
| `clean` | Clean build directory |
| `install` | Install to system (requires sudo) |
| `test` | Run basic driver tests |
| `dev-edit` | Open driver source files in editor |
| `check-patches` | Verify patches are applied correctly |
| `status` | Show build status and configuration |
| `pulseview-app` | Setup PulseView.app to use custom libsigrok (macOS) |
| `help` | Show usage information |

### Options

| Option | Description |
|--------|-------------|
| `--prefix PATH` | Set install prefix (default: `/usr/local`) |
| `--debug` | Enable debug build with symbols |
| `--verbose` | Show verbose build output |

### Examples

```bash
# Debug build with verbose output
./build_jumperless_driver.sh build --debug --verbose

# Install to custom location
./build_jumperless_driver.sh install --prefix=/opt/sigrok

# Development cycle
./build_jumperless_driver.sh dev-edit    # Edit files
./build_jumperless_driver.sh quick-build # Rebuild
./build_jumperless_driver.sh test        # Test
```

## Driver Implementation Details

### Device Capabilities

```c
#define JUMPERLESS_NUM_LOGIC_CHANNELS     8
#define JUMPERLESS_NUM_ANALOG_CHANNELS    4  
#define JUMPERLESS_MAX_SAMPLES            262144     // 256K samples
#define JUMPERLESS_MAX_SAMPLERATE         50000000UL // 50 MHz
#define JUMPERLESS_DEFAULT_SAMPLERATE     1000000UL  // 1 MHz
```

### SUMP Protocol (Digital)

The driver implements standard SUMP commands for digital logic analysis:

- `CMD_RESET` - Reset device
- `CMD_ARM_BASIC_TRIGGER` - Start acquisition  
- `CMD_SET_DIVIDER` - Set sample rate
- `CMD_CAPTURE_SIZE` - Set buffer size
- `CMD_SET_BASIC_TRIGGER_*` - Configure triggers

### Custom Protocol (Analog)

Custom commands for analog channel control:

- `CMD_JUMPERLESS_ANALOG_ENABLE` - Enable analog mode
- `CMD_JUMPERLESS_ANALOG_CONFIG` - Configure channels
- `CMD_JUMPERLESS_ANALOG_DATA` - Request analog data

### Mixed-Signal Operation

The driver manages both protocols simultaneously:

1. **Channel Detection**: Automatically detects active digital/analog channels
2. **Protocol Switching**: Seamlessly switches between SUMP and custom protocols
3. **Data Synchronization**: Aligns digital and analog sample timestamps
4. **Trigger Coordination**: Ensures triggers work across both domains

## Testing and Validation

### Basic Tests

```bash
# Check if driver loads
./build_jumperless_driver.sh test

# List available drivers (should include jumperless)
sigrok-cli --list-drivers | grep jumperless

# Test device detection
sigrok-cli --driver jumperless --list-devices

# Capture test data
sigrok-cli --driver jumperless --samples 1000 --output test.sr
```

### PulseView Integration

1. Launch PulseView
2. Go to **File → Connect to Device**
3. Select **Jumperless** from driver list
4. Configure serial port connection
5. Set up digital/analog channels as needed
6. Start acquisition

### Development Testing

```bash
# Enable debug mode for verbose logging
export SR_LOG_LEVEL=debug

# Test with specific connection
sigrok-cli --driver jumperless:conn=/dev/ttyUSB0 --continuous

# Test analog channels specifically  
sigrok-cli --driver jumperless --channels A0,A1 --time 1s
```

## Common Issues and Troubleshooting

### Build Issues

**Missing Dependencies**
```bash
# The script will detect and list missing dependencies
./build_jumperless_driver.sh build
# Follow the installation instructions provided
```

**Configure Fails**
```bash
# Check autogen log
cat examples/libsigrok-master/build/configure.log

# Force regeneration
./build_jumperless_driver.sh clean
./build_jumperless_driver.sh build
```

### Runtime Issues

**Driver Not Found**
- Ensure patches are applied: `./build_jumperless_driver.sh check-patches`
- Verify installation: `sigrok-cli --list-drivers | grep jumperless`
- Check library path: `export LD_LIBRARY_PATH=/usr/local/lib`

**Device Not Detected**
- Check serial port permissions: `ls -l /dev/ttyUSB*`
- Test basic serial communication
- Verify Jumperless firmware supports the protocol

**Mixed-Signal Issues**
- Ensure both digital and analog channels are properly configured
- Check sample rate compatibility between modes
- Verify trigger settings work for your setup

### macOS Specific Issues

**PulseView.app Not Recognizing Driver**
- Run `./build_jumperless_driver.sh pulseview-app` after any rebuild
- Verify with: `strings /Applications/PulseView.app/Contents/Frameworks/libsigrok.4.dylib | grep jumperless`
- Check which libraries PulseView uses: `otool -L /Applications/PulseView.app/Contents/MacOS/pulseview.real | grep sigrok`
- Restart PulseView.app completely after updating

**sigrok-cli Not Finding Driver**
```bash
# Set library path for testing
export DYLD_LIBRARY_PATH=/usr/local/lib:$DYLD_LIBRARY_PATH
sigrok-cli -L | grep jumperless
```

**Permission Issues**
```bash
# If installation fails, ensure proper permissions
sudo chown -R $(whoami) /usr/local/lib
sudo chown $(whoami) /Applications/PulseView.app/Contents/Frameworks/libsigrok.dylib
```

**Backup and Restore**
```bash
# Restore original PulseView.app libraries if needed
cp /Applications/PulseView.app/Contents/Frameworks/libsigrok.4.dylib.backup \
   /Applications/PulseView.app/Contents/Frameworks/libsigrok.4.dylib
cp /Applications/PulseView.app/Contents/Frameworks/libsigrokcxx.4.dylib.backup \
   /Applications/PulseView.app/Contents/Frameworks/libsigrokcxx.4.dylib
```

## Development Tips

### Code Organization

- Keep SUMP protocol implementation in separate functions
- Use clear naming for analog vs digital operations
- Maintain compatibility with existing sigrok patterns

### Adding Features

1. **New Commands**: Add to protocol.h definitions
2. **Channel Types**: Extend channel configuration
3. **Triggers**: Enhance trigger capabilities
4. **Sample Rates**: Add new rate options

### Testing Changes

```bash
# Quick development cycle
./build_jumperless_driver.sh dev-edit
# Make changes...
./build_jumperless_driver.sh quick-build
./build_jumperless_driver.sh test
```

### Debugging

```bash
# Enable all debug output
export SR_LOG_LEVEL=debug
export G_MESSAGES_DEBUG=all

# Run with debug info
sigrok-cli --driver jumperless --samples 100 --debug 2>&1 | tee debug.log
```

## Integration with Jumperless Firmware

The driver expects the Jumperless V5 firmware to support:

1. **SUMP Protocol**: Standard OLS-compatible commands for digital channels
2. **Custom Protocol**: Jumperless-specific commands for analog channels  
3. **USB Serial**: Communication via USB CDC-ACM interface
4. **Mixed Mode**: Ability to handle both protocols simultaneously

### Firmware Requirements

- USB VID/PID: 0403:6001 (configurable)
- Serial Settings: 115200 baud, 8N1
- Buffer Size: Minimum 256KB for sample storage
- Sample Rates: 1 Hz to 50 MHz range
- Trigger Support: Basic edge and level triggers

## Future Enhancements

### Planned Features

- [ ] Advanced trigger types (pattern, sequence)
- [ ] Higher analog sample rates
- [ ] Protocol decoder support
- [ ] Calibration and offset correction
- [ ] Streaming mode for continuous capture

### Contributing

1. Fork the repository
2. Make changes to driver files
3. Test thoroughly with the build script
4. Submit pull request with clear description

## References

- [LibSigrok Documentation](https://sigrok.org/wiki/Libsigrok)
- [SUMP Protocol Specification](https://www.sump.org/projects/analyzer/protocol/)
- [PulseView User Guide](https://sigrok.org/wiki/PulseView)
- [Jumperless V5 Hardware Documentation](../../README.md)

## License

This driver is licensed under the GNU General Public License v3+, consistent with the libsigrok project. 