import PyInstaller.__main__
import pathlib
import os
import platform
import shutil
import time
import subprocess

# macOS packaging paths
generated_app_path = pathlib.Path("/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/dist/Jumperless.app/Contents/MacOS/Jumperless")
generated_app_path_renamed = pathlib.Path("/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/dist/Jumperless.app/Contents/MacOS/Jumperless_cli")
target_app_path = pathlib.Path("/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/dist/Jumperless.app/Contents/MacOS/")
icon_path = pathlib.Path("icon.icns")
nonexec_launcher_path = pathlib.Path("jumperless_cli_launcher.sh")
launcher_path = pathlib.Path("Jumperless")
app_path = pathlib.Path("Jumperless.app")
apple_silicon_folder = pathlib.Path("apple silicon/Jumperless.app")
intel_folder = pathlib.Path("intel mac/Jumperless.app")

# Linux packaging paths
linux_dist_path_x64 = pathlib.Path("dist/JumperlessLinux_x64")
linux_appdir_path_x64 = pathlib.Path("JumperlessLinux_x64.AppDir")
appimage_path_x64 = pathlib.Path("Jumperless-x86_64.AppImage")

linux_dist_path_arm = pathlib.Path("dist/JumperlessLinux_arm64")
linux_appdir_path_arm = pathlib.Path("JumperlessLinux_arm64.AppDir")
appimage_path_arm = pathlib.Path("Jumperless-aarch64.AppImage")

# Windows packaging paths
windows_dist_path_x64 = pathlib.Path("dist/JumperlessWindows_x64")
windows_installer_dir = pathlib.Path("JumperlessWindows")
windows_exe_path = pathlib.Path("Jumperless-Windows-x64.exe")
windows_zip_path = pathlib.Path("Jumperless-Windows-x64.zip")

# Common paths
appdist_path = pathlib.Path("dist/Jumperless.app")
dmg_folder = pathlib.Path("JumperlessDMG/Jumperless.app")
python_folder = pathlib.Path("Jumperless Python/")

def check_and_update_requirements():
    """Generate new requirements.txt and update new_requirements flag if they differ"""
    print("=== Checking Requirements ===")
    
    try:
        # Define the packages actually used by JumperlessWokwiBridge.py
        # Based on the imports in the file
        core_packages = [
            'requests',        # For HTTP requests to Wokwi API and GitHub
            'pyserial',        # For serial communication (imported as 'serial')
            'psutil',          # For system/process monitoring
            'beautifulsoup4',  # For HTML parsing (imported as 'bs4')
            # 'pynput',         # For keyboard and mouse input (optional but recommended)
            # 'pywin32',         # For Windows volume detection and APIs
            # 'pyduinocli',      # For Arduino CLI support (optional but recommended)
            # 'colorama',        # For cross-platform colored output (optional but recommended)
            # 'packaging',       # For version comparison (optional but recommended)
            
        ]
        
        # Optional but recommended packages
        optional_packages = [
            'packaging',       # For version comparison (optional but recommended)
            'pyduinocli',      # For Arduino CLI support (optional but recommended)
            'colorama',        # For cross-platform colored output (optional but recommended)
        ]
        
        # Platform-specific requirements
        platform_specific_requirements = [
          'pywin32>=306; platform_system == "Windows"',  # Windows volume detection and APIs
          'pynput; platform_system == "Windows"',         # For keyboard and mouse input (optional but recommended)
        ]
        
        print("Generating minimal requirements for JumperlessWokwiBridge.py...")
        
        # Get current pip freeze output
        result = subprocess.run([
            "python3", "-m", "pip", "freeze"
        ], capture_output=True, text=True, check=True)
        
        all_installed = result.stdout.strip().splitlines()
        
        # Create a dictionary of installed packages
        installed_packages = {}
        for line in all_installed:
            if '==' in line:
                name, version = line.split('==', 1)
                installed_packages[name.lower()] = f"{name}=={version}"
        
        # Build minimal requirements list
        minimal_requirements = []
        missing_packages = []
        
        # Process core packages (required)
        for package in core_packages:
            lookup_name = package.lower()
            
            # Special case mappings
            if lookup_name == 'pyserial':
                # pyserial is installed as 'pyserial' but imported as 'serial'
                lookup_name = 'pyserial'
            elif lookup_name == 'beautifulsoup4':
                # beautifulsoup4 is imported as 'bs4'
                lookup_name = 'beautifulsoup4'
            
            if lookup_name in installed_packages:
                minimal_requirements.append(installed_packages[lookup_name])
                print(f"✅ Found core: {installed_packages[lookup_name]}")
            else:
                missing_packages.append(package)
                print(f"❌ Missing core: {package}")
        
        # Process optional packages
        for package in optional_packages:
            lookup_name = package.lower()
            
            if lookup_name in installed_packages:
                minimal_requirements.append(installed_packages[lookup_name])
                print(f"✅ Found optional: {installed_packages[lookup_name]}")
            else:
                print(f"⚠️  Optional not installed: {package}")
        
        # Add platform-specific requirements
        for req in platform_specific_requirements:
            minimal_requirements.append(req)
            print(f"🖥️  Added platform-specific: {req}")
        
        if missing_packages:
            print(f"\n❌ Missing required core packages: {', '.join(missing_packages)}")
            print("Please install them with: pip install " + ' '.join(missing_packages))
            return False
        
        # Sort requirements for consistency (but keep platform-specific at the end)
        regular_requirements = [req for req in minimal_requirements if ';' not in req]
        platform_requirements = [req for req in minimal_requirements if ';' in req]
        
        regular_requirements.sort()
        final_requirements = regular_requirements + platform_requirements
        
        new_requirements_content = '\n'.join(final_requirements)
        
        # Read existing requirements.txt if it exists
        existing_requirements_path = pathlib.Path("requirements.txt")
        existing_requirements_content = ""
        
        if existing_requirements_path.exists():
            with open(existing_requirements_path, 'r', encoding='utf-8') as f:
                existing_requirements_content = f.read().strip()
        
        # Compare requirements
        requirements_changed = new_requirements_content != existing_requirements_content
        
        if requirements_changed:
            print("📝 Requirements have changed!")
            
            # Write new minimal requirements.txt with header comment
            requirements_header = "# JumperlessWokwiBridge.py Requirements\n# Auto-generated minimal dependencies\n\n"
            
            with open(existing_requirements_path, 'w', encoding='utf-8') as f:
                f.write(requirements_header + new_requirements_content)
            print(f"✅ Updated requirements.txt with minimal dependencies")
            
            # Update new_requirements flag in JumperlessWokwiBridge.py
            bridge_file_path = pathlib.Path("JumperlessWokwiBridge.py")
            
            if bridge_file_path.exists():
                with open(bridge_file_path, 'r', encoding='utf-8') as f:
                    bridge_content = f.read()
                
                # Replace the new_requirements flag
                updated_content = bridge_content.replace(
                    "new_requirements = False",
                    "new_requirements = True"
                )
                
                if updated_content != bridge_content:
                    with open(bridge_file_path, 'w', encoding='utf-8') as f:
                        f.write(updated_content)
                    print("✅ Updated new_requirements flag to True in JumperlessWokwiBridge.py")
                else:
                    print("⚠️  Could not find 'new_requirements = False' in JumperlessWokwiBridge.py")
            else:
                print("⚠️  JumperlessWokwiBridge.py not found")
        else:
            print("✅ Requirements unchanged")
            
            # Ensure new_requirements is False when requirements haven't changed
            bridge_file_path = pathlib.Path("JumperlessWokwiBridge.py")
            
            if bridge_file_path.exists():
                with open(bridge_file_path, 'r', encoding='utf-8') as f:
                    bridge_content = f.read()
                
                # Make sure flag is False
                updated_content = bridge_content.replace(
                    "new_requirements = True",
                    "new_requirements = False"
                )
                
                if updated_content != bridge_content:
                    with open(bridge_file_path, 'w', encoding='utf-8') as f:
                        f.write(updated_content)
                    print("✅ Ensured new_requirements flag is False in JumperlessWokwiBridge.py")
        
        # Show summary
        print(f"📊 Total requirements: {len(final_requirements)} packages")
        print("📦 Core packages:")
        for req in regular_requirements:
            if any(core in req.lower() for core in ['requests', 'pyserial', 'psutil', 'beautifulsoup4']):
                print(f"   • {req} (required)")
            else:
                print(f"   • {req} (optional)")
        
        if platform_requirements:
            print("🖥️  Platform-specific:")
            for req in platform_requirements:
                print(f"   • {req}")
        
        return requirements_changed
        
    except subprocess.CalledProcessError as e:
        print(f"❌ Error generating requirements: {e}")
        print("Make sure you're in the correct virtual environment")
        return False
    except Exception as e:
        print(f"❌ Error checking requirements: {e}")
        return False

