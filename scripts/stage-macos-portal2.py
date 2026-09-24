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
			portal / 'portal/maps/background1.bsp',
			portal / 'portal/resource/gamemenu.res', portal / 'hl2/resource/clientscheme.res',
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
         ('Game', '|gameinfo_path|../portal2_gameplay_compat'),
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
subprocess.run([sys.executable, str(source / 'scripts/prepare-portal2-gameplay.py'),
                str(stage), str(portal), str(p2)], check=True)
subprocess.run([sys.executable, str(source / 'scripts/prepare-portal2-sound-manifest.py'),
                str(p2 / 'portal2'), str(stage / 'portal2_gameplay_compat')], check=True)
subprocess.run([sys.executable, str(source / 'scripts/prepare-portal2-intro-script.py'),
                '--game-dir', str(p2 / 'portal2'), '--output',
                str(stage / 'portal2_gameplay_compat/scripts/portal2_intro1_scenes.txt')], check=True)
subprocess.run([sys.executable, str(source / 'scripts/prepare-portal2-scenes.py'),
                str(p2 / 'portal2/scenes/scenes.image'),
                str(stage / 'portal2_gameplay_compat/scenes/scenes.image')], check=True)
subprocess.run([sys.executable, str(source / 'scripts/prepare-portal2-intro-audio.py'),
                str(p2 / 'portal2'), str(stage / 'portal2_gameplay_compat')], check=True)
subprocess.run([sys.executable, str(source / 'scripts/prepare-portal2-ui-localization.py'),
                str(stage / 'portal2_gameplay_compat')], check=True)
# Portal 2's GameUI resources target a different UI stack.  Use Portal 1's
# known-good menu definition and VGUI scheme in the isolated override.  The
# original game files remain read-only and older build targets are untouched.
p1_resource = portal / 'portal/resource'
p1_scheme = portal / 'hl2/resource/clientscheme.res'
menu_resource = stage / 'portal2_override/resource'
menu_resource.mkdir(parents=True, exist_ok=True)

# Portal 2's current Steam depot references background_menu from
# ChapterBackgrounds.txt but does not ship that BSP.  A failed startup
# background keeps GameUI hidden on this older engine.  Use Portal 1's known
# compatible animated background map as the bootstrap scene; all files stay in
# the isolated override and the Portal 2 install remains untouched.
menu_maps = stage / 'portal2_override/maps'
menu_scripts = stage / 'portal2_override/scripts'
menu_maps.mkdir(parents=True, exist_ok=True)
menu_scripts.mkdir(parents=True, exist_ok=True)
shutil.copy2(portal / 'portal/maps/background1.bsp', menu_maps / 'background_menu.bsp')
(menu_scripts / 'vscripts').mkdir(parents=True, exist_ok=True)
for test_script in (source / 'game/server/portal2/tests').glob('*.nut'):
    shutil.copy2(test_script, menu_scripts / 'vscripts' / test_script.name)
(menu_scripts / 'chapterbackgrounds.txt').write_text('''"chapters"
{
    1 "background_menu"
}
"BackgroundMaps"
{
    1 "background_menu"
}
''')
# Keep the classic GameUI surface deliberately small and source-faithful to
# what this ARM64 Portal 2 target can execute today.  In particular, do not
# expose Portal 1's commentary or an unverified Portal 2 co-op flow.  Starting
# a new game goes straight to the first supported Portal 2 map instead of the
# Portal 1 chapter picker.
(menu_resource / 'gamemenu.res').write_text('''"GameMenu"
{
    "1"
    {
        "label" "#GameUI_GameMenu_NewGame"
        "command" "engine map sp_a1_intro1"
        "notmulti" "1"
    }
    "2"
    {
        "label" "#GameUI_GameMenu_LoadGame"
        "command" "OpenLoadGameDialog"
        "notmulti" "1"
    }
    "3"
    {
        "label" "#GameUI_GameMenu_ResumeGame"
        "command" "ResumeGame"
        "InGameOrder" "10"
        "OnlyInGame" "1"
    }
    "4"
    {
        "label" "#GameUI_GameMenu_SaveGame"
        "command" "OpenSaveGameDialog"
        "InGameOrder" "20"
        "OnlyInGame" "1"
        "notmulti" "1"
    }
    "5"
    {
        "label" "#GameUI_GameMenu_LoadGame"
        "command" "OpenLoadGameDialog"
        "InGameOrder" "30"
        "OnlyInGame" "1"
        "notmulti" "1"
    }
    "6"
    {
        "label" "#GameUI_GameMenu_Disconnect"
        "command" "Disconnect"
        "InGameOrder" "80"
        "OnlyInGame" "1"
    }
    "9"
    {
        "label" "#GameUI_GameMenu_Options"
        "command" "OpenOptionsDialog"
        "InGameOrder" "90"
    }
    "10"
    {
        "label" "#GameUI_GameMenu_Quit"
        "command" "Quit"
        "InGameOrder" "100"
    }
}
''')
shutil.copy2(p1_scheme, menu_resource / 'clientscheme.res')
shutil.copy2(p1_scheme, menu_resource / 'clientscheme_override.res')
# The P1-derived client's touch-screen HUD looks up vgui/touch/* and the menu
# background, but Portal 2 ships neither asset set. Provide minimal VGUI
# materials in the override so FindMaterial stops failing (cosmetic noise).
touch_materials = ('use', 'jump', 'shoot', 'shoot_alt', 'crouch', 'tduck',
                   'zoom', 'speed', 'load', 'save', 'reload',
                   'flash_light_filled', 'next_weap', 'prev_weap', 'settings',
                   'menu', 'back')
