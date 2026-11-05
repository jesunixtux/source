#!/usr/bin/env python3
"""Stage an isolated ARM64 launch directory for the 2010 Portal 2 beta.

The beta ships i386 bins and a 64-combo shader cache. This script leaves the
original binaries in place and mounts the loose content with a VCS 6 /
288-combo shader cache. Content symlinks are not an OS read-only sandbox.
"""
import os
import hashlib
import re
import shutil
import struct
import subprocess
import sys
import zlib
from pathlib import Path

source = Path(__file__).resolve().parent.parent
beta = Path(os.environ.get(
    'PORTAL2_BETA_DIR',
    '/Volumes/External-SSD-WDGreen/Games/Local/Portal_2_beta_2010_corehub_2')).resolve()
build = Path(os.environ.get('BUILD_DIR', source / 'build')).resolve()
stage = Path(os.environ.get(
    'PORTAL2_STAGE_DIR', beta / 'portal2_arm64_stage')).resolve()

def portal_dir():
    candidates = [os.environ.get('PORTAL_DIR'),
                  str(Path.home() / 'Library/Application Support/Steam/steamapps/common/Portal'),
                  str(Path.home() / 'GameHub/steamapps/common/Portal')]
    for candidate in candidates:
        if candidate and (Path(candidate) / 'hl2/hl2_misc_dir.vpk').is_file():
            return Path(candidate).resolve()
    sys.exit('Missing Portal hl2/hl2_misc_dir.vpk (VCS 6, 288 dynamic combos)')

def die(message):
    sys.exit(message)

def require_arm64_portal2():
    launcher = build / 'launcher_main/hl2_launcher'
    if not launcher.is_file():
        die(f'Missing {launcher}; run scripts/build-macos-arm64.sh portal2')
    kind = subprocess.check_output(['file', str(launcher)], text=True)
    if 'arm64' not in kind:
        die(f'Launcher is not arm64: {kind.strip()}')
    for module in ('client', 'server'):
        cache = build / f'c4che/game/{module}_cache.py'
        text = cache.read_text() if cache.is_file() else ''
        if "GAMES = 'portal2'" not in text:
            die(f'{module} is not configured for portal2; run scripts/build-macos-arm64.sh portal2')
        library = build / f'game/{module}/lib{module}.dylib'
        if not library.is_file() or 'arm64' not in subprocess.check_output(['file', str(library)], text=True):
            die(f'Missing ARM64 module: {library}')

def vpk_extract(archive, predicate, output):
    data = archive.read_bytes()
    if len(data) < 12:
        raise ValueError(f'Truncated VPK header: {archive}')
    signature, version, tree_size = struct.unpack_from('<III', data, 0)
    if signature != 0x55AA1234 or version not in (1, 2):
        die(f'Unsupported VPK: {archive}')
    cursor = 12 if version == 1 else 28
    tree_end = cursor + tree_size
    if tree_end > len(data):
        raise ValueError(f'Truncated VPK tree: {archive}')

    def cstring():
        nonlocal cursor
        end = data.index(b'\x00', cursor, tree_end)
        text = data[cursor:end].decode('latin1')
        cursor = end + 1
        return text

    count = 0
    while True:
        extension = cstring()
        if not extension:
            break
        while True:
            directory = cstring()
            if not directory:
                break
            while True:
                filename = cstring()
                if not filename:
                    break
                if cursor + 18 > tree_end:
                    raise ValueError('Truncated VPK entry')
                crc, preload, archive_index, offset, length, term = struct.unpack_from('<IHHIIH', data, cursor)
                cursor += 18
                if term != 0xffff or cursor + preload > tree_end:
                    raise ValueError('Invalid VPK entry')
                preload_bytes = data[cursor:cursor + preload]
                cursor += preload
                key = f'{directory}/{filename}.{extension}' if directory != ' ' else f'{filename}.{extension}'
                key = key.replace('\\', '/')
                if Path(key).is_absolute() or '..' in Path(key).parts:
                    raise ValueError(f'Refusing VPK path {key}')
                if not predicate(key):
                    continue
                blob = bytearray(preload_bytes)
                if length:
                    if archive_index == 0x7fff:
                        blob += data[tree_end + offset:tree_end + offset + length]
                    else:
                        part = archive.with_name(archive.name.removesuffix('_dir.vpk') + f'_{archive_index:03d}.vpk')
                        with part.open('rb') as stream:
                            stream.seek(offset)
                            blob += stream.read(length)
                if len(blob) != preload + length or zlib.crc32(blob) != crc:
                    raise ValueError(f'Truncated or corrupt VPK data: {key}')
                path = output / key.lower()
                if not path.resolve().is_relative_to(output.resolve()):
                    raise ValueError(f'Refusing output symlink escape: {key}')
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(blob)
                count += 1
    return count

def shader_wanted(key):
    return key.startswith('shaders/') or (
        key.startswith('materials/cable/') and key.endswith('.vmt')) or key in (
            'materials/dev/bloomadd.vmt', 'materials/dev/lumcompare.vmt',
            'materials/dev/engine_post.vmt')