def package_macos():
    """Package for macOS"""
    print("=== Packaging for macOS ===")
    
    os.system(f"python3 -m PyInstaller --icon=\"/Users/kevinsanto/Documents/GitHub/JumperlessV5/jumperlesswokwibridge/icon.icns\" \
    -y \
    --console \
    --windowed \
    --path \"/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/.venv/lib/python3.13/site-packages\" \
    JumperlessWokwiBridge.py \
    --name Jumperless ")

    time.sleep(4)

    # Prepare launcher script
    print("Setting up launcher script...")
    os.system(f"chmod 755 {nonexec_launcher_path}")
    print("Changed permissions for " + str(nonexec_launcher_path) + '\n')

    print("Creating Jumperless launcher...")
    os.system(f'cp {nonexec_launcher_path} {launcher_path}')
    print("Copied launcher script to " + str(launcher_path) + '\n')

    print("Renaming main app to Jumperless_cli...")
    os.rename(generated_app_path, generated_app_path_renamed)
    print("Renamed " + str(generated_app_path) + " to " + str(generated_app_path_renamed) + '\n')

    print("Installing launcher into app bundle...")
    os.system(f"cp {launcher_path} {target_app_path}")
    print("Copied " + str(launcher_path) + " to " + str(target_app_path) + '\n')

    print("Copying app to DMG folder...")
    shutil.copytree(appdist_path, dmg_folder, dirs_exist_ok=True)
    print("Copied " + str(app_path) + " to " + str(dmg_folder) + '\n')

    print("Updating Python files in DMG...")
    # Delete existing Python files to ensure fresh copies
    python_bridge_file = python_folder / "JumperlessWokwiBridge.py"
    python_requirements_file = python_folder / "requirements.txt"
    python_launcher_file = python_folder / "jumperless_launcher.sh"
    
    if python_bridge_file.exists():
        os.remove(python_bridge_file)
        print("Deleted old JumperlessWokwiBridge.py")
    
    if python_requirements_file.exists():
        os.remove(python_requirements_file)
        print("Deleted old requirements.txt")
    
    if python_launcher_file.exists():
        os.remove(python_launcher_file)
        print("Deleted old launcher script")
    
    # Copy latest files
    shutil.copy2("JumperlessWokwiBridge.py", python_folder)
    shutil.copy2("requirements.txt", python_folder)
    shutil.copy2("jumperless_launcher.sh", python_folder)
    os.chmod(python_folder / "jumperless_launcher.sh", 0o755)
    
    print("Copied latest Python files and macOS launcher to " + str(python_folder) + '\n')

    print("Creating DMG installer...")
    os.system(f"rm -f Jumperless_Installer.dmg")
    os.chmod("createDMG.sh", 0o755)
    os.system("./createDMG.sh")

def create_appimage_for_arch(arch_name, dist_path, appdir_path, appimage_path, executable_name):
    """Helper function to create AppImage for a specific architecture"""
    print(f"Creating {arch_name} AppImage structure...")
    
    # Clean and create AppDir
    if appdir_path.exists():
        shutil.rmtree(appdir_path)
    appdir_path.mkdir()
    
    # Copy executable and dependencies
    if dist_path.exists():
        try:
            # First try normal copy
            shutil.copytree(dist_path, appdir_path / "usr" / "bin", dirs_exist_ok=True)
        except shutil.Error as e:
            print(f"Warning: Standard copy failed due to symbolic links: {e}")
            print("Attempting to copy with symbolic link resolution...")
            
            # Create the target directory
            target_dir = appdir_path / "usr" / "bin"
            target_dir.mkdir(parents=True, exist_ok=True)
            
            # Copy files manually, resolving symbolic links
            for item in dist_path.rglob('*'):
                if item.is_file():
                    # Calculate relative path
                    rel_path = item.relative_to(dist_path)
                    target_path = target_dir / rel_path
                    
                    # Create parent directories
                    target_path.parent.mkdir(parents=True, exist_ok=True)
                    
                    # Copy file, following symbolic links
                    if item.is_symlink():
                        try:
                            # Resolve the symbolic link and copy the actual file
                            real_path = item.resolve()
                            if real_path.exists():
                                shutil.copy2(real_path, target_path)
                            else:
                                print(f"Warning: Broken symlink {item} -> {real_path}")
                        except (OSError, RuntimeError):
                            print(f"Warning: Could not resolve symlink {item}")
                    else:
                        shutil.copy2(item, target_path)
                elif item.is_dir() and not item.is_symlink():
                    # Create directory
                    rel_path = item.relative_to(dist_path)
                    target_path = target_dir / rel_path
                    target_path.mkdir(parents=True, exist_ok=True)
    else:
        print(f"Warning: {dist_path} not found, skipping {arch_name}")
        return False
    
    # Create desktop entry
    desktop_content = f"""[Desktop Entry]
Type=Application
Name=Jumperless {arch_name}
Exec=AppRun
Icon=jumperless
Comment=Jumperless Wokwi Bridge ({arch_name})
Categories=Development;Electronics;
Terminal=true
"""
    
    with open(appdir_path / "jumperless.desktop", "w", encoding='utf-8') as f:
        f.write(desktop_content)
    
    # Create AppRun script
    apprun_content = f"""#!/bin/bash
# AppRun script for Jumperless {arch_name}

# Get the directory where this AppImage is located
HERE="$(dirname "$(readlink -f "${{BASH_SOURCE[0]}}")")"

# Make launcher script executable
chmod +x "$HERE/jumperless_cli_launcher.sh"

# Run the launcher script
exec "$HERE/jumperless_cli_launcher.sh" "$@"
"""
    
    apprun_path = appdir_path / "AppRun"
    with open(apprun_path, "w", encoding='utf-8') as f:
        f.write(apprun_content)
    os.chmod(apprun_path, 0o755)
    
    # Copy launcher script
    launcher_source = "jumperless_cli_launcher.sh"
    if os.path.exists(launcher_source):
        shutil.copy2(launcher_source, appdir_path / "jumperless_cli_launcher.sh")
        os.chmod(appdir_path / "jumperless_cli_launcher.sh", 0o755)
    else:
        print(f"Warning: {launcher_source} not found for AppImage")
    
    # Copy icon (convert to PNG if needed)
    if os.path.exists("icon.png"):
        shutil.copy2("icon.png", appdir_path / "jumperless.png")
    elif os.path.exists("icon.icns"):
        # Try to convert icns to png using sips (macOS) or ImageMagick
        try:
            os.system(f"sips -s format png icon.icns --out {appdir_path}/jumperless.png")
        except:
            try:
                os.system(f"convert icon.icns {appdir_path}/jumperless.png")
            except:
                print("Warning: Could not convert icon. Creating placeholder.")
                # Create a simple placeholder icon
                with open(appdir_path / "jumperless.png", "w", encoding='utf-8') as f:
                    f.write("")  # Placeholder
    
    return True

