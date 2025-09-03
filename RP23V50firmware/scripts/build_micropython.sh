#!/bin/bash

# Build script for MicroPython embed port with built-in modules and Jumperless integration
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

echo -e "${GREEN}Building MicroPython embed port with built-in modules...${NC}"

# Check if we already have a working micropython_embed with Jumperless integration
cd "$MICROPYTHON_LOCAL_PATH"
if [ -f "micropython_embed/genhdr/qstrdefs.generated.h" ] && grep -q "jumperless" micropython_embed/genhdr/qstrdefs.generated.h; then
    echo -e "${GREEN}◆ MicroPython embed port with Jumperless integration already exists!${NC}"
    
    # Verify the existing build
    QSTR_COUNT=$(grep -c "^QDEF" micropython_embed/genhdr/qstrdefs.generated.h || true)
    JUMPERLESS_QSTRS=$(grep -c "jumperless\|dac_set\|adc_get\|nodes_connect" micropython_embed/genhdr/qstrdefs.generated.h || true)
    TIME_QSTRS=$(grep -c "time\|sleep\|ticks" micropython_embed/genhdr/qstrdefs.generated.h || true)
    echo -e "${GREEN}   Found $QSTR_COUNT total QSTR definitions${NC}"
    echo -e "${GREEN}   Jumperless module QSTRs found: $JUMPERLESS_QSTRS${NC}"
    echo -e "${GREEN}   Time module QSTRs found: $TIME_QSTRS${NC}"
    echo -e "${GREEN}◆ MicroPython embed port is ready with built-in modules!${NC}"
    # exit 0
fi

echo -e "${YELLOW}Building MicroPython embed port with Jumperless module and built-in modules...${NC}"

# Check if MicroPython repo exists
if [ ! -d "$MICROPYTHON_REPO_PATH" ]; then
    echo -e "${YELLOW}Cloning MicroPython repository (read-only)...${NC}"
    git clone "https://github.com/micropython/micropython.git" "${MICROPYTHON_REPO_PATH}"
    
    # Configure the repository to prevent accidental pushes to GitHub
    pushd "${MICROPYTHON_REPO_PATH}"
    echo -e "${YELLOW}Configuring repository to prevent GitHub commits...${NC}"
    # Remove the origin remote to prevent accidental pushes
    git remote remove origin 2>/dev/null || true
    # Set a dummy remote URL to prevent accidental remote operations
    git remote add origin "file:///dev/null"
    popd
fi

pushd "${MICROPYTHON_REPO_PATH}"
# Ensure no remote operations can happen
if git remote get-url origin 2>/dev/null | grep -q "github.com"; then
    echo -e "${YELLOW}WARNING: Removing GitHub remote to prevent accidental commits...${NC}"
    git remote remove origin
    git remote add origin "file:///dev/null"
fi

git checkout "${MICROPYTHON_VERSION}"
# Initialize submodules needed for the build
echo -e "${YELLOW}Initializing required submodules...${NC}"
git submodule update --init --recursive lib/uzlib lib/libm lib/libm_dbl
popd

# Build mpy-cross first
echo -e "${YELLOW}Building mpy-cross compiler...${NC}"
cd "$MICROPYTHON_REPO_PATH"
make -C mpy-cross V=1

# Clean previous build
echo -e "${YELLOW}Cleaning previous MicroPython embed build...${NC}"
cd "$MICROPYTHON_REPO_PATH/ports/embed"
# Set environment variables for the build
export MICROPYTHON_TOP="$MICROPYTHON_REPO_PATH"
make -f embed.mk clean-micropython-embed-package V=1

cd "$MICROPYTHON_LOCAL_PATH"
if [ -d "micropython_embed" ]; then
    echo -e "${YELLOW}Cleaning previous micropython_embed...${NC}"
    rm -rf micropython_embed
fi

