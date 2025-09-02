# MicroPython RP2 port library for PlatformIO
# This includes all the pre-built MicroPython source files from the RP2 port

set(MICROPYTHON_EMBED_DIR ${CMAKE_CURRENT_LIST_DIR}/micropython_embed)

# Include all MicroPython core source files
file(GLOB_RECURSE MICROPYTHON_SOURCES
    ${MICROPYTHON_EMBED_DIR}/py/*.c
    ${MICROPYTHON_EMBED_DIR}/shared/runtime/*.c
    ${MICROPYTHON_EMBED_DIR}/port/*.c
)

# Exclude files that might conflict with our custom implementations
list(FILTER MICROPYTHON_SOURCES EXCLUDE REGEX ".*mphalport\\.c$")
list(FILTER MICROPYTHON_SOURCES EXCLUDE REGEX ".*main\\.c$")

# Create the micropython library
add_library(micropython STATIC ${MICROPYTHON_SOURCES})

# Add include directories for RP2 port
target_include_directories(micropython PUBLIC
    ${MICROPYTHON_EMBED_DIR}
    ${MICROPYTHON_EMBED_DIR}/py
    ${MICROPYTHON_EMBED_DIR}/port
    ${MICROPYTHON_EMBED_DIR}/genhdr
    ${MICROPYTHON_EMBED_DIR}/shared/readline
    ${MICROPYTHON_EMBED_DIR}/shared/runtime
    ${MICROPYTHON_EMBED_DIR}/shared/netutils
    ${MICROPYTHON_EMBED_DIR}/shared/timeutils
    ${MICROPYTHON_EMBED_DIR}/shared/tinyusb
    ${MICROPYTHON_EMBED_DIR}/port/rp2_common
)

# Set compile flags for MicroPython RP2 port
target_compile_definitions(micropython PUBLIC
    # MICROPY_ENABLE_GC=0
    # MICROPY_HELPER_REPL=1
    # MODULE_JUMPERLESS_ENABLED=1
    # # RP2 port specific defines
    # MICROPY_PY_MACHINE_UART=1
    # JL_USE_PICO_HAL=1
    # MICROPY_CONFIG_ROM_LEVEL=50
    # MICROPY_PY_MACHINE=1
    # MICROPY_PY_TIME=1
    # MICROPY_PY_OS=1
    # MICROPY_VFS=1
    # MICROPY_VFS_LFS2=1
    # MICROPY_VFS_FAT=1
    # MICROPY_PY_THREAD=1
    PICO_RP2350=1
    PICO_ARM=1
) 