def download_appimagetool():
    """Download AppImage build tool if not present"""
    if os.path.exists("appimagetool-x86_64.AppImage"):
        return True
    
    # Check if we're on a platform that can run the AppImage tool
    current_platform = platform.machine().lower()
    current_os = platform.system()
    
    if current_os == "Darwin" and current_platform in ['arm64', 'aarch64']:
        print("⚠️  Running on Apple Silicon macOS - AppImage tools may not work")
        print("   Will create tar.gz packages instead")
        return False
    
    print("Downloading appimagetool...")
    # Try curl first (available on macOS), then wget (Linux)
    download_success = False
    if os.system("which curl > /dev/null 2>&1") == 0:
        if os.system("curl -L -o appimagetool-x86_64.AppImage https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage") == 0:
            download_success = True
    elif os.system("which wget > /dev/null 2>&1") == 0:
        if os.system("wget -q https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage") == 0:
            download_success = True
    
    if download_success and os.path.exists("appimagetool-x86_64.AppImage"):
        os.chmod("appimagetool-x86_64.AppImage", 0o755)
        
        # Test if the AppImage tool can actually run
        test_result = os.system("./appimagetool-x86_64.AppImage --help > /dev/null 2>&1")
        if test_result != 0:
            print("⚠️  Downloaded AppImage tool cannot run on this platform")
            print("   Will create tar.gz packages instead")
            return False
        
        return True
    else:
        print("Warning: Could not download appimagetool.")
        return False

def package_linux_appimage():
    """Package for Linux as AppImage (both x86_64 and ARM64)"""
    print("=== Packaging Linux AppImages ===")
    
    # Create x86_64 executable
    print("Creating Linux x86_64 executable...")
    os.system(f"python3 -m PyInstaller \
    -y \
    --console \
    --onedir \
    --name JumperlessLinux_x64 \
    JumperlessWokwiBridge.py")
    
    time.sleep(2)
    
    # Create ARM64 executable (cross-compilation)
    print("Creating Linux ARM64 executable...")
    print("Note: Cross-compiling ARM64 from non-ARM64 systems may have limitations...")
    arm64_result = os.system(f"python3 -m PyInstaller \
    -y \
    --console \
    --onedir \
    --name JumperlessLinux_arm64 \
    --target-architecture arm64 \
    JumperlessWokwiBridge.py")
    
    time.sleep(2)
    
    # Check if ARM64 compilation succeeded
    linux_dist_path_x64 = pathlib.Path("dist/JumperlessLinux_x64")
    linux_dist_path_arm = pathlib.Path("dist/JumperlessLinux_arm64")
    linux_appdir_path_x64 = pathlib.Path("JumperlessLinux_x64.AppDir")
    linux_appdir_path_arm = pathlib.Path("JumperlessLinux_arm64.AppDir")
    appimage_path_x64 = pathlib.Path("Jumperless-x86_64.AppImage")
    appimage_path_arm = pathlib.Path("Jumperless-aarch64.AppImage")
    
    arm64_success = arm64_result == 0 and linux_dist_path_arm.exists()
    if not arm64_success:
        print("⚠️  ARM64 cross-compilation failed or not supported on this platform")
        print("   For best ARM64 results, run this packager on a native ARM64 Linux system")
    
    # Download AppImage tool
    appimagetool_available = download_appimagetool()
    if not appimagetool_available:
        print("⚠️  AppImage tools not available - will create tar.gz packages instead")
    
    # Determine which architectures to build
    architectures = [
        ("x86_64", linux_dist_path_x64, linux_appdir_path_x64, appimage_path_x64, "JumperlessLinux_x64")
    ]
    
    if arm64_success:
        architectures.append(
            ("ARM64", linux_dist_path_arm, linux_appdir_path_arm, appimage_path_arm, "JumperlessLinux_arm64")
        )
    
    success_count = 0
    
    for arch_name, dist_path, appdir_path, appimage_path, executable_name in architectures:
        print(f"\n--- Creating {arch_name} package ---")
        
        if create_appimage_for_arch(arch_name, dist_path, appdir_path, appimage_path, executable_name):
            # Try to create AppImage
            if appimagetool_available and os.path.exists("appimagetool-x86_64.AppImage"):
                print(f"Building {arch_name} AppImage with appimagetool...")
                result = os.system(f"./appimagetool-x86_64.AppImage {appdir_path} {appimage_path}")
                if result == 0:
                    print(f"✅ Created {arch_name} AppImage: {appimage_path}")
                    success_count += 1
                else:
                    print(f"AppImage creation failed for {arch_name}, creating tar.gz instead...")
                    tar_name = f"Jumperless-linux-{arch_name.lower()}.tar.gz"
                    os.system(f"tar -czf {tar_name} -C {appdir_path.parent} {appdir_path.name}")
                    print(f"✅ Created {arch_name} tar.gz package: {tar_name}")
                    success_count += 1
            else:
                print(f"Creating {arch_name} tar.gz package...")
                tar_name = f"Jumperless-linux-{arch_name.lower()}.tar.gz"
                os.system(f"tar -czf {tar_name} -C {appdir_path.parent} {appdir_path.name}")
                print(f"✅ Created {arch_name} tar.gz package: {tar_name}")
                success_count += 1
    
    print(f"\n🎉 Linux AppImage packaging complete! Created {success_count} architecture packages.")
    
    if success_count > 0:
        print("\n📦 Created packages:")
        for arch_name, _, _, appimage_path, _ in architectures:
            if os.path.exists(appimage_path):
                print(f"  • {arch_name}: {appimage_path} (AppImage)")
            elif os.path.exists(f"Jumperless-linux-{arch_name.lower()}.tar.gz"):
                print(f"  • {arch_name}: Jumperless-linux-{arch_name.lower()}.tar.gz (tar.gz)")
        
        print(f"\n💡 AppImage usage:")
        print(f"  1. Download the AppImage for your architecture")
        print(f"  2. Make executable: chmod +x Jumperless-*.AppImage")
        print(f"  3. Run: ./Jumperless-*.AppImage")
        print(f"  4. No installation required!")
        
        print(f"\n🖥️  Architecture notes:")
        print(f"  • x86_64: Works on most Linux PCs")
        print(f"  • ARM64: For Raspberry Pi 4+ and ARM servers")
    
    # Provide guidance for ARM64 if it wasn't created
    if not arm64_success:
        print(f"\n💡 To create ARM64 AppImages:")
        print(f"   • Run this packager on a Raspberry Pi or ARM64 Linux system")
        print(f"   • Or use a Linux ARM64 virtual machine/container")
        print(f"   • The x86_64 package works on most Linux systems")
    
    return success_count > 0

def package_windows():
    """Package for Windows as single executable"""
    print("=== Packaging for Windows ===")
    
    # Create Windows executable as single file
    print("Creating Windows x64 single executable...")
    print("Note: Cross-compiling Windows from macOS may have limitations...")
    
    # Use PyInstaller to create single Windows executable
    windows_result = os.system(f"python3 -m PyInstaller \
    -y \
    --onefile \
    --console \
    --name Jumperless \
    --distpath . \
    --icon icon.ico \
    JumperlessWokwiBridge.py")
    
    time.sleep(2)
    
    # Check if Windows compilation succeeded
    windows_exe = pathlib.Path("Jumperless.exe")
    windows_success = windows_result == 0 and windows_exe.exists()
    
    if not windows_success:
        print("⚠️  Windows cross-compilation failed or not supported on this platform")
        print("   For reliable Windows executables, use GitHub Actions or native Windows build")
        print("   See: https://github.com/yourusername/yourrepo/actions")
        return False
    
    # Show file info
    if windows_exe.exists():
        exe_size = windows_exe.stat().st_size / (1024*1024)
        print(f"✅ Created Windows executable: {windows_exe}")
        print(f"📊 Jumperless.exe size: {exe_size:.1f} MB")
    
    print("\n🎉 Windows packaging complete!")
    print(f"\nWindows users can:")
    print(f"  1. Download Jumperless.exe")
    print(f"  2. Run it directly - no installation needed!")
    print(f"  3. Portable - works from any folder")