# Copy our custom mpconfigport.h (and include) to the embed port so QSTR scan sees our extras
echo -e "${YELLOW}Setting up custom configuration for embed port...${NC}"
cp "$MICROPYTHON_LOCAL_PATH/port/mpconfigport.h" "$MICROPYTHON_REPO_PATH/ports/embed/"
# Ensure the include referenced by MICROPY_PY_MACHINE_INCLUDEFILE is visible to the embed build
mkdir -p "$MICROPYTHON_REPO_PATH/ports/embed/port"
cp "$MICROPYTHON_LOCAL_PATH/port/modmachine_jl.inc" "$MICROPYTHON_REPO_PATH/ports/embed/port/"



# Modify the embed.mk to include extmod modules
echo -e "${YELLOW}Modifying embed port to include extmod modules...${NC}"
cd "$MICROPYTHON_REPO_PATH/ports/embed"

# Create a backup of the original embed.mk
if [ ! -f "embed.mk.backup" ]; then
    cp embed.mk embed.mk.backup
fi

# Modify embed.mk to include extmod modules
cat > embed_with_extmod.mk << 'EOF'
# This file is part of the MicroPython project, http://micropython.org/
# Modified to include specific extmod modules for time, os

# Set the build output directory for the generated files.
BUILD = build-embed

# Include the core environment definitions; this will set $(TOP).
include $(MICROPYTHON_TOP)/py/mkenv.mk

# Include py core make definitions.
include $(TOP)/py/py.mk

# Define extmod source files we specifically want  
SRC_EXTMOD_C = \
	extmod/modtime.c \
	extmod/modplatform.c \
	extmod/moductypes.c \
	extmod/modmachine.c \

# Define shared source files we want
SRC_SHARED_C = \
	shared/readline/readline.c \

# Process extmod sources like regular sources
PY_O += $(addprefix $(BUILD)/, $(SRC_EXTMOD_C:.c=.o))
PY_O += $(addprefix $(BUILD)/, $(SRC_SHARED_C:.c=.o))
SRC_QSTR += $(SRC_EXTMOD_C)
SRC_QSTR += $(SRC_SHARED_C)

# Set the location of the MicroPython embed port.
MICROPYTHON_EMBED_PORT = $(MICROPYTHON_TOP)/ports/embed

# Set default makefile-level MicroPython feature configurations.
MICROPY_ROM_TEXT_COMPRESSION ?= 0

# Set CFLAGS for the MicroPython build.
CFLAGS += -I. -I$(TOP) -I$(BUILD) -I$(MICROPYTHON_EMBED_PORT) -I$(MICROPYTHON_EMBED_PORT)/port
CFLAGS += -Wall -Werror -std=c99

# Define the required generated header files.
GENHDR_OUTPUT = $(addprefix $(BUILD)/genhdr/, \
	moduledefs.h \
	mpversion.h \
	qstrdefs.generated.h \
	root_pointers.h \
	)

# Define the top-level target, the generated output files.
.PHONY: all
all: micropython-embed-package

clean: clean-micropython-embed-package

.PHONY: clean-micropython-embed-package
clean-micropython-embed-package:
	$(RM) -rf $(PACKAGE_DIR)

PACKAGE_DIR ?= micropython_embed
PACKAGE_DIR_LIST = $(addprefix $(PACKAGE_DIR)/,py extmod shared/runtime shared/timeutils shared/readline genhdr port drivers/bus)

