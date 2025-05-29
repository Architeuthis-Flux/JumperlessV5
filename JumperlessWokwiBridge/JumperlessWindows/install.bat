@echo off
echo Installing Jumperless...
echo.
echo This will copy Jumperless.exe to your Programs folder.
echo Press any key to continue or close this window to cancel.
pause >nul

set "INSTALL_DIR=%ProgramFiles%\Jumperless"

echo Creating installation directory...
mkdir "%INSTALL_DIR%" 2>nul

echo Copying executable...
copy /Y "Jumperless.exe" "%INSTALL_DIR%\"
copy /Y "Jumperless_launcher.bat" "%INSTALL_DIR%\" 2>nul
copy /Y "requirements.txt" "%INSTALL_DIR%\" 2>nul
copy /Y "README.md" "%INSTALL_DIR%\" 2>nul

echo Creating desktop shortcut...
set "DESKTOP=%USERPROFILE%\Desktop"
echo @echo off > "%DESKTOP%\Jumperless.bat"
echo cd /d "%INSTALL_DIR%" >> "%DESKTOP%\Jumperless.bat"
echo start "Jumperless" "Jumperless.exe" >> "%DESKTOP%\Jumperless.bat"

echo Adding to PATH (optional - may require restart)...
setx PATH "%PATH%;%INSTALL_DIR%" >nul 2>&1

echo.
echo Installation complete!
echo You can now run Jumperless from:
echo - Desktop shortcut: Jumperless.bat
echo - Command line: Jumperless.exe
echo - Start menu or directly from: %INSTALL_DIR%
echo.
pause