def package_python_windows():
    """Package for Windows as simple zip archive with Python source"""
    print("=== Packaging Python Source for Windows ===")
    
    # Windows distribution folder
    windows_folder = pathlib.Path("Jumperless_Windows")
    zip_name = "Jumperless_Windows.zip"
    
    # Clean up existing folder
    if windows_folder.exists():
        shutil.rmtree(windows_folder)
    
    print("Creating universal Windows Python package...")
    
    try:
        # Create main folder
        windows_folder.mkdir(parents=True, exist_ok=True)
        
        # Copy main Python script
        if os.path.exists("JumperlessWokwiBridge.py"):
            shutil.copy2("JumperlessWokwiBridge.py", windows_folder)
            print(f"✅ Copied JumperlessWokwiBridge.py")
        else:
            print(f"❌ JumperlessWokwiBridge.py not found")
            return False
        
        # Copy requirements.txt
        if os.path.exists("requirements.txt"):
            shutil.copy2("requirements.txt", windows_folder)
            print(f"✅ Copied requirements.txt")
        else:
            print(f"⚠️  requirements.txt not found")
        
        # Create Windows batch launcher script with process killing
        windows_launcher_dest = windows_folder / "jumperless_launcher.bat"
        
        batch_launcher_content = """@echo off
setlocal enabledelayedexpansion

REM Jumperless Windows Launcher
REM Simple, stable launcher

echo.
echo ===============================================
echo         Jumperless Windows Launcher
echo ===============================================
echo.

REM Get the directory where this batch file is located
set "SCRIPT_DIR=%~dp0"

REM Detect current terminal environment for better user experience
set "TERMINAL_INFO=Unknown terminal"

if defined WT_SESSION (
    set "TERMINAL_INFO=Windows Terminal"
) else if defined TABBY_CONFIG_DIRECTORY (
    set "TERMINAL_INFO=Tabby Terminal"
) else if defined HYPER_VERSION (
    set "TERMINAL_INFO=Hyper Terminal"
) else if defined ConEmuPID (
    set "TERMINAL_INFO=ConEmu"
) else if defined ANSICON (
    set "TERMINAL_INFO=ANSICON-enabled terminal"
) else if defined TERM (
    set "TERMINAL_INFO=UNIX-style terminal (%TERM%)"
)

echo Running in: %TERMINAL_INFO%

REM Check if Python is available
python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [x] ERROR: Python is not installed or not in PATH
    echo [i] Please install Python 3.6+ from python.org and try again
    echo [i] Make sure to check "Add Python to PATH" during installation
    echo.
    pause
    exit /b 1
)

REM Check if pip is available
python -m pip --version >nul 2>&1
if %errorlevel% neq 0 (
    echo [x] ERROR: pip is not available
    echo [i] Please reinstall Python with pip included
    echo.
    pause
    exit /b 1
)

REM Install requirements if requirements.txt exists
if exist "%SCRIPT_DIR%requirements.txt" (
    echo [→] Installing Python dependencies...
    python -m pip install -r "%SCRIPT_DIR%requirements.txt"
    if %errorlevel% neq 0 (
        echo [!] Warning: Failed to install some dependencies
        echo [i] You may need to run: pip install -r requirements.txt
        echo [i] Or use a virtual environment
        echo.
        pause
    )
)

REM Run the main application
echo.
echo ===============================================
echo [→] Starting Jumperless Bridge...
echo ===============================================
echo.
cd /d "%SCRIPT_DIR%"
python JumperlessWokwiBridge.py %*

REM Handle exit
echo.
echo ===============================================
echo [i] Jumperless has exited
echo ===============================================
pause
"""
        
        with open(windows_launcher_dest, 'w', encoding='utf-8') as f:
            f.write(batch_launcher_content)
        print(f"✅ Created Windows launcher: jumperless_launcher.bat")
        
        # Create executable copy without .bat extension for double-clicking
        windows_launcher_executable = windows_folder / "jumperless_launcher"
        
        # For Windows, we need to create a .cmd file (which is executable) but without showing the extension
        windows_launcher_cmd = windows_folder / "jumperless_launcher.cmd"
        with open(windows_launcher_cmd, 'w', encoding='utf-8') as f:
            f.write(batch_launcher_content)
        print(f"✅ Created Windows executable launcher: jumperless_launcher.cmd")
        
        # Also create a Python wrapper script for better cross-platform compatibility
        python_wrapper_content = '''#!/usr/bin/env python3
"""
Jumperless Windows Python Launcher Wrapper
Cross-platform launcher that can be executed directly
"""
import os
import sys
import subprocess
import time
import signal

def main():
    """Main launcher function"""
    # Get script directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    print("\n" + "="*50)
    
    print("    Jumperless Windows Python Launcher")
    print("="*50 + "\n")
    
    # Check for Python
    try:
        subprocess.run([sys.executable, '--version'], check=True, capture_output=True)
        print(f"✅ Python found: {sys.executable}")
    except:
        print("❌ Python not working properly")
        input("Press Enter to exit...")
        return 1
    
    # Install requirements
    requirements_file = os.path.join(script_dir, 'requirements.txt')
    if os.path.exists(requirements_file):
        print("📦 Installing Python dependencies...")
        try:
            subprocess.run([sys.executable, '-m', 'pip', 'install', '-r', requirements_file], 
                          check=True)
            print("✅ Dependencies installed")
        except subprocess.CalledProcessError:
            print("⚠️  Warning: Some dependencies may not have installed")
    
    # Run main application
    print("\n🚀 Starting Jumperless Bridge...")
    print("="*50)
    
    main_script = os.path.join(script_dir, 'JumperlessWokwiBridge.py')
    if not os.path.exists(main_script):
        print(f"❌ Main script not found: {main_script}")
        input("Press Enter to exit...")
        return 1
    
    try:
        # Change to script directory and run
        os.chdir(script_dir)
        result = subprocess.run([sys.executable, 'JumperlessWokwiBridge.py'] + sys.argv[1:])
        return result.returncode
    except KeyboardInterrupt:
        print("\n\n⚠️  Interrupted by user")
        return 0
    except Exception as e:
        print(f"❌ Error running Jumperless: {e}")
        input("Press Enter to exit...")
        return 1

if __name__ == "__main__":
    sys.exit(main())
'''
        
        python_wrapper_dest = windows_folder / "jumperless_launcher.py"
        with open(python_wrapper_dest, 'w', encoding='utf-8') as f:
            f.write(python_wrapper_content)
        print(f"✅ Created Python wrapper launcher: jumperless_launcher.py")
        
        # Create a simple README
        readme_content = f"""# Jumperless Bridge for Windows (Python Source)

## Quick Start
1. Extract this archive: Right-click {zip_name} -> Extract All
2. Run the launcher: Double-click `jumperless_launcher.bat`

## Launcher Options
Multiple launcher options are provided for different use cases:

### Option 1: Batch Launchers (Recommended for most users)
- `jumperless_launcher.bat` - Full-featured batch launcher
- `jumperless_launcher.cmd` - Alternative batch launcher (same functionality)

### Option 2: Python Wrapper (For advanced users)
- `jumperless_launcher.py` - Cross-platform Python launcher

### Option 3: Manual Execution
- Run `python JumperlessWokwiBridge.py` directly after installing requirements

## Simple Launcher Features
All launchers provide:
- ✅ Automatic Python dependency installation
- ✅ Clear feedback about terminal environment
- ✅ Graceful error handling and user-friendly messages

## Requirements
- Python 3.6 or higher (from python.org)
- pip (included with Python)

## Manual Installation
If the launcher doesn't work, you can run manually:

1. Open Command Prompt in the extracted folder
2. Install dependencies:
   ```cmd
   pip install -r requirements.txt
   ```

3. Run the application:
   ```cmd
   python JumperlessWokwiBridge.py
   ```

## Files Included
- `JumperlessWokwiBridge.py` - Main application
- `requirements.txt` - Python dependencies
- `jumperless_launcher.bat` - Windows batch launcher
- `jumperless_launcher.cmd` - Alternative batch launcher
- `jumperless_launcher.py` - Python wrapper launcher
- `README.md` - This file

## Compatibility
This package works on all Windows versions that support Python 3.6+
(Windows 7, 8, 10, 11 - both 32-bit and 64-bit)

## Notes
- All launchers automatically install Python dependencies
- You may want to use a Python virtual environment
- Make sure Python is added to your PATH during installation
- Process killing ensures clean startup without conflicts

## Troubleshooting
- If "python is not recognized": Reinstall Python and check "Add to PATH"
- If dependencies fail to install: Try running as administrator
- For permission issues: Use `pip install --user -r requirements.txt`
- If launchers don't work: Try the Python wrapper or manual execution

## Support
Visit: https://github.com/Architeuthis-Flux/JumperlessV5
"""
        
        readme_path = windows_folder / "README.md"
        with open(readme_path, 'w', encoding='utf-8') as f:
            f.write(readme_content)
        print(f"✅ Created README.md")
        
        # Create the zip archive
        print(f"📦 Creating {zip_name}...")
        
        # Remove existing zip file if it exists
        if os.path.exists(zip_name):
            os.remove(zip_name)
        
        # Create zip using Python's zipfile module for cross-platform compatibility
        import zipfile
        with zipfile.ZipFile(zip_name, 'w', zipfile.ZIP_DEFLATED) as zipf:
            for file_path in windows_folder.rglob('*'):
                if file_path.is_file():
                    arcname = file_path.relative_to(windows_folder.parent)
                    zipf.write(file_path, arcname)
        
        if os.path.exists(zip_name):
            # Get file size
            zip_size = os.path.getsize(zip_name) / 1024  # Size in KB
            print(f"✅ Created Windows Python package: {zip_name} ({zip_size:.1f} KB)")
            
            print(f"\n🎉 Windows Python packaging complete!")
            print(f"\n📦 Created package: {zip_name}")
            print(f"📊 Package size: {zip_size:.1f} KB")
            
            print(f"\n💡 Usage instructions:")
            print(f"  1. Download {zip_name}")
            print(f"  2. Extract: Right-click -> Extract All")
            print(f"  3. Run: Double-click jumperless_launcher.bat")
            print(f"  4. The launcher will install dependencies automatically")
            
            print(f"\n📋 Manual installation (if launcher fails):")
            print(f"  1. pip install -r requirements.txt")
            print(f"  2. python JumperlessWokwiBridge.py")
            
            print(f"\n🖥️  Compatibility:")
            print(f"  • Works on Windows 7, 8, 10, 11 (32-bit and 64-bit)")
            print(f"  • Requires Python 3.6+ installed")
            
            # Clean up temporary folder
            try:
                if windows_folder.exists():
                    shutil.rmtree(windows_folder)
                print(f"\n🧹 Cleaned up temporary folder")
            except Exception as e:
                print(f"⚠️  Could not clean up temporary folder: {e}")
            
            return True
        else:
            print(f"❌ Failed to create {zip_name}")
            return False
            
    except Exception as e:
        print(f"❌ Error creating Windows Python package: {e}")
        return False

