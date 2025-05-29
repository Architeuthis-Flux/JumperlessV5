import PyInstaller.__main__

import pathlib
import os
import platform
import shutil
import time


# PyInstaller.__main__.run([
#     'JumperlessWokwiBridge.py',
#     '--onefile',
#     '--windowed',
#     '--onedir',
#     '--icon=icon.icns',
#     #'--add-binary=arduino-cli:.',
#     '--console',
#     '--target-arch=arm64',
#     '--path=/Users/kevinsanto/Documents/GitHub/Jumperless/Jumperless_Wokwi_Bridge_App/JumperlessWokwiBridge/.venv/lib/python3.12/site-packages',
#     '--noconfirm',
#     #'--clean',

    
# ])

# os.system(f"python -m PyInstaller --icon=\"/Users/kevinsanto/Documents/GitHub/JumperlessV5/jumperlesswokwibridge/icon.icns\" \
# -y \
# --console \
# --windowed \
# --target-arch universal2 \
# --path \"/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/.venv/lib/python3.12/site-packages\" \
# JumperlessWokwiBridge.py \
# --name Jumperless")
# time.sleep(4)
generated_app_path = pathlib.Path("/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/dist/Jumperless.app/Contents/MacOS/Jumperless")
generated_app_path_renamed = pathlib.Path("/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/dist/Jumperless.app/Contents/MacOS/Jumperless_cli")
target_app_path = pathlib.Path   ("/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/dist/Jumperless.app/Contents/MacOS/")
icon_path = pathlib.Path("icon.icns")
nonexec_launcher_path = pathlib.Path("jumperless_cli_launcher.sh")
launcher_path = pathlib.Path("Jumperless")
app_path = pathlib.Path("Jumperless.app")
apple_silicon_folder = pathlib.Path("apple silicon/Jumperless.app")
intel_folder = pathlib.Path("intel mac/Jumperless.app")

# Linux packaging paths
linux_dist_path = pathlib.Path("dist/JumperlessLinux")
linux_appdir_path = pathlib.Path("JumperlessLinux.AppDir")
appimage_path = pathlib.Path("Jumperless-x86_64.AppImage")

appdist_path = pathlib.Path("dist/Jumperless.app")
dmg_folder = pathlib.Path("JumperlessDMG/Jumperless.app")
python_folder = pathlib.Path("Jumperless\ Python/")

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

    # rename the app to JumperlessWokwiBridge_cli
    print("chmodding launcher")
    os.system(f"chmod 755 {nonexec_launcher_path}")
    print("Changed permissions for " + str(nonexec_launcher_path)+ '\n')

    print("Renaming launcher to Jumperless")
    os.system(f'cp {nonexec_launcher_path} {launcher_path}')
    print("Renamed " + str(nonexec_launcher_path) + " to " + str(launcher_path)+ '\n')

    print("Renaming app main app to Jumperless_cli")
    os.rename(generated_app_path, generated_app_path_renamed)
    print("Renamed " + str(generated_app_path) + "(main app) to " + str(generated_app_path_renamed)+ '\n')

    os.system(f"cp {launcher_path} {target_app_path}")
    print("Copied " + str(launcher_path) + "(launcher) to " + str(target_app_path)+ '\n')

    shutil.copytree(appdist_path, dmg_folder, dirs_exist_ok=True )
    print("Copied " + str(app_path) + " to " + str(dmg_folder)+ '\n')

    os.system(f"cp JumperlessWokwiBridge.py {python_folder}")
    os.system(f"cp requirements.txt {python_folder}")
    print("Copied " + "JumperlessWokwiBridge.py" + " to " + str(python_folder)+ '\n')

    os.system(f"rm -f Jumperless_Installer.dmg")
    os.chmod("createDMG.sh", 0o755)
    os.system("./createDMG.sh")

