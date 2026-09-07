#!/usr/bin/env python3
"""Create a new, isolated Portal 2 map experiment; never deploy over Steam files."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import struct
import ast
import re

source = Path(__file__).resolve().parent.parent
common = Path.home() / 'Library/Application Support/Steam/steamapps/common'
p2 = Path(os.environ.get('PORTAL2_DIR', common / 'Portal 2')).resolve()
portal = Path(os.environ.get('PORTAL_DIR', common / 'Portal')).resolve()
build = Path(os.environ.get('BUILD_DIR', source / 'build')).resolve()
stage = Path(os.environ.get('PORTAL2_STAGE_DIR', p2 / 'portal2_arm64_test')).resolve()
misc = portal / 'hl2/hl2_misc_dir.vpk'
required = [p2 / 'portal2/pak01_dir.vpk', misc,
            portal / 'hl2/hl2_textures_dir.vpk', portal / 'portal/portal_pak_dir.vpk',
            build / 'launcher_main/hl2_launcher']
for path in required:
    if not path.is_file():
        sys.exit(f'Missing prerequisite: {path}')
if stage.exists():
    sys.exit(f'Refusing to overwrite existing staging: {stage}')
for module in ('client', 'server'):
    cache = build / f'c4che/game/{module}_cache.py'
    match = re.search(r'^GAMES\s*=\s*(.+)$', cache.read_text(), re.M) if cache.is_file() else None
    if not match or ast.literal_eval(match.group(1)) != 'portal2':
        sys.exit(f'Build is not configured for portal2 ({module}); run scripts/build-macos-arm64.sh portal2')
try:
    import vpk
except ImportError:
    sys.exit('Python dependency missing: install vpk in your Python environment.')
# Fail before creating the stage if the shader ABI is incompatible.
header = struct.unpack('<7I', vpk.open(str(misc))['shaders/fxc/lightmappedgeneric_ps20b.vcs'].read()[:28])
if header[0] != 6 or header[2] != 288:
    sys.exit('Expected Anniversary renderer cache (VCS 6, 288 dynamic combos).')

for folder in ('bin', 'portal2/cfg', 'portal2/save', 'portal2_override/cfg', 'renderer_compat'):
    (stage / folder).mkdir(parents=True, exist_ok=True)
shutil.copy2(build / 'launcher_main/hl2_launcher', stage / 'hl2_osx')
for lib in build.rglob('*.dylib'):
    target = stage / 'bin' / lib.name
    if target.is_symlink():
        sys.exit(f'Duplicate library name in build: {lib.name}')
    target.symlink_to(lib)
(stage / 'portal2_content').symlink_to(p2 / 'portal2', target_is_directory=True)
(stage / 'platform').symlink_to(p2 / 'platform', target_is_directory=True)
# Never symlink cfg/save to Steam: the engine writes configuration and saves.
for cfg in (p2 / 'portal2/cfg').glob('*.cfg'):
    if cfg.name.lower() in ('config.cfg', 'autoexec.cfg', 'video.cfg', 'videoconfig.cfg'):
        continue
    shutil.copy2(cfg, stage / 'portal2/cfg' / cfg.name)

paths = [('Game', '|gameinfo_path|../renderer_compat'),
         ('Game', '|gameinfo_path|../portal2_material_compat'),
         ('Game', '|gameinfo_path|../portal2_override'),
         ('Game', '|gameinfo_path|.')]
# VPKs must be explicit: this filesystem does not auto-mount pak01_dir.vpk.
for dlc in ('portal2_dlc2', 'portal2_dlc1'):
    if (p2 / dlc / 'pak01_dir.vpk').is_file():
        paths.append(('Game', str(p2 / dlc / 'pak01_dir.vpk')))
paths += [('Game', '|gameinfo_path|../portal2_content/pak01_dir.vpk'),
          ('Game', '|gameinfo_path|../portal2_content'),
          ('Game', str(portal / 'portal/portal_pak_dir.vpk')),
          ('Game', str(misc)), ('Game', str(portal / 'hl2/hl2_textures_dir.vpk')),
          ('Game', '|gameinfo_path|../platform'),
          ('Platform', '|gameinfo_path|../platform'),
          ('Mod', '|gameinfo_path|../portal2_override'),
          ('Default_Write_Path', '|gameinfo_path|../portal2_override')]
if any('"' in path or '\n' in path for _, path in paths):
    sys.exit('Unsupported quote/newline in install path')
search = '\n'.join(f'            {kind} "{path}"' for kind, path in paths)
(stage / 'portal2/gameinfo.txt').write_text('''"GameInfo"
{
    game "PORTAL 2 ARM64 Experimental"
    title "PORTAL 2 ARM64 Experimental"
    type singleplayer_only
    FileSystem
    {
        SteamAppId 620
        SearchPaths
        {
''' + search + '''
        }
    }
}
''')
(stage / 'steam_appid.txt').write_text('620\n')
subprocess.run([sys.executable, str(source / 'scripts/prepare-portal2-shaders.py'),
                str(stage), str(misc)], check=True)
subprocess.run([sys.executable, str(source / 'scripts/prepare-portal2-material-compat.py'),
                str(stage), str(p2)], check=True)
(stage / 'Jugar-Portal2-Experimental.command').write_text('''#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
exec ./hl2_osx -game portal2 -novid -windowed -w 1024 -h 768 -console \\
  +sv_cheats 1 +mat_fullbright 0 +mat_disable_bloom 1 +mat_colorcorrection 0 \\
  +map "${1:-sp_a1_intro1}"
''')
(stage / 'Jugar-Portal2-Experimental.command').chmod(0o755)
print(f'Staged isolated map experiment: {stage}')
print('Run Jugar-Portal2-Experimental.command; this is not the full Portal 2 game.')