def package_python_macos():
    """Package for macOS as simple tar.gz archive with Python source"""
    print("=== Packaging Python Source for macOS ===")
    
    # macOS distribution folder
    macos_folder = pathlib.Path("Jumperless_macOS")
    tar_name = "Jumperless_macOS.tar.gz"
    
    # Clean up existing folder
    if macos_folder.exists():
        shutil.rmtree(macos_folder)
    
    print("Creating universal macOS Python package...")
    
    try:
        # Create main folder
        macos_folder.mkdir(parents=True, exist_ok=True)
        
        # Copy main Python script
        if os.path.exists("JumperlessWokwiBridge.py"):
            shutil.copy2("JumperlessWokwiBridge.py", macos_folder)
            print(f"✅ Copied JumperlessWokwiBridge.py")
        else:
            print(f"❌ JumperlessWokwiBridge.py not found")
            return False
        
        # Copy requirements.txt
        if os.path.exists("requirements.txt"):
            shutil.copy2("requirements.txt", macos_folder)
            print(f"✅ Copied requirements.txt")
        else:
            print(f"⚠️  requirements.txt not found")
        
        # Create macOS launcher script with comprehensive process killing
        macos_launcher_dest = macos_folder / "jumperless_launcher.sh"
        
        macos_launcher_content = """#!/bin/bash
# Jumperless macOS Launcher
# Simple, stable launcher with terminal resizing

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo ""
echo "==============================================="
echo "       Jumperless macOS Launcher"
echo "==============================================="
echo ""

# Check if Python 3 is available
if command -v python3 &> /dev/null; then
    PYTHON_CMD="python3"
elif command -v python &> /dev/null; then
    PYTHON_CMD="python"
else
    echo "❌ Error: Python 3 is not installed"
    echo ""
    echo "Please install Python 3:"
    echo "  • Download from python.org, or"
    echo "  • Install via Homebrew: brew install python3, or"
    echo "  • Use the system Python 3 (if available)"
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

echo "✅ Python found: $PYTHON_CMD"

# Check if pip is available
if ! $PYTHON_CMD -m pip --version &> /dev/null; then
    echo "❌ Error: pip is not available"
    echo "Please install pip or reinstall Python with pip included"
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

echo "✅ pip is available"

# Install requirements if requirements.txt exists
if [ -f "$SCRIPT_DIR/requirements.txt" ]; then
    echo "📦 Installing Python dependencies..."
    $PYTHON_CMD -m pip install -r "$SCRIPT_DIR/requirements.txt"
    if [ $? -ne 0 ]; then
        echo ""
        echo "⚠️  Warning: Failed to install some dependencies"
        echo "You may need to run: pip install -r requirements.txt"
        echo "Or use a virtual environment"
        echo ""
        read -p "Press Enter to continue anyway..."
    else
        echo "✅ Dependencies installed successfully"
    fi
fi

# Run the main application
echo ""
echo "==============================================="
echo "🚀 Starting Jumperless Bridge..."
echo "==============================================="
echo ""
cd "$SCRIPT_DIR"
exec $PYTHON_CMD JumperlessWokwiBridge.py "$@"
"""
        
        with open(macos_launcher_dest, 'w', encoding='utf-8') as f:
            f.write(macos_launcher_content)
        os.chmod(macos_launcher_dest, 0o755)  # Make executable
        print(f"✅ Created and made executable: jumperless_launcher.sh")
        
        # Create executable copy without .sh extension for double-clicking
        macos_launcher_executable = macos_folder / "jumperless_launcher"
        with open(macos_launcher_executable, 'w', encoding='utf-8') as f:
            f.write(macos_launcher_content)
        os.chmod(macos_launcher_executable, 0o755)  # Make executable
        print(f"✅ Created executable launcher (no extension): jumperless_launcher")
        
        # Create a Python wrapper for better cross-platform compatibility
        python_wrapper_content = '#!/usr/bin/env python3\n'
        python_wrapper_content += '"""\n'
        python_wrapper_content += 'Jumperless macOS Python Launcher Wrapper\n'
        python_wrapper_content += 'Cross-platform launcher that can be executed directly\n'
        python_wrapper_content += '"""\n'
        python_wrapper_content += 'import os\n'
        python_wrapper_content += 'import sys\n'
        python_wrapper_content += 'import subprocess\n\n'
        python_wrapper_content += 'def main():\n'
        python_wrapper_content += '    """Main launcher function"""\n'
        python_wrapper_content += '    # Get script directory\n'
        python_wrapper_content += '    script_dir = os.path.dirname(os.path.abspath(__file__))\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    print("\\\\n" + "="*50)\n'
        python_wrapper_content += '    print("    Jumperless macOS Python Launcher")\n'
        python_wrapper_content += '    print("="*50 + "\\\\n")\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    # Find Python\n'
        python_wrapper_content += '    python_cmd = None\n'
        python_wrapper_content += '    for cmd in [\'python3\', \'python\']:\n'
        python_wrapper_content += '        try:\n'
        python_wrapper_content += '            result = subprocess.run([cmd, \'--version\'], \n'
        python_wrapper_content += '                                  capture_output=True, check=True)\n'
        python_wrapper_content += '            python_cmd = cmd\n'
        python_wrapper_content += '            print(f"✅ Python found: {cmd}")\n'
        python_wrapper_content += '            break\n'
        python_wrapper_content += '        except Exception:\n'
        python_wrapper_content += '            continue\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    if not python_cmd:\n'
        python_wrapper_content += '        print("❌ Python 3 not found")\n'
        python_wrapper_content += '        input("\\\\nPress Enter to exit...")\n'
        python_wrapper_content += '        return 1\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    # Install requirements\n'
        python_wrapper_content += '    requirements_file = os.path.join(script_dir, \'requirements.txt\')\n'
        python_wrapper_content += '    if os.path.exists(requirements_file):\n'
        python_wrapper_content += '        print("📦 Installing Python dependencies...")\n'
        python_wrapper_content += '        try:\n'
        python_wrapper_content += '            subprocess.run([python_cmd, \'-m\', \'pip\', \'install\', \'-r\', requirements_file], \n'
        python_wrapper_content += '                          check=True)\n'
        python_wrapper_content += '            print("✅ Dependencies installed successfully")\n'
        python_wrapper_content += '        except subprocess.CalledProcessError:\n'
        python_wrapper_content += '            print("⚠️  Warning: Some dependencies may not have installed")\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    # Run main application\n'
        python_wrapper_content += '    print("\\\\n🚀 Starting Jumperless Bridge...")\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    main_script = os.path.join(script_dir, \'JumperlessWokwiBridge.py\')\n'
        python_wrapper_content += '    if not os.path.exists(main_script):\n'
        python_wrapper_content += '        print(f"❌ Main script not found: {main_script}")\n'
        python_wrapper_content += '        input("Press Enter to exit...")\n'
        python_wrapper_content += '        return 1\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    try:\n'
        python_wrapper_content += '        # Change to script directory and run\n'
        python_wrapper_content += '        os.chdir(script_dir)\n'
        python_wrapper_content += '        result = subprocess.run([python_cmd, \'JumperlessWokwiBridge.py\'] + sys.argv[1:])\n'
        python_wrapper_content += '        return result.returncode\n'
        python_wrapper_content += '    except KeyboardInterrupt:\n'
        python_wrapper_content += '        print("\\\\n\\\\n⚠️  Interrupted by user")\n'
        python_wrapper_content += '        return 0\n'
        python_wrapper_content += '    except Exception as e:\n'
        python_wrapper_content += '        print(f"❌ Error running Jumperless: {e}")\n'
        python_wrapper_content += '        input("Press Enter to exit...")\n'
        python_wrapper_content += '        return 1\n\n'
        python_wrapper_content += 'if __name__ == "__main__":\n'
        python_wrapper_content += '    sys.exit(main())\n'
        
        python_wrapper_dest = macos_folder / "jumperless_launcher.py"
        with open(python_wrapper_dest, 'w', encoding='utf-8') as f:
            f.write(python_wrapper_content)
        os.chmod(python_wrapper_dest, 0o755)  # Make executable
        print(f"✅ Created Python wrapper launcher: jumperless_launcher.py")
        
        # Create a simple README
        readme_content = f"""# Jumperless Bridge for macOS (Python Source)

## Quick Start
1. Extract this archive: `tar -xzf {tar_name}`
2. Run the launcher: `./jumperless_launcher.sh`

## Launcher Options
Multiple launcher options are provided for different use cases:

### Option 1: Shell Script Launchers (Recommended for most users)
- `jumperless_launcher.sh` - Full-featured shell launcher
- `jumperless_launcher` - Executable launcher (no extension) for double-clicking

### Option 2: Python Wrapper (For advanced users)
- `jumperless_launcher.py` - Cross-platform Python launcher

### Option 3: Manual Execution
- Run `python3 JumperlessWokwiBridge.py` directly after installing requirements

## Simple Launcher Features
All launchers provide:
- ✅ Automatic Python dependency installation
- ✅ Clear feedback about terminal environment
- ✅ Graceful error handling and user-friendly messages

## Requirements
- Python 3.6 or higher
- pip (Python package installer)

## Installation Options
### Option 1: Official Python
Download from python.org (recommended for beginners)

### Option 2: Homebrew
```bash
brew install python3
```

### Option 3: System Python (macOS 12.3+)
Recent macOS versions include Python 3

## Manual Installation
If the launcher doesn't work, you can run manually:

1. Install dependencies:
   ```bash
   pip3 install -r requirements.txt
   ```

2. Run the application:
   ```bash
   python3 JumperlessWokwiBridge.py
   ```

## Files Included
- `JumperlessWokwiBridge.py` - Main application
- `requirements.txt` - Python dependencies
- `jumperless_launcher.sh` - macOS shell launcher script
- `jumperless_launcher` - Executable launcher (no extension)
- `jumperless_launcher.py` - Python wrapper launcher
- `README.md` - This file

## Compatibility
This package works on:
- macOS 10.13+ (Intel and Apple Silicon)
- Any macOS version with Python 3.6+

## Notes
- All launchers automatically install Python dependencies
- You may want to use a Python virtual environment
- Compatible with both Intel and Apple Silicon Macs
- Process killing ensures clean startup without conflicts
- Launchers detect and close existing Terminal/iTerm2 windows

## Troubleshooting
- If Python is not found: Install from python.org or use Homebrew
- For permission issues: Try `pip3 install --user -r requirements.txt`
- On older macOS: You may need to install Command Line Tools
- If launchers don't work: Try the Python wrapper or manual execution
- For double-click issues: Use the `jumperless_launcher` file (no extension)

## Support
Visit: https://github.com/Architeuthis-Flux/JumperlessV5
"""
        
        readme_path = macos_folder / "README.md"
        with open(readme_path, 'w', encoding='utf-8') as f:
            f.write(readme_content)
        print(f"✅ Created README.md")
        
        # Create the tar.gz archive
        print(f"📦 Creating {tar_name}...")
        
        # Remove existing tar file if it exists
        if os.path.exists(tar_name):
            os.remove(tar_name)
        
        # Create tar.gz
        result = os.system(f"tar -czf {tar_name} {macos_folder.name}")
        
        if result == 0 and os.path.exists(tar_name):
            # Get file size
            tar_size = os.path.getsize(tar_name) / 1024  # Size in KB
            print(f"✅ Created macOS Python package: {tar_name} ({tar_size:.1f} KB)")
            
            print(f"\n🎉 macOS Python packaging complete!")
            print(f"\n📦 Created package: {tar_name}")
            print(f"📊 Package size: {tar_size:.1f} KB")
            
            print(f"\n💡 Usage instructions:")
            print(f"  1. Download {tar_name}")
            print(f"  2. Extract: tar -xzf {tar_name}")
            print(f"  3. Run: ./jumperless_launcher.sh")
            print(f"  4. The launcher will install dependencies automatically")
            
            print(f"\n📋 Manual installation (if launcher fails):")
            print(f"  1. pip3 install -r requirements.txt")
            print(f"  2. python3 JumperlessWokwiBridge.py")
            
            print(f"\n🖥️  Compatibility:")
            print(f"  • Works on Intel and Apple Silicon Macs")
            print(f"  • Supports macOS 10.13+ with Python 3.6+")
            
            # Clean up temporary folder
            try:
                if macos_folder.exists():
                    shutil.rmtree(macos_folder)
                print(f"\n🧹 Cleaned up temporary folder")
            except Exception as e:
                print(f"⚠️  Could not clean up temporary folder: {e}")
            
            return True
        else:
            print(f"❌ Failed to create {tar_name}")
            return False
            
    except Exception as e:
        print(f"❌ Error creating macOS Python package: {e}")
        return False

