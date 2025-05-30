# -*- mode: python ; coding: utf-8 -*-

block_cipher = None

# Exclude Windows-specific modules
excluded_modules = [
    'win32api', 'win32gui', 'win32con', 'win32file', 'win32pipe',
    'win32process', 'win32security', 'win32service', 'pywintypes',
    'pythoncom', 'win32clipboard', 'win32event', 'win32evtlog'
]

a = Analysis(
    ['JumperlessWokwiBridge.py'],
    pathex=[],
    binaries=[],
    datas=[],
    hiddenimports=[],
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=excluded_modules,
    win_no_prefer_redirects=False,
    win_private_assemblies=False,
    cipher=block_cipher,
    noarchive=False,
)

pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    [],
    exclude_binaries=True,
    name='JumperlessLinux',
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
)

coll = COLLECT(
    exe,
    a.binaries,
    a.zipfiles,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='JumperlessLinux',
) 