def stage_beta_vgui(content, override):
    resource = override / 'resource'
    resource.mkdir(parents=True, exist_ok=True)
    origins = []
    for name in ('SourceScheme.res', 'ClientScheme.res', 'basemodui_scheme.res', 'GameMenu.res'):
        original = content / 'resource' / name
        target = resource / name.lower()
        shutil.copy2(original, target)
        origins.append(f'{hashlib.sha256(target.read_bytes()).hexdigest()}  {name}  {original}')
    (resource / 'VGUI_ORIGIN.txt').write_text('\n'.join(origins) + '\n')
    # The beta references background_menu but does not ship its BSP.
    console = override / 'materials/console'
    console.mkdir(parents=True, exist_ok=True)
    for aspect in ('', '_widescreen'):
        for suffix in ('.vmt', '.vtf'):
            shutil.copy2(content / f'materials/console/background01{aspect}{suffix}',
                          console / f'background_menu{aspect}{suffix}')

def stage_beta_portal_effects(portal, content, output):
    archives = [portal / name for name in ('portal/portal_pak_dir.vpk',
                                          'hl2/hl2_misc_dir.vpk', 'hl2/hl2_textures_dir.vpk')]
    pcfs = {'particles/portalgun.pcf', 'particles/portal_projectile.pcf', 'particles/portals.pcf'}
    pending = set(pcfs)
    # These names are requested by the P1-derived native portal/gun renderer.
    prefixes = ('materials/models/portals/', 'materials/effects/portal_',
                'materials/sprites/grav_')
    sprites = {'glow04_noz', 'bluelight', 'orangelight', 'whitelight'}
    pending.update(f'materials/sprites/{name}.vmt' for name in sprites)
    pending.update(('materials/particle/particle_glow_05.vmt', 'materials/decals/portalstencildecal.vmt'))
    for archive in archives:
        vpk_extract(archive, lambda key: key.lower().startswith(prefixes), output)
    pending.update(str(path.relative_to(output)) for path in output.rglob('*.vmt'))
    written = set()
    while pending:
        for archive in archives:
            vpk_extract(archive, lambda key: key.lower() in pending and not (output / key.lower()).is_file(), output)
        current, pending = pending, set()
        for name in sorted(current - written):
            path = output / name
            data = path.read_bytes() # Missing native dependencies must not be silently skipped.
            written.add(name)
            if name in pcfs:
                # shortcut: use compatible effects until this branch decodes the beta's DMX-5 PCFs.
                if b'dmx encoding binary 2' not in data[:100]:
                    raise ValueError(f'Expected DMX-2 particle: {name}')
                refs = re.findall(rb'[A-Za-z0-9_./-]+\.vmt', data)
                pending.update(ref.decode().lower() if ref.startswith(b'materials/') else
                               'materials/' + ref.decode().lower() for ref in refs)
            elif name.endswith('.vmt'):
                text = data.decode('utf8', errors='replace')
                for ref in re.findall(r'"?\$(?:basetexture|bumpmap|detail|envmapmask|normalmap|dudvmap|phongexponenttexture|lightwarptexture|selfillummask)"?\s+"?([^"\s{}]+)', text, re.I):
                    pending.add('materials/' + ref.replace('\\', '/').lower().removesuffix('.vtf') + '.vtf')
                pending.update(ref.replace('\\', '/').lower() for ref in re.findall(r'"?include"?\s+"([^"\n]+)"', text, re.I))
        pending -= written
    manifest = (content / 'particles/particles_manifest.txt').read_text()
    end = manifest.rfind('}')
    if end < 0:
        raise ValueError('Invalid beta particle manifest')
    extra = ''.join(f'\n    "file" "{name}"' for name in sorted(pcfs) if name not in manifest)
    (output / 'particles/particles_manifest.txt').write_text(manifest[:end] + extra + '\n' + manifest[end:])
    files = sorted(path for path in output.rglob('*') if path.is_file())
    (output / 'ORIGIN.txt').write_text('Portal/HL2 compatible portal effects; beta gun models and VGUI are not replaced.\n' +
        '\n'.join(f'{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(output)}' for path in files) + '\n')
    print(f'Compatible portal effects: {len(files)} files in {output}')

