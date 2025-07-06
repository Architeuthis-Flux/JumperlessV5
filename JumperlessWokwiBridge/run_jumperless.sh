#!/bin/bash

# Simple Jumperless AppImage Launcher
# Detects architecture and runs the correct AppImage

echo "Starting Jumperless..."

# Detect architecture
ARCH=$(uname -m)
echo "Architecture: $ARCH"

# Set AppImage name based on architecture
case "$ARCH" in
    x86_64)
        APPIMAGE="Jumperless-x86_64.AppImage"
        ;;
    aarch64|arm64)
        APPIMAGE="Jumperless-aarch64.AppImage"
        ;;
    *)
        echo "Unsupported architecture: $ARCH"
        echo "Supported: x86_64, aarch64/arm64"
        echo "Trying x86_64 AppImage..."
        APPIMAGE="Jumperless-x86_64.AppImage"
        ;;
esac

# Look for AppImage in builds directory
if [ -f "builds/linux/appimage/$APPIMAGE" ]; then
    APPIMAGE_PATH="builds/linux/appimage/$APPIMAGE"
else
    echo "AppImage not found: builds/linux/appimage/$APPIMAGE"
    echo "To create AppImages, run: python3 JumperlessAppPackager.py"
    exit 1
fi

# Make executable if not already
if [ ! -x "$APPIMAGE_PATH" ]; then
    echo "Making AppImage executable..."
    chmod +x "$APPIMAGE_PATH"
fi

# Run the AppImage
echo "Running: $APPIMAGE_PATH"
exec "$APPIMAGE_PATH" "$@" 