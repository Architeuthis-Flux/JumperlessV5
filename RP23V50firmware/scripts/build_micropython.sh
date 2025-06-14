#!/bin/bash

# Build script for MicroPython embedding
set -e

# Configuration
MICROPYTHON_VERSION="v1.25.0"
MICROPYTHON_REPO_PATH=$(realpath "$(dirname "$0")/../micropython_repo")
MICROPYTHON_LOCAL_PATH=$(realpath "$(dirname "$0")/../lib/micropython")
PROJECT_ROOT=$(realpath "$(dirname "$0")/../")

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building MicroPython embed port for Jumperless...${NC}"



# Check if we already have a working micropython_embed with Jumperless integration
cd "$MICROPYTHON_LOCAL_PATH"
if [ -f "micropython_embed/genhdr/qstrdefs.generated.h" ] && grep -q "jumperless" micropython_embed/genhdr/qstrdefs.generated.h; then
    echo -e "${GREEN}✅ MicroPython embed port with Jumperless integration already exists!${NC}"
    echo -e "${GREEN}   Skipping build - using existing setup${NC}"
    
    # Verify the existing build
    QSTR_COUNT=$(grep -c "^QDEF" micropython_embed/genhdr/qstrdefs.generated.h || true)
    JUMPERLESS_QSTRS=$(grep -c "jumperless\|dac_set\|adc_get\|nodes_connect" micropython_embed/genhdr/qstrdefs.generated.h || true)
    echo -e "${GREEN}   Found $QSTR_COUNT QSTR definitions${NC}"
    echo -e "${GREEN}   Jumperless module QSTRs found: $JUMPERLESS_QSTRS${NC}"
    echo -e "${GREEN}✅ MicroPython is ready for use with Jumperless native module enabled!${NC}"
    exit 0
fi

echo -e "${YELLOW}Building MicroPython embed port with Jumperless module...${NC}"

# Check if MicroPython repo exists
if [ ! -d "$MICROPYTHON_REPO_PATH" ]; then
    git clone "https://github.com/micropython/micropython.git" "${MICROPYTHON_REPO_PATH}"
fi

pushd "${MICROPYTHON_REPO_PATH}"
# Fetch all tags to ensure we have the release tags
git fetch --tags
# git checkout main
git checkout "${MICROPYTHON_VERSION}"
# Initialize submodules needed for the build
git submodule update --init --recursive lib/uzlib lib/libm lib/libm_dbl
popd

# Clean previous build
echo -e "${YELLOW}Cleaning previous MicroPython build...${NC}"
cd "$MICROPYTHON_REPO_PATH/ports/embed"
if [ -d "build-embed" ]; then
    rm -rf build-embed
fi

cd "$MICROPYTHON_LOCAL_PATH"
if [ -d "micropython_embed" ]; then
    echo -e "${YELLOW}Cleaning previous micropython_embed...${NC}"
    rm -rf micropython_embed
fi

# Build the embed port with proper QSTR generation and Jumperless module
echo -e "${YELLOW}Building MicroPython embed port with Jumperless module...${NC}"
cd "$MICROPYTHON_REPO_PATH/ports/embed"
make -f embed.mk MICROPYTHON_TOP="$MICROPYTHON_REPO_PATH" USER_C_MODULES="$PROJECT_ROOT/modules" V=1

# Copy the built files to our micropython_embed directory
echo -e "${YELLOW}Copying built files to micropython_embed directory...${NC}"
cd "$MICROPYTHON_LOCAL_PATH"
if [ -d "micropython_embed" ]; then
    rm -rf micropython_embed
fi
cp -r "$MICROPYTHON_REPO_PATH/ports/embed/build-embed" micropython_embed

# Verify the build
if [ -f "micropython_embed/genhdr/qstrdefs.generated.h" ]; then
    QSTR_COUNT=$(grep -c "^QDEF" micropython_embed/genhdr/qstrdefs.generated.h || true)
    JUMPERLESS_QSTRS=$(grep -c "jumperless\|dac_set\|adc_get\|nodes_connect" micropython_embed/genhdr/qstrdefs.generated.h || true)
    echo -e "${GREEN}✅ MicroPython embed build successful!${NC}"
    echo -e "${GREEN}   Generated $QSTR_COUNT QSTR definitions${NC}"
    if [ "$JUMPERLESS_QSTRS" -gt 0 ]; then
        echo -e "${GREEN}   Jumperless module QSTRs found: $JUMPERLESS_QSTRS${NC}"
    else
        echo -e "${YELLOW}   Warning: No Jumperless module QSTRs detected${NC}"
    fi
    echo -e "${GREEN}   Files ready for PlatformIO integration${NC}"
else
    echo -e "${RED}❌ MicroPython embed build failed!${NC}"
    echo -e "${RED}   qstrdefs.generated.h not found${NC}"
    exit 1
fi

# Verify Jumperless module files are present
echo -e "${YELLOW}Verifying Jumperless module integration...${NC}"
if [ -f "$PROJECT_ROOT/modules/jumperless/modjumperless.c" ]; then
    echo -e "${GREEN}   ✅ Jumperless MicroPython module found${NC}"
else
    echo -e "${RED}   ❌ Jumperless MicroPython module missing${NC}"
fi

if [ -f "$PROJECT_ROOT/src/JumperlessMicroPythonAPI.cpp" ]; then
    echo -e "${GREEN}   ✅ Jumperless API wrapper found${NC}"
else
    echo -e "${RED}   ❌ Jumperless API wrapper missing${NC}"
fi

echo -e "${GREEN}✅ MicroPython is ready for use with Jumperless native module enabled!${NC}" 