def main():
    require_arm64_portal2()
    portal = portal_dir()
    game = beta / 'game'
    content = game / 'portal2'
    if not (content / 'maps/sp_a1_intro1.bsp').is_file():
        die(f'Beta content not found: {content}')
    if stage.exists():
        die(f'Refusing to overwrite existing staging: {stage}')
    try:
        stage.resolve().relative_to(content.resolve())
        die('Refusing to stage inside the beta portal2 content directory')
    except ValueError:
        pass

    for folder in ('bin', 'portal2/cfg', 'portal2/save', 'portal2_override/cfg', 'renderer_compat'):
        (stage / folder).mkdir(parents=True)
    shutil.copy2(build / 'launcher_main/hl2_launcher', stage / 'hl2_osx')
    os.chmod(stage / 'hl2_osx', 0o755)
    for lib in build.rglob('*.dylib'):
        target = stage / 'bin' / lib.name
        if target.exists() or target.is_symlink():
            die(f'Duplicate library name in build: {lib.name}')
        target.symlink_to(lib)

    skip = {'bin', 'cfg', 'save', 'shaders'}
    for child in content.iterdir():
        if child.name.lower() in skip or child.name.lower().startswith('gameinfo'):
            continue
        destination = stage / 'portal2' / child.name
        if child.is_dir():
            destination.symlink_to(child, target_is_directory=True)
        elif child.is_file():
            destination.symlink_to(child)
    for cfg in (content / 'cfg').glob('*.cfg'):
        if cfg.name.lower() in ('config.cfg', 'autoexec.cfg', 'video.cfg', 'videoconfig.cfg'):
            continue
        shutil.copy2(cfg, stage / 'portal2/cfg' / cfg.name)

    platform = stage / 'platform'
    platform.mkdir()
    for child in (game / 'platform').iterdir():
        if child.name.lower() == 'shaders':
            continue
        if child.is_dir():
            shutil.copytree(child, platform / child.name)
        else:
            shutil.copy2(child, platform / child.name)

    misc = portal / 'hl2/hl2_misc_dir.vpk'
    renderer = stage / 'renderer_compat'
    count = vpk_extract(misc, shader_wanted, renderer)
    header = struct.unpack('<7I', (renderer / 'shaders/fxc/lightmappedgeneric_ps20b.vcs').read_bytes()[:28])
    if header[0] != 6 or header[2] != 288:
        die(f'Shader cache is VCS {header[0]} dynamic {header[2]}, expected 6/288')
    for alias in (stage / 'shaders', stage / 'portal2/shaders', platform / 'shaders'):
        alias.symlink_to(renderer / 'shaders', target_is_directory=True)

    paths = [
        ('Game', '|gameinfo_path|../renderer_compat'),
        ('Game', '|gameinfo_path|../portal2_effects_compat'),
        ('Game', '|gameinfo_path|../portal2_override'),
        ('Game', '|gameinfo_path|.'),
        ('Game', str(game / 'portal2_tempcontent')),
        ('Game', str(game / 'portal')),
        ('Game', '|gameinfo_path|../platform'),
        ('Platform', '|gameinfo_path|../platform'),
        ('Mod', '|gameinfo_path|../portal2_override'),
        ('Default_Write_Path', '|gameinfo_path|../portal2_override'),
    ]
    if any('"' in path or '\n' in path or '\r' in path for _, path in paths):
        die('Unsupported quote/newline in install path')
    search = '\n'.join(f'            {kind} "{path}"' for kind, path in paths)
    (stage / 'portal2/gameinfo.txt').write_text('''"GameInfo"
{
    game "PORTAL 2 ARM64 Beta"
    title "PORTAL 2 ARM64 Beta"
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

    override = stage / 'portal2_override'
    stage_beta_vgui(content, override)
    stage_beta_portal_effects(portal, content, stage / 'portal2_effects_compat')
    # Keep startup independent of development configs mounted from the beta.
    (override / 'cfg/valve.rc').write_text('exec config_default.cfg\nstuffcmds\nstartupmenu\n')
    (override / 'cfg/autoexec.cfg').write_text('// Isolated ARM64 beta configuration.\n')
    subprocess.run([sys.executable, str(source / 'scripts/prepare-portal2-scenes.py'),
                    str(content / 'scenes/scenes.image'),
                    str(override / 'scenes/scenes.image')], check=True)
    touch = override / 'materials/vgui/touch'
    touch.mkdir(parents=True)
    for name in ('use', 'jump', 'shoot', 'shoot_alt', 'crouch', 'tduck', 'zoom',
                 'speed', 'load', 'save', 'reload', 'flash_light_filled',
                 'next_weap', 'prev_weap', 'settings', 'menu', 'back'):
        (touch / f'{name}.vmt').write_text('"UnlitGeneric"\n{\n\t"$basetexture" "vgui/white"\n}\n')

    command = stage / 'Jugar-Portal2-Beta.command'
    command.write_text('''#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
exec ./hl2_osx -game portal2 -novid -nosoundcachewrite -windowed -w 1280 -h 720 \\
  +sv_cheats 1 +map sp_a1_intro1 "$@"
''')
    command.chmod(0o755)
    menu = stage / 'Menu-Portal2-Beta.command'
    menu.write_text('''#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
exec ./hl2_osx -game portal2 -novid -nosoundcachewrite -nobackgroundlevel \\
  -windowed -w 1280 -h 720 -condebug +gameui_activate "$@"
''')
    menu.chmod(0o755)
    print(f'Staged {count} shader files from {misc}')
    print(f'Staged ARM64 beta launch: {stage}')
    print('The 2010 i386 bins were not copied. Run Jugar-Portal2-Beta.command')

if __name__ == '__main__':
    main()
