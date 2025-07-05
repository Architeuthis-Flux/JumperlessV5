#!/usr/bin/env python3
"""
Jumperless Linux Python Launcher Wrapper
Cross-platform launcher that can be executed directly
"""
import os
import sys
import subprocess
import time
import signal

def kill_existing_instances():
    """Kill existing Jumperless instances on Linux"""
    print("Checking for existing Jumperless instances...")
    
    # Close terminal windows using xdotool if available
    try:
        subprocess.run(['xdotool', 'search', '--name', 'Jumperless', 'windowclose'], 
                      capture_output=True, check=False)
        subprocess.run(['xdotool', 'search', '--name', 'jumperless', 'windowclose'], 
                      capture_output=True, check=False)
    except Exception:
        pass
    
    # Try wmctrl for window management
    try:
        subprocess.run(['wmctrl', '-c', 'Jumperless'], 
                      capture_output=True, check=False)
        subprocess.run(['wmctrl', '-c', 'jumperless'], 
                      capture_output=True, check=False)
    except Exception:
        pass
    
    # Kill processes
    process_patterns = [
        "Jumperless_cli",
        "jumperless_cli", 
        "JumperlessWokwiBridge",
        "Jumperless.app"
    ]
    
    for pattern in process_patterns:
        try:
            subprocess.run(['pkill', '-f', pattern], 
                          capture_output=True, check=False)
        except Exception:
            pass
    
    time.sleep(1)
    print("Cleared existing instances and terminal windows.")

def main():
    """Main launcher function"""
    # Get script directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Kill existing instances
    kill_existing_instances()
    
    print("\\n" + "="*50)
    print("    Jumperless Linux Python Launcher")
    print("="*50 + "\\n")
    
    # Find Python
    python_cmd = None
    for cmd in ['python3', 'python']:
        try:
            result = subprocess.run([cmd, '--version'], 
                                  capture_output=True, check=True)
            python_cmd = cmd
            print(f"✅ Python found: {cmd}")
            break
        except Exception:
            continue
    
    if not python_cmd:
        print("❌ Python 3 not found")
        input("\\nPress Enter to exit...")
        return 1
    
    # Install requirements
    requirements_file = os.path.join(script_dir, 'requirements.txt')
    if os.path.exists(requirements_file):
        print("📦 Installing Python dependencies...")
        try:
            subprocess.run([python_cmd, '-m', 'pip', 'install', '-r', requirements_file], 
                          check=True)
            print("✅ Dependencies installed successfully")
        except subprocess.CalledProcessError:
            print("⚠️  Warning: Some dependencies may not have installed")
    
    # Run main application
    print("\\n🚀 Starting Jumperless Bridge...")
    
    main_script = os.path.join(script_dir, 'JumperlessWokwiBridge.py')
    if not os.path.exists(main_script):
        print(f"❌ Main script not found: {main_script}")
        input("Press Enter to exit...")
        return 1
    
    try:
        # Change to script directory and run
        os.chdir(script_dir)
        result = subprocess.run([python_cmd, 'JumperlessWokwiBridge.py'] + sys.argv[1:])
        return result.returncode
    except KeyboardInterrupt:
        print("\\n\\n⚠️  Interrupted by user")
        return 0
    except Exception as e:
        print(f"❌ Error running Jumperless: {e}")
        input("Press Enter to exit...")
        return 1

if __name__ == "__main__":
    sys.exit(main())
