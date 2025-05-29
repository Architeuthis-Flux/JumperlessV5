import PyInstaller.__main__
import pathlib
import os
import platform
import shutil
import time

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

def package_macos():
    """Package for macOS"""
    print("=== Packaging for macOS ===")
    
    os.system(f"python -m PyInstaller --icon=\"/Users/kevinsanto/Documents/GitHub/JumperlessV5/jumperlesswokwibridge/icon.icns\" \
    -y \
    --console \
    --windowed \
    --target-arch universal2 \
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

    print("Copying Python files...")
    os.system(f"cp JumperlessWokwiBridge.py {python_folder}")
    os.system(f"cp requirements.txt {python_folder}")
    print("Copied Python files to " + str(python_folder) + '\n')

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
        shutil.copytree(dist_path, appdir_path / "usr" / "bin", dirs_exist_ok=True)
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
    
    with open(appdir_path / "jumperless.desktop", "w") as f:
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
    with open(apprun_path, "w") as f:
        f.write(apprun_content)
    os.chmod(apprun_path, 0o755)
    
    # Copy launcher script
    shutil.copy2(nonexec_launcher_path, appdir_path / "jumperless_cli_launcher.sh")
    os.chmod(appdir_path / "jumperless_cli_launcher.sh", 0o755)
    
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
                with open(appdir_path / "jumperless.png", "w") as f:
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

def package_linux():
    """Package for Linux as AppImage (both x86_64 and ARM64)"""
    print("=== Packaging for Linux ===")
    
    # Create x86_64 executable
    print("Creating Linux x86_64 executable...")
    os.system(f"python -m PyInstaller \
    -y \
    --console \
    --onedir \
    --name JumperlessLinux_x64 \
    JumperlessWokwiBridge.py")
    
    time.sleep(2)
    
    # Create ARM64 executable (cross-compilation)
    print("Creating Linux ARM64 executable...")
    print("Note: Cross-compiling ARM64 from macOS may have limitations...")
    arm64_result = os.system(f"python -m PyInstaller \
    -y \
    --console \
    --onedir \
    --name JumperlessLinux_arm64 \
    --target-architecture arm64 \
    JumperlessWokwiBridge.py")
    
    time.sleep(2)
    
    # Check if ARM64 compilation succeeded
    arm64_success = arm64_result == 0 and linux_dist_path_arm.exists()
    if not arm64_success:
        print("⚠️  ARM64 cross-compilation failed or not supported on this platform")
        print("   For best ARM64 results, run this packager on a native ARM64 Linux system")
    
    # Download AppImage tool (but note it might not work on all platforms)
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
    
    print(f"\n🎉 Linux packaging complete! Created {success_count} architecture packages.")
    if success_count > 0:
        print("\nCreated packages for:")
        for arch_name, _, _, appimage_path, _ in architectures:
            if os.path.exists(appimage_path):
                print(f"  📦 {arch_name}: {appimage_path}")
            elif os.path.exists(f"Jumperless-linux-{arch_name.lower()}.tar.gz"):
                print(f"  📦 {arch_name}: Jumperless-linux-{arch_name.lower()}.tar.gz")
    
    # Provide guidance for ARM64 if it wasn't created
    if not arm64_success:
        print(f"\n💡 To create ARM64 Linux packages:")
        print(f"   • Run this packager on a Raspberry Pi or ARM64 Linux system")
        print(f"   • Or use a Linux ARM64 virtual machine/container")
        print(f"   • The x86_64 package works on most Linux systems")
    
    # Explain tar.gz vs AppImage
    if success_count > 0 and not appimagetool_available:
        print(f"\n📝 Note: Created tar.gz packages instead of AppImage due to platform limitations")
        print(f"   Users can extract and run: tar -xzf package.tar.gz && cd directory && ./AppRun")

def package_windows():
    """Package for Windows as single executable"""
    print("=== Packaging for Windows ===")
    
    # Create Windows executable as single file
    print("Creating Windows x64 single executable...")
    print("Note: Cross-compiling Windows from macOS may have limitations...")
    
    # Use PyInstaller to create single Windows executable
    windows_result = os.system(f"python -m PyInstaller \
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
        return
    
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

def main():
    """Main packaging function"""
    current_os = platform.system()
    
    if current_os == "Darwin":  # macOS
        package_macos()
        print("\n" + "="*50)
        print("macOS packaging complete!")
        
        # Ask if user wants to also create Linux package
        response = "y"
        print("Do you want to also create a Linux package? (y/n): ")

        if response in ['y', 'yes']:
            package_linux()
            print("\nLinux packaging complete!")
        
        # Ask if user wants to also create Windows package  
        response_windows = "y"
        print("Do you want to also create a Windows package? (y/n): ")
        
        if response_windows in ['y', 'yes']:
            package_windows()
            print("\nWindows packaging complete!")
    
    elif current_os == "Linux":
        package_linux()
        print("Linux packaging complete!")
    
    elif current_os == "Windows":
        package_windows()
        print("Windows packaging complete!")
    
    else:
        print(f"Unsupported platform: {current_os}")
        print("This packager supports macOS, Linux, and Windows.")

if __name__ == "__main__":
    main() 