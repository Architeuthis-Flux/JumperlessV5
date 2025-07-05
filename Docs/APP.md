# App installation guide

## Find the latest release
https://github.com/Architeuthis-Flux/JumperlessV5/releases/latest

The link above will magically lead you to the latest version, and will look something like https://github.com/Architeuthis-Flux/JumperlessV5/releases/tag/5.1.4.1

## At the bottom under Assets, download the Jumperless App for your OS
- Windows
  - Jumperless.exe
  - Jumperless-Windows-x64.zip
- macOS Jumperless_Installer.dmg
- Linux
  - x86 Jumperless-linux-x86_64.tar.gz (if you're not sure which flavor of Linux, use this one)
  - arm64 Jumperless-linux-arm64.tar.gz
- Other, needs Python
  1. download `JumperlessWokwiBridge.py` and `requirements.txt`
  2. open your favorite terminal, navigate to the folder where you downloaded the two files above.
  3. `pip install -r requirements.txt` # run this command to install the needed Python libraries
  4. `python3 JumperlessWokwiBridge.py` # open the app, will update firmware if there's a newer version

## Straight up firmware update
If the steps above don't work for you, or you don't want to fiddle with Python:
- download the `firmware.uf2` file
- hold the boot button on the back of the Jumperless V5 USB port while plugging in the board
- drag / copy the `firmware.uf2` file to the drive that appears when

## old school Jumperless docs
If you want a picture with an arrow to the boot button, or you're interested in Jumperless history, check the docs for the OG Jumperless: https://hackaday.io/project/191238-jumperless/log/222858-getting-started-using-your-jumperless
