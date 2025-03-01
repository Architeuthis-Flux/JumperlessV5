#!/usr/bin/env bash

create-dmg \
--volname "Jumperless Installer" \
--volicon "icon.icns" \
--background "JumperlessWokwiDMGwindow4x.png" \
--window-pos 240 240 \
--window-size 580 590 \
--icon-size 100 \
--icon "Jumperless.app" 72 245 \
--app-drop-link 395 245 \
--hide-extension "Jumperless.app" \
--codesign "Kevin Cappuccio (LK2RWK9EUK)" \
--add-folder "Jumperless Python" "Jumperless Python" 69 460 \
"Jumperless_Installer.dmg" \
"JumperlessDMG/" 