vgui_touch = stage / 'portal2_override/materials/vgui/touch'
vgui_touch.mkdir(parents=True, exist_ok=True)
for name in touch_materials:
    (vgui_touch / f'{name}.vmt').write_text(
        '"UnlitGeneric"\n{\n\t"$basetexture" "vgui/white"\n}\n')
console_mat = stage / 'portal2_override/materials/console'
console_mat.mkdir(parents=True, exist_ok=True)
# The classic GameUI asks the engine for console/backgroundNN, while Portal 2
# stores its authored menu backgrounds under vgui/backgrounds.  Put lossless
# copies in the isolated override and adapt the material names the ARM64
# GameUI actually resolves.  This also supplies the launcher's raw VTF lookup
# before GameUI has loaded a VMT.
p2_pak = vpk.open(str(p2 / 'portal2/pak01_dir.vpk'))
for index in range(1, 6):
    for widescreen in ('', '_widescreen'):
        suffix = f'background{index:02d}{widescreen}'
        source_vtf = f'materials/vgui/backgrounds/{suffix}.vtf'
        target_vtf = console_mat / f'{suffix}.vtf'
        target_vtf.write_bytes(p2_pak[source_vtf].read())
        (console_mat / f'{suffix}.vmt').write_text(
            '"UnlitGeneric"\n{\n'
            f'\t"$basetexture" "console/{suffix}"\n'
            '\t"$vertexcolor" "1"\n'
            '\t"$vertexalpha" "1"\n'
            '\t"$ignorez" "1"\n'
            '\t"$nolod" "1"\n'
            '}\n')
(console_mat / 'background_menu.vmt').write_text(
    '"UnlitGeneric"\n{\n\t"$basetexture" "console/background01"\n}\n')

(stage / 'portal2/cfg/portal2_arm64.cfg').write_text('''// Isolated compatibility aliases; they do not touch Steam's Portal 2 cfg.
alias +zoom_in +zoom
alias -zoom_in -zoom
alias +zoom_out +zoom
alias -zoom_out -zoom
# Static render bounds can interfere with authored movers such as elevators.
# Keep this diagnostic fallback off until a model-specific collision proxy exists.
portal2_staticprop_bbox_fallback 0
portal2_dynamicprop_bbox_fallback 1
cl_drawhud 0
''')
(stage / 'Jugar-Portal2-Experimental.command').write_text('''#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
exec ./hl2_osx -game portal2 -novid -nosoundcachewrite -windowed -w 1024 -h 768 \\
  +exec portal2_arm64 \\
  +gameui_activate \\
  +sv_cheats 1 +cl_drawhud 0 +mat_fullbright 0 +mat_disable_bloom 1 +mat_colorcorrection 0 \\
  +bind F10 gameui_activate +bind ESCAPE gameui_activate \\
  +bind z +zoom +bind KP_INS +zoom_in \\
  +bind F6 portal2_equip_portalgun +bind F7 portal2_intro_playground
''')
(stage / 'Jugar-Portal2-Experimental.command').chmod(0o755)
(stage / 'Jugar-Portal2-PortalGun.command').write_text('''#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
exec ./hl2_osx -game portal2 -novid -nosoundcachewrite -portal2_intro_playground \\
  +exec portal2_arm64 \\
  -windowed -w 1024 -h 768 +sv_cheats 1 +mat_fullbright 0 \\
  +mat_disable_bloom 1 +mat_colorcorrection 0 \\
  +bind F10 gameui_activate +bind ESCAPE gameui_activate \\
  +bind z +zoom +bind KP_INS +zoom_in \\
  +bind F6 portal2_equip_portalgun +bind F7 portal2_intro_playground \\
  +bind MOUSE1 +attack +bind MOUSE2 +attack2 +bind e +use \\
  +bind w +forward +bind s +back +bind a +moveleft +bind d +moveright \\
  +bind SPACE +jump +map sp_a1_intro1
''')
(stage / 'Jugar-Portal2-PortalGun.command').chmod(0o755)
print(f'Staged isolated map experiment: {stage}')
print('Run Jugar-Portal2-Experimental.command; this is not the full Portal 2 game.')