def package_linux():
    """Package for Linux as simple tar.gz archive with Python source"""
    print("=== Packaging Linux Python Source ===")
    
    # Linux distribution folder
    linux_folder = pathlib.Path("Jumperless_Linux")
    tar_name = "Jumperless_Linux.tar.gz"
    
    # Clean up existing folder
    if linux_folder.exists():
        shutil.rmtree(linux_folder)
    
    print("Creating universal Linux Python package...")
    
    try:
        # Create main folder
        linux_folder.mkdir(parents=True, exist_ok=True)
        
        # Copy main Python script
        if os.path.exists("JumperlessWokwiBridge.py"):
            shutil.copy2("JumperlessWokwiBridge.py", linux_folder)
            print(f"✅ Copied JumperlessWokwiBridge.py")
        else:
            print(f"❌ JumperlessWokwiBridge.py not found")
            return False
        
        # Copy requirements.txt
        if os.path.exists("requirements.txt"):
            shutil.copy2("requirements.txt", linux_folder)
            print(f"✅ Copied requirements.txt")
        else:
            print(f"⚠️  requirements.txt not found")
        
        # Copy and setup Linux launcher script with comprehensive process killing
        linux_launcher_source = "jumperless_launcher.sh"
        linux_launcher_dest = linux_folder / "jumperless_launcher.sh"
        
        if os.path.exists(linux_launcher_source):
            shutil.copy2(linux_launcher_source, linux_launcher_dest)
            os.chmod(linux_launcher_dest, 0o755)  # Make executable
            print(f"✅ Copied and made executable: jumperless_launcher.sh")
        else:
            print(f"⚠️  {linux_launcher_source} not found, creating comprehensive launcher")
            # Create a comprehensive launcher script with process killing
            comprehensive_launcher_content = """#!/bin/bash
# Jumperless Linux Launcher
# Simple, stable launcher with terminal resizing

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo ""
echo "==============================================="
echo "       Jumperless Linux Launcher"
echo "==============================================="
echo ""

# Check if Python 3 is available
if command -v python3 &> /dev/null; then
    PYTHON_CMD="python3"
elif command -v python &> /dev/null; then
    PYTHON_CMD="python"
else
    echo "❌ Error: Python 3 is not installed or not in PATH"
    echo "Please install Python 3 and try again"
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

echo "✅ Python found: $PYTHON_CMD"

# Check if pip is available
if ! $PYTHON_CMD -m pip --version &> /dev/null; then
    echo "❌ Error: pip is not available"
    echo "Please install pip and try again"
    echo ""
    read -p "Press Enter to exit..."
    exit 1
fi

echo "✅ pip is available"

# Install requirements if requirements.txt exists
if [ -f "$SCRIPT_DIR/requirements.txt" ]; then
    echo "📦 Installing Python dependencies..."
    $PYTHON_CMD -m pip install -r "$SCRIPT_DIR/requirements.txt"
    if [ $? -ne 0 ]; then
        echo ""
        echo "⚠️  Warning: Failed to install some dependencies"
        echo "You may need to run: pip install -r requirements.txt"
        echo "Or use a virtual environment"
        echo ""
        read -p "Press Enter to continue anyway..."
    else
        echo "✅ Dependencies installed successfully"
    fi
fi

# Run the main application
echo ""
echo "==============================================="
echo "🚀 Starting Jumperless Bridge..."
echo "==============================================="
echo ""
cd "$SCRIPT_DIR"
exec $PYTHON_CMD JumperlessWokwiBridge.py "$@"
"""
            with open(linux_launcher_dest, 'w', encoding='utf-8') as f:
                f.write(comprehensive_launcher_content)
            os.chmod(linux_launcher_dest, 0o755)
            print(f"✅ Created comprehensive launcher script")
        
        # Create executable copy without .sh extension for double-clicking
        linux_launcher_executable = linux_folder / "jumperless_launcher"
        if os.path.exists(linux_launcher_dest):
            shutil.copy2(linux_launcher_dest, linux_launcher_executable)
            os.chmod(linux_launcher_executable, 0o755)  # Make executable
            print(f"✅ Created executable launcher (no extension): jumperless_launcher")
        
        # Create a Python wrapper for better cross-platform compatibility
        python_wrapper_content = '#!/usr/bin/env python3\n'
        python_wrapper_content += '"""\n'
        python_wrapper_content += 'Jumperless Linux Python Launcher Wrapper\n'
        python_wrapper_content += 'Cross-platform launcher that can be executed directly\n'
        python_wrapper_content += '"""\n'
        python_wrapper_content += 'import os\n'
        python_wrapper_content += 'import sys\n'
        python_wrapper_content += 'import subprocess\n\n'
        python_wrapper_content += 'def main():\n'
        python_wrapper_content += '    """Main launcher function"""\n'
        python_wrapper_content += '    # Get script directory\n'
        python_wrapper_content += '    script_dir = os.path.dirname(os.path.abspath(__file__))\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    print("\\\\n" + "="*50)\n'
        python_wrapper_content += '    print("    Jumperless Linux Python Launcher")\n'
        python_wrapper_content += '    print("="*50 + "\\\\n")\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    # Find Python\n'
        python_wrapper_content += '    python_cmd = None\n'
        python_wrapper_content += '    for cmd in [\'python3\', \'python\']:\n'
        python_wrapper_content += '        try:\n'
        python_wrapper_content += '            result = subprocess.run([cmd, \'--version\'], \n'
        python_wrapper_content += '                                  capture_output=True, check=True)\n'
        python_wrapper_content += '            python_cmd = cmd\n'
        python_wrapper_content += '            print(f"✅ Python found: {cmd}")\n'
        python_wrapper_content += '            break\n'
        python_wrapper_content += '        except Exception:\n'
        python_wrapper_content += '            continue\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    if not python_cmd:\n'
        python_wrapper_content += '        print("❌ Python 3 not found")\n'
        python_wrapper_content += '        input("\\\\nPress Enter to exit...")\n'
        python_wrapper_content += '        return 1\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    # Install requirements\n'
        python_wrapper_content += '    requirements_file = os.path.join(script_dir, \'requirements.txt\')\n'
        python_wrapper_content += '    if os.path.exists(requirements_file):\n'
        python_wrapper_content += '        print("📦 Installing Python dependencies...")\n'
        python_wrapper_content += '        try:\n'
        python_wrapper_content += '            subprocess.run([python_cmd, \'-m\', \'pip\', \'install\', \'-r\', requirements_file], \n'
        python_wrapper_content += '                          check=True)\n'
        python_wrapper_content += '            print("✅ Dependencies installed successfully")\n'
        python_wrapper_content += '        except subprocess.CalledProcessError:\n'
        python_wrapper_content += '            print("⚠️  Warning: Some dependencies may not have installed")\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    # Run main application\n'
        python_wrapper_content += '    print("\\\\n🚀 Starting Jumperless Bridge...")\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    main_script = os.path.join(script_dir, \'JumperlessWokwiBridge.py\')\n'
        python_wrapper_content += '    if not os.path.exists(main_script):\n'
        python_wrapper_content += '        print(f"❌ Main script not found: {main_script}")\n'
        python_wrapper_content += '        input("Press Enter to exit...")\n'
        python_wrapper_content += '        return 1\n'
        python_wrapper_content += '    \n'
        python_wrapper_content += '    try:\n'
        python_wrapper_content += '        # Change to script directory and run\n'
        python_wrapper_content += '        os.chdir(script_dir)\n'
        python_wrapper_content += '        result = subprocess.run([python_cmd, \'JumperlessWokwiBridge.py\'] + sys.argv[1:])\n'
        python_wrapper_content += '        return result.returncode\n'
        python_wrapper_content += '    except KeyboardInterrupt:\n'
        python_wrapper_content += '        print("\\\\n\\\\n⚠️  Interrupted by user")\n'
        python_wrapper_content += '        return 0\n'
        python_wrapper_content += '    except Exception as e:\n'
        python_wrapper_content += '        print(f"❌ Error running Jumperless: {e}")\n'
        python_wrapper_content += '        input("Press Enter to exit...")\n'
        python_wrapper_content += '        return 1\n\n'
        python_wrapper_content += 'if __name__ == "__main__":\n'
        python_wrapper_content += '    sys.exit(main())\n'
        
        python_wrapper_dest = linux_folder / "jumperless_launcher.py"
        with open(python_wrapper_dest, 'w', encoding='utf-8') as f:
            f.write(python_wrapper_content)
        os.chmod(python_wrapper_dest, 0o755)  # Make executable
        print(f"✅ Created Python wrapper launcher: jumperless_launcher.py")
        
        # Create a simple README
        readme_content = f"""# Jumperless Bridge for Linux

## Quick Start
1. Extract this archive: `tar -xzf {tar_name}`
2. Run the launcher: `./jumperless_launcher.sh`

## Launcher Options
Multiple launcher options are provided for different use cases:

### Option 1: Shell Script Launchers (Recommended for most users)
- `jumperless_launcher.sh` - Full-featured shell launcher
- `jumperless_launcher` - Executable launcher (no extension) for double-clicking

### Option 2: Python Wrapper (For advanced users)
- `jumperless_launcher.py` - Cross-platform Python launcher

### Option 3: Manual Execution
- Run `python3 JumperlessWokwiBridge.py` directly after installing requirements

## Simple Launcher Features
All launchers provide:
- ✅ Automatic Python dependency installation
- ✅ Clear feedback about terminal environment
- ✅ Graceful error handling and user-friendly messages

## Requirements
- Python 3.6 or higher
- pip (Python package installer)

## Optional Tools for Enhanced Terminal Management
- `xdotool` - For X11 window management (recommended)
- `wmctrl` - Alternative window management tool
- Most Linux distributions include these or they can be installed via package manager

## Manual Installation
If the launcher doesn't work, you can run manually:

1. Install dependencies:
   ```bash
   pip install -r requirements.txt
   ```

2. Run the application:
   ```bash
   python3 JumperlessWokwiBridge.py
   ```

## Files Included
- `JumperlessWokwiBridge.py` - Main application
- `requirements.txt` - Python dependencies
- `jumperless_launcher.sh` - Linux shell launcher script
- `jumperless_launcher` - Executable launcher (no extension)
- `jumperless_launcher.py` - Python wrapper launcher
- `README.md` - This file

## Compatibility
This package works on all Linux architectures (x86_64, ARM64, etc.)
since it contains pure Python code.

## Notes
- All launchers automatically install Python dependencies
- You may want to use a Python virtual environment
- For system-wide installation, you may need sudo for pip install
- Process killing ensures clean startup without conflicts
- Terminal window management works best with xdotool installed

## Troubleshooting
- If Python is not found: Install `python3` via your package manager
- For permission issues: Try `pip install --user -r requirements.txt`
- If terminal windows don't close: Install `xdotool` or `wmctrl`
- If launchers don't work: Try the Python wrapper or manual execution
- For double-click issues: Use the `jumperless_launcher` file (no extension)
- On some distros: You may need to install `python3-pip` separately

## Support
Visit: https://github.com/Architeuthis-Flux/JumperlessV5
"""
        
        readme_path = linux_folder / "README.md"
        with open(readme_path, 'w', encoding='utf-8') as f:
            f.write(readme_content)
        print(f"✅ Created README.md")
        
        # Create the tar.gz archive
        print(f"📦 Creating {tar_name}...")
        
        # Remove existing tar file if it exists
        if os.path.exists(tar_name):
            os.remove(tar_name)
        
        # Create tar.gz
        result = os.system(f"tar -czf {tar_name} {linux_folder.name}")
        
        if result == 0 and os.path.exists(tar_name):
            # Get file size
            tar_size = os.path.getsize(tar_name) / 1024  # Size in KB
            print(f"✅ Created Linux Python package: {tar_name} ({tar_size:.1f} KB)")
            
            print(f"\n🎉 Linux Python packaging complete!")
            print(f"\n📦 Created package: {tar_name}")
            print(f"📊 Package size: {tar_size:.1f} KB")
            
            print(f"\n💡 Usage instructions:")
            print(f"  1. Download {tar_name}")
            print(f"  2. Extract: tar -xzf {tar_name}")
            print(f"  3. Run: ./jumperless_launcher.sh")
            print(f"  4. The launcher will install dependencies automatically")
            
            print(f"\n📋 Manual installation (if launcher fails):")
            print(f"  1. pip install -r requirements.txt")
            print(f"  2. python3 JumperlessWokwiBridge.py")
            
            print(f"\n🖥️  Architecture support:")
            print(f"  • Works on x86_64, ARM64, and other Linux architectures")
            print(f"  • Pure Python code - no compiled binaries")
            
            # Clean up temporary folder
            try:
                if linux_folder.exists():
                    shutil.rmtree(linux_folder)
                print(f"\n🧹 Cleaned up temporary folder")
            except Exception as e:
                print(f"⚠️  Could not clean up temporary folder: {e}")
            
            return True
        else:
            print(f"❌ Failed to create {tar_name}")
            return False
            
    except Exception as e:
        print(f"❌ Error creating Linux Python package: {e}")
        return False