def package_linux():
    """Package for Linux as AppImage"""
    print("=== Packaging for Linux ===")
    
    # Create Linux executable with PyInstaller
    print("Creating Linux executable...")
    os.system(f"python -m PyInstaller \
    -y \
    --console \
    --onedir \
    --name JumperlessLinux \
    JumperlessWokwiBridge.py")
    
    time.sleep(2)
    
    # Create AppDir structure
    print("Creating AppImage structure...")
    if linux_appdir_path.exists():
        shutil.rmtree(linux_appdir_path)
    
    linux_appdir_path.mkdir()
    
    # Copy executable and dependencies
    if linux_dist_path.exists():
        shutil.copytree(linux_dist_path, linux_appdir_path / "usr" / "bin", dirs_exist_ok=True)
    
    # Create desktop entry
    desktop_content = """[Desktop Entry]
Type=Application
Name=Jumperless
Exec=AppRun
Icon=jumperless
Comment=Jumperless Wokwi Bridge
Categories=Development;Electronics;
Terminal=true
"""
    
    with open(linux_appdir_path / "jumperless.desktop", "w") as f:
        f.write(desktop_content)
    
    # Create AppRun script
    apprun_content = f"""#!/bin/bash
# AppRun script for Jumperless

# Get the directory where this AppImage is located
HERE="$(dirname "$(readlink -f "${{BASH_SOURCE[0]}}")")"

# Make launcher script executable
chmod +x "$HERE/jumperless_cli_launcher.sh"

# Run the launcher script
exec "$HERE/jumperless_cli_launcher.sh" "$@"
"""
    
    apprun_path = linux_appdir_path / "AppRun"
    with open(apprun_path, "w") as f:
        f.write(apprun_content)
    
    os.chmod(apprun_path, 0o755)
    
    # Copy launcher script
    shutil.copy2(nonexec_launcher_path, linux_appdir_path / "jumperless_cli_launcher.sh")
    os.chmod(linux_appdir_path / "jumperless_cli_launcher.sh", 0o755)
    
    # Copy icon (convert to PNG if needed)
    if os.path.exists("icon.png"):
        shutil.copy2("icon.png", linux_appdir_path / "jumperless.png")
    elif os.path.exists("icon.icns"):
        # Try to convert icns to png using sips (macOS) or ImageMagick
        try:
            os.system(f"sips -s format png icon.icns --out {linux_appdir_path}/jumperless.png")
        except:
            try:
                os.system(f"convert icon.icns {linux_appdir_path}/jumperless.png")
            except:
                print("Warning: Could not convert icon. Creating placeholder.")
                # Create a simple placeholder icon
                with open(linux_appdir_path / "jumperless.png", "w") as f:
                    f.write("")  # Placeholder
    
    # Download and use appimagetool if available
    print("Creating AppImage...")
    if not os.path.exists("appimagetool-x86_64.AppImage"):
        print("Downloading appimagetool...")
        os.system("wget -q https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage")
        os.chmod("appimagetool-x86_64.AppImage", 0o755)
    
    # Create the AppImage
    if os.path.exists("appimagetool-x86_64.AppImage"):
        os.system(f"./appimagetool-x86_64.AppImage {linux_appdir_path} {appimage_path}")
        print(f"Created AppImage: {appimage_path}")
    else:
        print("Warning: appimagetool not available. Creating tar.gz instead...")
        os.system(f"tar -czf Jumperless-linux-x86_64.tar.gz -C {linux_appdir_path.parent} {linux_appdir_path.name}")
        print("Created tar.gz package: Jumperless-linux-x86_64.tar.gz")

def main():
    """Main packaging function"""
    current_os = platform.system()
    
    if current_os == "Darwin":  # macOS
        package_macos()
        print("\n" + "="*50)
        print("macOS packaging complete!")
        
        # Ask if user wants to also create Linux package
        response = input("Do you want to also create a Linux package? (y/n): ").lower().strip()
        if response in ['y', 'yes']:
            package_linux()
            print("\nLinux packaging complete!")
    
    elif current_os == "Linux":
        package_linux()
        print("Linux packaging complete!")
    
    else:
        print(f"Unsupported platform: {current_os}")
        print("This packager supports macOS and Linux only.")

if __name__ == "__main__":
    main()









# os.system(f"cp {"JumperlessWokwiBridge.app"} {App}")

# os.system(f"cp {icon_path} {generated_app_path}")

# print("Done")


# rename the app to JumperlessWokwiBridge_cli
print("chmodding launcher")
os.system(f"chmod 755 {nonexec_launcher_path}")
print("Changed permissions for " + str(nonexec_launcher_path)+ '\n')

print("Renaming launcher to Jumperless")
os.system(f'cp {nonexec_launcher_path} {launcher_path}')
print("Renamed " + str(nonexec_launcher_path) + " to " + str(launcher_path)+ '\n')


print("Renaming app main app to Jumperless_cli")
os.rename(generated_app_path, generated_app_path_renamed)
print("Renamed " + str(generated_app_path) + "(main app) to " + str(generated_app_path_renamed)+ '\n')


os.system(f"cp {launcher_path} {target_app_path}")
print("Copied " + str(launcher_path) + "(launcher) to " + str(target_app_path)+ '\n')

shutil.copytree(appdist_path, dmg_folder, dirs_exist_ok=True )
# os.system(f"cp {appdist_path} {dmg_folder}")
print("Copied " + str(app_path) + " to " + str(dmg_folder)+ '\n')


# //shutil.copytree(appdist_path, python_folder, dirs_exist_ok=True )

os.system(f"cp JumperlessWokwiBridge.py {python_folder}")
os.system(f"cp requirements.txt {python_folder}")
print("Copied " + "JumperlessWokwiBridge.py" + " to " + str(python_folder)+ '\n')

os.system(f"rm Jumperless_Installer.dmg")



os.chmod("createDMG.sh", 0o755)

os.system("./createDMG.sh")
# os.system(f"cp {generated_app_path} {target_app_path}")
# print("Copied " + str(generated_app_path) + " to " + str(target_app_path))

# App = "/Applications/"
# print("Copying app to applications folder")
# shutil.copytree("Jumperless.app", App + "Jumperless.app", dirs_exist_ok=True )
# print("Copied " + str(app_path) + " to " + str(App)+ '\n')

# print("Copying app to apple silicon folder")
# shutil.copytree("Jumperless.app", apple_silicon_folder, dirs_exist_ok=True )

# print("doing this all again for intel mac")




# python -m PyInstaller \
# --icon="/Users/kevinsanto/Documents/GitHub/Jumperless/Jumperless_Wokwi_Bridge_App/jumperlesswokwibridge/icon.icns" \
# -y \
# --console \
# --windowed \
# --target-arch x86_64 \
# --path "/Users/kevinsanto/Documents/GitHub/Jumperless/Jumperless_Wokwi_Bridge_App/JumperlessWokwiBridge/.venv/lib/python3.12/site-packages" \
# --add-binary "arduino-cli:." \
# JumperlessWokwiBridge.py 