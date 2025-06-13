#!/bin/bash

# Build script for MicroPython embedding
set -e

# Configuration
MICROPYTHON_VERSION="v1.25.0"
MICROPYTHON_REPO_PATH=$(realpath "$(dirname "$0")/../.micropython/micropython")
MICROPYTHON_LOCAL_PATH="$(dirname "$0")/../lib/micropython"
PROJECT_ROOT="$(dirname "$0")/.."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building MicroPython embed port for Jumperless...${NC}"

# Check if MicroPython repo exists
if [ ! -d "$MICROPYTHON_REPO_PATH" ]; then
    git clone "https://github.com/micropython/micropython.git" "${MICROPYTHON_REPO_PATH}"
fi

pushd "${MICROPYTHON_REPO_PATH}"
git checkout "${MICROPYTHON_VERSION}"
popd

# Clean previous build
cd "$MICROPYTHON_LOCAL_PATH"
if [ -d "build-embed" ]; then
    echo -e "${YELLOW}Cleaning previous MicroPython build...${NC}"
    rm -rf build-embed
fi

if [ -d "micropython_embed" ]; then
    echo -e "${YELLOW}Cleaning previous micropython_embed...${NC}"
    rm -rf micropython_embed
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

# Hack around micropython embed port including stdout def already
rm micropython_embed/port/mphalport.[ch]

echo -e "${GREEN}✅ MicroPython is ready for use with floating point support enabled!${NC}" 