def main():
    """Main packaging function"""
    # Check and update requirements first
    requirements_changed = check_and_update_requirements()
    
    if requirements_changed:
        print("\n🔄 Requirements updated - new dependencies will be included in app updates")
    
    print("\n" + "="*50)
    
    current_os = platform.system()
    
    if current_os == "Darwin":  # macOS
        print("🍎 Running on macOS - Multiple packaging options available:")
        print("\n1. Native macOS App (.dmg) - Recommended for end users")
        print("2. Python Source Package (.tar.gz) - For developers/advanced users")
        print("3. Both packages")
        print("4. Also create Linux and Windows Python packages")
        
        try:
            choice = input("\nChoose option (1-4): ").strip()
        except (EOFError, KeyboardInterrupt):
            choice = "1"
        
        if choice == "1":
            package_macos()
        elif choice == "2":
            package_python_macos()
        elif choice == "3":
            package_macos()
            print("\n" + "="*50)
            package_python_macos()
        elif choice == "4":
            package_macos()
            print("\n" + "="*50)
            package_python_macos()
            print("\n" + "="*50)
            package_linux()
            print("\n" + "="*50)
            package_python_windows()
        else:
            print("Invalid choice, creating native macOS app...")
            package_macos()
        
        print("\n🎉 macOS packaging complete!")
        
    elif current_os == "Linux":
        print("🐧 Running on Linux - Multiple packaging options available:")
        print("\n1. Python Source Package (.tar.gz) + AppImage (.AppImage) - Recommended")
        print("2. Native AppImage (.AppImage) only")
        print("3. Both packages (same as option 1)")
        
        try:
            choice = input("\nChoose option (1-3): ").strip()
        except (EOFError, KeyboardInterrupt):
            choice = "1"
        
        if choice == "1":
            package_linux()
            print("\n" + "="*50)
            package_linux_appimage()
        elif choice == "2":
            package_linux_appimage()
        elif choice == "3":
            package_linux()
            print("\n" + "="*50)
            package_linux_appimage()
        else:
            print("Invalid choice, creating both tar.gz and AppImage packages...")
            package_linux()
            print("\n" + "="*50)
            package_linux_appimage()
        
        print("\n🎉 Linux packaging complete!")
    
    elif current_os == "Windows":
        print("🪟 Running on Windows - Multiple packaging options available:")
        print("\n1. Native Windows Executable (.exe) - Recommended for end users")
        print("2. Python Source Package (.zip) - For developers/advanced users")
        print("3. Both packages")
        
        try:
            choice = input("\nChoose option (1-3): ").strip()
        except (EOFError, KeyboardInterrupt):
            choice = "1"
        
        if choice == "1":
            package_windows()
        elif choice == "2":
            package_python_windows()
        elif choice == "3":
            package_windows()
            print("\n" + "="*50)
            package_python_windows()
        else:
            print("Invalid choice, creating native Windows executable...")
            package_windows()
        
        print("\nWindows packaging complete!")
    
    else:
        print(f"Unsupported platform: {current_os}")
        print("This packager supports macOS, Linux, and Windows.")

if __name__ == "__main__":
    main() 