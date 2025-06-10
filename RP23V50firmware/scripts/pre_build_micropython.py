#!/usr/bin/env python3
"""
Pre-build script for PlatformIO to build MicroPython embed port
This ensures all necessary MicroPython files including QSTR definitions are generated
before the main compilation begins.
"""

import os
import sys
import subprocess
from pathlib import Path

Import("env", "projenv")

def check_micropython_repo():
    """Check if MicroPython repository exists"""
    micropython_path = Path.home() / "src" / "micropython" / "micropython"
    if not micropython_path.exists():
        print("❌ MicroPython repository not found!")
        print(f"   Expected at: {micropython_path}")
        print("\n🔧 To fix this, run:")
        print("   mkdir -p ~/src/micropython")
        print("   cd ~/src/micropython")
        print("   git clone https://github.com/micropython/micropython.git")
        print("   cd micropython/mpy-cross")
        print("   make")
        sys.exit(1)
    return micropython_path

def check_qstr_files():
    """Check if QSTR files need to be regenerated"""
    project_root = Path(env.get("PROJECT_DIR"))
    micropython_dir = project_root / "src" / "micropython"
    qstr_file = micropython_dir / "micropython_embed" / "genhdr" / "qstrdefs.generated.h"
    
    # Always rebuild if qstr file doesn't exist
    if not qstr_file.exists():
        return True
    
    # Check if any source files are newer than the qstr file
    config_file = micropython_dir / "mpconfigport.h"
    if config_file.exists() and config_file.stat().st_mtime > qstr_file.stat().st_mtime:
        return True
        
    return False

def build_micropython():
    """Build the MicroPython embed port"""
    print("🔨 Building MicroPython embed port...")
    
    project_root = Path(env.get("PROJECT_DIR"))
    micropython_path = check_micropython_repo()
    micropython_dir = project_root / "src" / "micropython"
    
    # Change to micropython directory
    os.chdir(str(micropython_dir))
    
    # Build command
    cmd = [
        "make", 
        "-f", "micropython_embed.mk", 
        f"MICROPYTHON_TOP={micropython_path}",
        "V=1"  # Verbose output for debugging
    ]
    
    try:
        # Clean previous build if needed
        if (micropython_dir / "micropython_embed").exists():
            print("🧹 Cleaning previous MicroPython build...")
            subprocess.run(["make", "-f", "micropython_embed.mk", f"MICROPYTHON_TOP={micropython_path}", "clean"], 
                         check=True, capture_output=True)
        
        # Run the build
        print(f"   Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        
        # Check if build was successful
        qstr_file = micropython_dir / "micropython_embed" / "genhdr" / "qstrdefs.generated.h"
        if qstr_file.exists():
            # Count generated QSTRs
            with open(qstr_file, 'r') as f:
                qstr_count = sum(1 for line in f if line.startswith('QDEF'))
            print(f"✅ MicroPython build successful! Generated {qstr_count} QSTR definitions")
            print("🎯 Floating point support enabled for RP2350 FPU")
        else:
            print("❌ MicroPython build failed - qstrdefs.generated.h not found")
            sys.exit(1)
            
    except subprocess.CalledProcessError as e:
        print(f"❌ MicroPython build failed with error:")
        print(f"   Return code: {e.returncode}")
        if e.stdout:
            print(f"   STDOUT: {e.stdout}")
        if e.stderr:
            print(f"   STDERR: {e.stderr}")
        sys.exit(1)
    
    # Return to original directory
    os.chdir(str(project_root))

def main():
    """Main pre-build function"""
    print("🚀 Pre-build: Checking MicroPython embed port...")
    
    # Always check and build MicroPython to ensure QSTR files are up to date
    if check_qstr_files():
        print("⚡ MicroPython rebuild required")
        build_micropython()
    else:
        print("✅ MicroPython embed port is up to date")
        print("🎯 Floating point support ready for RP2350 FPU")

# Run the pre-build check
if __name__ == "__main__":
    main()
else:
    # Called by PlatformIO
    main() 