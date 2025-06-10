#!/bin/bash

# Build script for MicroPython embedding
set -e

# Configuration
MICROPYTHON_REPO_PATH="${HOME}/src/micropython/micropython"
MICROPYTHON_LOCAL_PATH="$(dirname "$0")/../src/micropython"
PROJECT_ROOT="$(dirname "$0")/.."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building MicroPython embed port for Jumperless...${NC}"

# Check if MicroPython repo exists
if [ ! -d "$MICROPYTHON_REPO_PATH" ]; then
    echo -e "${RED}Error: MicroPython repository not found at $MICROPYTHON_REPO_PATH${NC}"
    echo -e "${YELLOW}Please clone MicroPython repository:${NC}"
    echo "  mkdir -p $HOME/src/micropython"
    echo "  cd $HOME/src/micropython"
    echo "  git clone https://github.com/micropython/micropython.git"
    echo "  cd micropython/mpy-cross"
    echo "  make"
    exit 1
fi

# Check if mpy-cross is built
if [ ! -f "$MICROPYTHON_REPO_PATH/mpy-cross/build/mpy-cross" ]; then
    echo -e "${YELLOW}Building mpy-cross compiler...${NC}"
    cd "$MICROPYTHON_REPO_PATH/mpy-cross"
    make
fi

# Clean previous build
cd "$MICROPYTHON_LOCAL_PATH"
if [ -d "micropython_embed" ]; then
    echo -e "${YELLOW}Cleaning previous MicroPython build...${NC}"
    rm -rf micropython_embed
fi

# Remove the manually edited qstrdefs file that was causing issues
if [ -f "micropython_embed/genhdr/qstrdefs.generated.h" ]; then
    rm -f "micropython_embed/genhdr/qstrdefs.generated.h"
fi

# Build the embed port with proper QSTR generation
echo -e "${YELLOW}Building MicroPython embed port...${NC}"
make -f micropython_embed.mk MICROPYTHON_TOP="$MICROPYTHON_REPO_PATH" V=1

# Verify the build
if [ -f "micropython_embed/genhdr/qstrdefs.generated.h" ]; then
    QSTR_COUNT=$(grep -c "^QDEF" micropython_embed/genhdr/qstrdefs.generated.h || true)
    echo -e "${GREEN}✅ MicroPython embed build successful!${NC}"
    echo -e "${GREEN}   Generated $QSTR_COUNT QSTR definitions${NC}"
    echo -e "${GREEN}   Files ready for PlatformIO integration${NC}"
else
    echo -e "${RED}❌ MicroPython embed build failed!${NC}"
    echo -e "${RED}   qstrdefs.generated.h not found${NC}"
    exit 1
fi

echo -e "${GREEN}✅ MicroPython is ready for use with floating point support enabled!${NC}" 