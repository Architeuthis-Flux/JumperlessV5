# -*- mode: python ; coding: utf-8 -*-


a = Analysis(
    ['JumperlessWokwiBridge.py'],
    pathex=['/Users/kevinsanto/Documents/GitHub/JumperlessV5/JumperlessWokwiBridge/.venv/lib/python3.13/site-packages'],
    binaries=[],
    datas=[],
    hiddenimports=[],
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
    name='Jumperless',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    console=False,
    disable_windowed_traceback=False,
    argv_emulation=False,
    target_arch='universal2',
    codesign_identity=None,
    entitlements_file=None,
    icon=['/Users/kevinsanto/Documents/GitHub/JumperlessV5/jumperlesswokwibridge/icon.icns'],
)
coll = COLLECT(
    exe,
    a.binaries,
    a.datas,
    strip=False,
    upx=True,
    upx_exclude=[],
    name='Jumperless',
)
app = BUNDLE(
    coll,
    name='Jumperless.app',
    icon='/Users/kevinsanto/Documents/GitHub/JumperlessV5/jumperlesswokwibridge/icon.icns',
    bundle_identifier=None,
)
