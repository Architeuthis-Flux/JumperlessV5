Import("env")
import os
import glob

# Add MicroPython include paths only for MicroPython source files
def setup_micropython_build():
    # Base path for MicroPython
    mp_base = "src/micropython/micropython_embed"
    
    # Include paths needed for MicroPython compilation
    mp_includes = [
        "src/micropython",
        "src/micropython/micropython_embed", 
        "src/micropython/micropython_embed/genhdr",
        "src/micropython/micropython_embed/py",
        "src/micropython/micropython_embed/shared/runtime",
        "src/micropython/micropython_embed/port",
    ]
    
    # Find all MicroPython source files
    mp_c_files = []
    mp_cpp_files = []
    
    for root, dirs, files in os.walk(mp_base):
        for file in files:
            file_path = os.path.join(root, file)
            if file.endswith('.c'):
                mp_c_files.append(file_path)
            elif file.endswith('.cpp'):
                mp_cpp_files.append(file_path)
    
    print(f"Found {len(mp_c_files)} C files and {len(mp_cpp_files)} C++ files in MicroPython")
    
    # Collect all MicroPython object files to add to linking
    mp_objects = []
    
    # Add the MicroPython source files back to the build with specific include paths
    for c_file in mp_c_files:
        obj_path = env.Object(
            target=c_file.replace('/', '_').replace('.c', '.c.o'),
            source=c_file,
            CPPPATH=env.get("CPPPATH", []) + [env.Dir(d) for d in mp_includes]
        )
        mp_objects.append(obj_path)
    
    for cpp_file in mp_cpp_files:
        obj_path = env.Object(
            target=cpp_file.replace('/', '_').replace('.cpp', '.cpp.o'),
            source=cpp_file,
            CPPPATH=env.get("CPPPATH", []) + [env.Dir(d) for d in mp_includes]
        )
        mp_objects.append(obj_path)
    
    # Add all MicroPython objects to the program
    env.Append(LIBS=mp_objects)

# Execute the setup
setup_micropython_build() 