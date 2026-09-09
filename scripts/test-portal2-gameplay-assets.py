#!/usr/bin/env python3
"""Check gun asset provenance; generate only a temporary stage unless --stage is supplied."""
import argparse
import hashlib
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

import vpk


PCFS = ('particles/portalgun.pcf', 'particles/portal_projectile.pcf',
        'particles/portals.pcf')
GUN_MODELS = ('models/weapons/v_portalgun.', 'models/weapons/w_portalgun.')
PLAYER_MODELS = ('models/player/chell/player.', 'models/player_animations.')
GUN_MATERIALS = ('materials/models/weapons/v_models/v_portalgun/',
                 'materials/models/weapons/w_models/portalgun/')
# Include the shader inputs not covered by prepare's generic dependency scan.
# The gun's prefix extraction must supply these, too.
TEXTURE_KEYS = ('basetexture', 'bumpmap', 'detail', 'envmapmask', 'normalmap',
                'dudvmap', 'phongexponenttexture', 'lightwarptexture',
                'selfillummask')


def archive_index(archives):
    result = {}
    for archive in archives:
        for name in archive:
            result.setdefault(name.lower(), (archive, name))
    return result


def read_entry(index, name):
    archive, original = index[name]
    return archive[original].read()


def verify(stage, portal, portal2):
    output = stage / 'portal2_gameplay_compat'
    modern = archive_index([vpk.open(str(portal2 / 'portal2/pak01_dir.vpk'))])
    legacy = archive_index([vpk.open(str(portal / name)) for name in
                            ('portal/portal_pak_dir.vpk', 'hl2/hl2_misc_dir.vpk',
                             'hl2/hl2_textures_dir.vpk')])
    failures = []
    required = set(PCFS + ('scripts/weapon_portalgun.txt',))
    counts = {'legacy_particles': 0, 'portal2_models': 0, 'portal2_textures': 0,
              'player_models': 0, 'gun_materials': 0, 'texture_references': 0, 'origin_hashes': 0}

    def check(condition, description):
        if not condition:
            failures.append(description)

    def staged(name):
        path = output / name
        if not path.is_file():
            failures.append('Missing staged asset: ' + name)
            return b''
        return path.read_bytes()

    for name in PCFS:
        actual = staged(name)
        check(actual == read_entry(legacy, name), 'Not the Portal 1 particle bytes: ' + name)
        check(b'dmx encoding binary 2' in actual[:100], 'Not a DMX binary-2 particle: ' + name)
        counts['legacy_particles'] += 1
    weapon = 'scripts/weapon_portalgun.txt'
    check(staged(weapon) == read_entry(legacy, weapon), 'Weapon definition must retain the Portal 1 fallback')

    for name in modern:
        if name.startswith(PLAYER_MODELS):
            required.add(name)
            check(staged(name) == read_entry(modern, name), 'Not the Portal 2 player model bytes: ' + name)
            counts['player_models'] += 1
        if name.startswith(GUN_MODELS):
            required.add(name)
            data = staged(name)
            check(data == read_entry(modern, name), 'Not the Portal 2 model bytes: ' + name)
            if name.endswith('.mdl'):
                check(len(data) >= 8 and data[:4] == b'IDST' and
                      struct.unpack_from('<i', data, 4)[0] == 49,
                      'Not a Studio 49 gun model: ' + name)
            counts['portal2_models'] += 1
        elif name.startswith(GUN_MATERIALS) and name.endswith('.vtf'):
            required.add(name)
            check(staged(name) == read_entry(modern, name), 'Not the Portal 2 texture bytes: ' + name)
            counts['portal2_textures'] += 1
        elif name.startswith(GUN_MATERIALS) and name.endswith('.vmt'):
            required.add(name)
            source = read_entry(modern, name).decode('utf8', errors='replace')
            actual = staged(name).decode('utf8', errors='replace')
            # Only materials with unsupported client proxies are allowed to be
            # rewritten; every other Portal 2 gun material stays byte-exact.
            unsupported = re.search(r'\b(?:FizzlerVortex|LightedFloorButton|LightedMouth)\b', source, re.I)
            if unsupported:
                check(not re.search(r'\bProxies\s*\{', actual, re.I),
                      'Unsupported client proxy retained: ' + name)
            else:
                check(actual == source, 'Unexpected gun material rewrite: ' + name)
            pattern = r'"?\$(' + '|'.join(TEXTURE_KEYS) + r')"?\s+"?([^"\s{}]+)'
            for key, ref in re.findall(pattern, source, re.I):
                texture = 'materials/' + ref.replace('\\', '/').lower().removesuffix('.vtf') + '.vtf'
                required.add(texture)
                check((output / texture).is_file(), f'Missing ${key} texture for {name}: {texture}')
                # Proxy removal must not discard texture assignments.
                check(any(k.lower() == key.lower() and value == ref
                          for k, value in re.findall(pattern, actual, re.I)),
                      f'Lost ${key} texture assignment: {name}')
                counts['texture_references'] += 1
            counts['gun_materials'] += 1
    check(counts['portal2_models'] >= 6, 'Expected both complete Portal 2 gun model sets')
    check(counts['player_models'] > 0, 'No Portal 2 Chell model files checked')
    check(counts['portal2_textures'] > 0, 'No Portal 2 gun textures checked')
    check(counts['gun_materials'] > 0, 'No Portal 2 gun materials checked')

    manifest = staged('particles/particles_manifest.txt').decode(errors='replace')
    for name in PCFS:
        check(name in manifest, 'Particle missing from manifest: ' + name)
    recorded = set()
    for digest, name in re.findall(r'^([0-9a-f]{64})  (.+)$', staged('ORIGIN.txt').decode(), re.M):
        check(hashlib.sha256(staged(name)).hexdigest() == digest, 'Incorrect ORIGIN hash: ' + name)
        recorded.add(name)
        counts['origin_hashes'] += 1
    # A full game stage also contains intro audio/localization generated by
    # other helpers. Their files do not belong to this helper's ORIGIN list.
    for name in required:
        check(name in recorded, 'Gun asset missing from ORIGIN: ' + name)
    check(counts['origin_hashes'] > 0, 'No asset provenance hashes checked')

    for failure in failures:
        print('FAIL: ' + failure, file=sys.stderr)
    print(('FAIL' if failures else 'PASS') + ': ' + ', '.join(f'{key}={value}' for key, value in counts.items()))
    return not failures


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('portal', type=Path, help='Installed Portal directory (read-only)')
    parser.add_argument('portal2', type=Path, help='Installed Portal 2 directory (read-only)')
    parser.add_argument('--stage', type=Path, help='Validate an existing stage without modifying it')
    args = parser.parse_args()
    if args.stage is not None:
        return verify(args.stage, args.portal, args.portal2)
    with tempfile.TemporaryDirectory(prefix='portal2-gun-assets-test-') as temporary:
        stage = Path(temporary)
        subprocess.run([sys.executable, str(Path(__file__).with_name('prepare-portal2-gameplay.py')),
                        str(stage), str(args.portal), str(args.portal2)], check=True)
        return verify(stage, args.portal, args.portal2)


if __name__ == '__main__':
    try:
        sys.exit(0 if main() else 1)
    except (OSError, KeyError, subprocess.CalledProcessError) as error:
        print('FAIL: ' + str(error), file=sys.stderr)
        sys.exit(1)
