#!/bin/bash

# Direct Jumperless AppImage Runner
# Runs the AppImage directly without any fancy detection

echo "Direct Jumperless Runner"
echo "======================"

# Check if we're on ARM64 or x86_64
ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then
    APPIMAGE_NAME="Jumperless-aarch64.AppImage"
else
    APPIMAGE_NAME="Jumperless-x86_64.AppImage"
fi

# Look for AppImage in builds directory
APPIMAGE="builds/linux/appimage/$APPIMAGE_NAME"
if [ ! -f "$APPIMAGE" ]; then
    echo "AppImage not found: $APPIMAGE"
    echo "Run: python3 JumperlessAppPackager.py"
    exit 1
fi

# Run it
echo "Running $APPIMAGE"
chmod +x "$APPIMAGE"
exec "$APPIMAGE" "$@" 