.PHONY: micropython-embed-package
micropython-embed-package: $(GENHDR_OUTPUT)
	$(ECHO) "Generate micropython_embed output:"
	$(Q)$(RM) -rf $(PACKAGE_DIR_LIST)
	$(Q)$(MKDIR) -p $(PACKAGE_DIR_LIST)
	$(ECHO) "- py"
	$(Q)$(CP) $(TOP)/py/*.[ch] $(PACKAGE_DIR)/py
	$(ECHO) "- extmod (specific modules only)"
	$(Q)$(CP) $(TOP)/extmod/modtime.c $(PACKAGE_DIR)/extmod
	$(Q)$(CP) $(TOP)/extmod/modtime.h $(PACKAGE_DIR)/extmod
	$(Q)$(CP) $(TOP)/extmod/modplatform.c $(PACKAGE_DIR)/extmod
	$(Q)$(CP) $(TOP)/extmod/modplatform.h $(PACKAGE_DIR)/extmod
	$(Q)$(CP) $(TOP)/extmod/moductypes.c $(PACKAGE_DIR)/extmod
	$(Q)$(CP) $(TOP)/extmod/misc.h $(PACKAGE_DIR)/extmod
	# Provide machine glue header for ports and its dependencies
	$(Q)$(CP) $(TOP)/extmod/modmachine.h $(PACKAGE_DIR)/extmod
	# Machine module implementation
	$(Q)$(CP) $(TOP)/extmod/modmachine.c $(PACKAGE_DIR)/extmod
	# Drivers headers needed by modmachine.h
	$(Q)$(MKDIR) -p $(PACKAGE_DIR)/drivers/bus || true
	$(Q)$(CP) $(TOP)/drivers/bus/spi.h $(PACKAGE_DIR)/drivers/bus
	# Skip machine_uart sources for embed build; not needed unless enabling UART

	$(ECHO) "- shared"
	$(Q)$(CP) $(TOP)/shared/runtime/gchelper.h $(PACKAGE_DIR)/shared/runtime
	$(Q)$(CP) $(TOP)/shared/runtime/gchelper_generic.c $(PACKAGE_DIR)/shared/runtime
	$(Q)$(CP) $(TOP)/shared/runtime/pyexec.h $(PACKAGE_DIR)/shared/runtime
	$(Q)$(CP) $(TOP)/shared/runtime/pyexec.c $(PACKAGE_DIR)/shared/runtime
	$(Q)$(CP) $(TOP)/shared/runtime/mpirq.h $(PACKAGE_DIR)/shared/runtime
	$(Q)$(MKDIR) -p $(PACKAGE_DIR)/shared/timeutils || true
	$(Q)$(CP) $(TOP)/shared/timeutils/*.h $(PACKAGE_DIR)/shared/timeutils || true
	$(Q)$(CP) $(TOP)/shared/timeutils/*.c $(PACKAGE_DIR)/shared/timeutils || true
	$(ECHO) "- shared/readline"
	$(Q)$(MKDIR) -p $(PACKAGE_DIR)/shared/readline || true
	$(Q)$(CP) $(TOP)/shared/readline/*.h $(PACKAGE_DIR)/shared/readline || true
	$(Q)$(CP) $(TOP)/shared/readline/*.c $(PACKAGE_DIR)/shared/readline || true
	$(ECHO) "- genhdr"
	$(Q)$(CP) $(GENHDR_OUTPUT) $(PACKAGE_DIR)/genhdr
	$(ECHO) "- port"
	$(Q)$(CP) $(MICROPYTHON_EMBED_PORT)/port/*.[ch] $(PACKAGE_DIR)/port

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk
EOF

# Build the embed port with Jumperless module
echo -e "${YELLOW}Building MicroPython embed port with Jumperless module...${NC}"
echo -e "${YELLOW}This includes built-in modules: time, machine, os, math, etc.${NC}"
cd "$MICROPYTHON_REPO_PATH/ports/embed"

# Set environment variables for the build
export MICROPYTHON_TOP="$MICROPYTHON_REPO_PATH"
export USER_C_MODULES="$PROJECT_ROOT/modules"

# Build the embed port using the modified makefile that includes extmod
make -f embed_with_extmod.mk PACKAGE_DIR="$MICROPYTHON_LOCAL_PATH/micropython_embed" V=1

# Copy mpconfigport.h to the micropython_embed directory so it can be found during PlatformIO compilation
echo -e "${YELLOW}Copying mpconfigport.h to micropython_embed directory...${NC}"

cp "$MICROPYTHON_LOCAL_PATH/port/mpconfigport.h" "$MICROPYTHON_LOCAL_PATH/micropython_embed/"

# Do not remove machine module; we include it properly now

# echo -e "${RED}$MICROPYTHON_LOCAL_PATH/micropython_embed/port/mphalport.h${NC}"
if [  -f "$MICROPYTHON_LOCAL_PATH/micropython_embed/port/mphalport.h" ]; then
    echo -e "${YELLOW}Removing embed port's mphalport files...${NC}"
    rm "$MICROPYTHON_LOCAL_PATH/micropython_embed/port/mphalport.h"
    rm "$MICROPYTHON_LOCAL_PATH/micropython_embed/port/mphalport.c"
fi

# Verify the build
if [ -f "$MICROPYTHON_LOCAL_PATH/micropython_embed/genhdr/qstrdefs.generated.h" ]; then
    QSTR_COUNT=$(grep -c "^QDEF" "$MICROPYTHON_LOCAL_PATH/micropython_embed/genhdr/qstrdefs.generated.h" || true)
    JUMPERLESS_QSTRS=$(grep -c "jumperless\|dac_set\|adc_get\|nodes_connect" "$MICROPYTHON_LOCAL_PATH/micropython_embed/genhdr/qstrdefs.generated.h" || true)
    TIME_QSTRS=$(grep -c "time\|sleep\|ticks" "$MICROPYTHON_LOCAL_PATH/micropython_embed/genhdr/qstrdefs.generated.h" || true)
    MACHINE_QSTRS=$(grep -c "machine\|Pin\|ADC\|PWM" "$MICROPYTHON_LOCAL_PATH/micropython_embed/genhdr/qstrdefs.generated.h" || true)
    
    echo -e "${GREEN}◆ MicroPython embed build successful!${NC}"
    echo -e "${GREEN}   Generated $QSTR_COUNT total QSTR definitions${NC}"
    if [ "$JUMPERLESS_QSTRS" -gt 0 ]; then
        echo -e "${GREEN}   Jumperless module QSTRs found: $JUMPERLESS_QSTRS${NC}"
    else
        echo -e "${YELLOW}   Warning: No Jumperless module QSTRs detected${NC}"
    fi
    echo -e "${GREEN}   Time module QSTRs found: $TIME_QSTRS${NC}"
    echo -e "${GREEN}   Machine module QSTRs found: $MACHINE_QSTRS${NC}"
    echo -e "${GREEN}   Files ready for PlatformIO integration with embed API${NC}"
    echo -e "${GREEN}   Available modules: time, machine, os, math, gc, array, etc.${NC}"
else
    echo -e "${RED}◇ MicroPython embed build failed!${NC}"
    echo -e "${RED}   qstrdefs.generated.h not found${NC}"
    exit 1
fi

# Verify Jumperless module files are present
echo -e "${YELLOW}Verifying Jumperless module integration...${NC}"
if [ -f "$PROJECT_ROOT/modules/jumperless/modjumperless.c" ]; then
    echo -e "${GREEN}   ◆ Jumperless MicroPython module found${NC}"
else
    echo -e "${RED}   ◇ Jumperless MicroPython module missing${NC}"
fi

if [ -f "$PROJECT_ROOT/src/JumperlessMicroPythonAPI.cpp" ]; then
    echo -e "${GREEN}   ◆ Jumperless API wrapper found${NC}"
else
    echo -e "${RED}   ◇ Jumperless API wrapper missing${NC}"
fi

echo -e "${GREEN}◆ MicroPython embed port is ready with built-in modules!${NC}"
echo -e "${GREEN}   You can now use: mp_embed_init(), mp_embed_exec_str(), mp_embed_deinit()${NC}"
echo -e "${GREEN}   Available modules: import time, import machine, import os, etc.${NC}" 