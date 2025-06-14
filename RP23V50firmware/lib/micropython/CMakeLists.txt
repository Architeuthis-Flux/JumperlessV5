# MicroPython embed library for PlatformIO
# This includes all the pre-built MicroPython source files

set(MICROPYTHON_EMBED_DIR ${CMAKE_CURRENT_LIST_DIR}/micropython_embed)

# Include all MicroPython core source files
file(GLOB_RECURSE MICROPYTHON_SOURCES
    ${MICROPYTHON_EMBED_DIR}/py/*.c
    ${MICROPYTHON_EMBED_DIR}/shared/runtime/*.c
    ${MICROPYTHON_EMBED_DIR}/port/*.c
)

# Create the micropython library
add_library(micropython STATIC ${MICROPYTHON_SOURCES})

# Add include directories
target_include_directories(micropython PUBLIC
    ${MICROPYTHON_EMBED_DIR}
    ${MICROPYTHON_EMBED_DIR}/py
    ${MICROPYTHON_EMBED_DIR}/port
    ${MICROPYTHON_EMBED_DIR}/genhdr
    ${MICROPYTHON_EMBED_DIR}/shared/runtime
)

# Set compile flags for MicroPython
target_compile_definitions(micropython PUBLIC
    MICROPY_ENABLE_GC=1
    MICROPY_HELPER_REPL=1
    MODULE_JUMPERLESS_ENABLED=1
) 