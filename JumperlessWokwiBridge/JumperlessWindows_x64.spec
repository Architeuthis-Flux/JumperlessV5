# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['JumperlessWokwiBridge.py'],
    pathex=[],
    binaries=[],
    datas=[],
    hiddenimports=['serial', 'serial.tools.list_ports', 'psutil', 'requests', 'colorama', 'beautifulsoup4', 'bs4'],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
    optimize=0,
)
pyz = PYZ(a.pure)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='JumperlessWindows_x64',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=True,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch=None,
    codesign_identity=None,
    entitlements_file=None,
    icon=['icon.ico'],
)
coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='JumperlessWindows_x64',
)
