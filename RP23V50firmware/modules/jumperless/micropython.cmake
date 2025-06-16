# Create an INTERFACE library for the Jumperless C module
add_library(usermod_jumperless INTERFACE)

# Add the Jumperless module source files
target_sources(usermod_jumperless INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/modjumperless.c
   
)

# Add the current directory as an include directory
target_include_directories(usermod_jumperless INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

# Link the INTERFACE library to the usermod target
target_link_libraries(usermod INTERFACE usermod_jumperless) 