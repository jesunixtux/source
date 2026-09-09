#!/usr/bin/env python3
"""Stage Portal 2 portal-gun/portal visuals with compatible particle fallbacks."""
import argparse
import hashlib
from pathlib import Path
import re
import vpk

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('stage', type=Path)
p.add_argument('portal', type=Path)
p.add_argument('portal2', type=Path)
a = p.parse_args()
out = a.stage.resolve() / 'portal2_gameplay_compat'
# Portal 2's view/world gun and portal frame models are Studio 49 and are the
# visual target of this port.  The ARM64 Studio decoder accepts that format.
# Portal 1 remains a fallback for the weapon definition and its DMX-2 particle
# files; no asset inside either installed game is modified.
portal2_archive = vpk.open(str(a.portal2 / 'portal2/pak01_dir.vpk'))
legacy_archives = [vpk.open(str(a.portal / name)) for name in
                   ('portal/portal_pak_dir.vpk', 'hl2/hl2_misc_dir.vpk', 'hl2/hl2_textures_dir.vpk')]
entries = {}
for archive in [portal2_archive] + legacy_archives:
    for name in archive:
        entries.setdefault(name.lower(), (archive, name))
legacy_entries = {}
for archive in legacy_archives:
    for name in archive:
        legacy_entries.setdefault(name.lower(), (archive, name))
prefixes = ('models/weapons/v_portalgun.', 'models/weapons/w_portalgun.', 'models/portals/',
            'models/player/chell/player.', 'models/player_animations.',
            'materials/models/player/chell/',
            'materials/models/weapons/v_models/v_portalgun/',
            'materials/models/weapons/w_models/portalgun/', 'materials/models/portals/',
            'materials/effects/portal_', 'materials/sprites/portalgun_effects',
            'materials/sprites/hud/portal_crosshairs')
pcfs = ['particles/portalgun.pcf', 'particles/portal_projectile.pcf', 'particles/portals.pcf']
legacy_fallbacks = set(pcfs + ['scripts/weapon_portalgun.txt'])
pending = [name for name in entries if name.startswith(prefixes)]
pending += pcfs + ['scripts/weapon_portalgun.txt']
# Particle material references are null-terminated strings in DMX binary 2.
for name in pcfs:
    archive, original = legacy_entries[name]
    data = archive[original].read()
    if b'dmx encoding binary 2' not in data[:100]:
        p.error('Portal particle cache is not compatible DMX binary 2: ' + name)
    for match in re.findall(rb'[A-Za-z0-9_./-]+\.vmt', data):
        material = match.decode().lower()
        pending.append(material if material.startswith('materials/') else 'materials/' + material)
written = {}
while pending:
    name = pending.pop().lower()
    if name in written or name not in entries:
        continue
    # These files must come from the same legacy archive whose DMX header was
    # validated above. The merged index otherwise prefers Portal 2's binary-5
    # PCFs, which the binary-2 decoder cannot load.
    archive, original = (legacy_entries if name in legacy_fallbacks else entries)[name]
    data = archive[original].read()
    target = out / name
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    written[name] = hashlib.sha256(data).hexdigest()
    if name.endswith('.vmt'):
        text = data.decode('utf8', errors='replace')
        # Portal 2's gun materials use client proxies that do not exist in
        # this renderer.  Keep the textures/self-illum and make the material
        # static instead of producing an error every frame.
        if re.search(r'\b(?:FizzlerVortex|LightedFloorButton|LightedMouth)\b', text, re.I):
            text = re.sub(r'(?is)\bProxies\s*\{(?:[^{}]|\{[^{}]*\})*\}', '', text)
            data = text.encode('utf8')
            target.write_bytes(data)
            written[name] = hashlib.sha256(data).hexdigest()
        for ref in re.findall(r'"?\$(?:basetexture|bumpmap|detail|envmapmask|normalmap|dudvmap)"?\s+"?([^"\s{}]+)', text, re.I):
            pending.append('materials/' + ref.replace('\\', '/').lower().removesuffix('.vtf') + '.vtf')
        for ref in re.findall(r'"?include"?\s+"([^"\n]+)"', text, re.I):
            pending.append(ref.lower())

manifest = a.portal2 / 'portal2/particles/particles_manifest.txt'
if manifest.is_file():
    text = manifest.read_text(errors='replace')
else:
    archive = vpk.open(str(a.portal2 / 'portal2/pak01_dir.vpk'))
    text = archive['particles/particles_manifest.txt'].read().decode()
end = text.rfind('}')
if end < 0:
    p.error('Invalid Portal 2 particle manifest')
extra = ''.join(f'\n    "file" "{name}"' for name in pcfs if name not in text)
(out / 'particles/particles_manifest.txt').write_text(text[:end] + extra + '\n' + text[end:])
(out / 'ORIGIN.txt').write_text('Portal 2 Studio 49 portal-gun and portal-frame assets.\n'
    'Portal 1 DMX-2 particle fallback and weapon definition retained for the compatible renderer.\n'
    'Portal 2 particle manifest preserved, with the fallback effect files appended.\n' +
    '\n'.join(f'{digest}  {name}' for name, digest in sorted(written.items())) + '\n')
print(f'Gameplay assets: {len(written)} files in {out}')
