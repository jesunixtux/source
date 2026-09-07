#!/usr/bin/env python3
"""Generate explicit, lossy material adapters ONLY inside a new Portal 2 stage."""
import argparse
from pathlib import Path
import re
import vpk

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('stage', type=Path)
p.add_argument('content', type=Path)
args = p.parse_args()
stage = args.stage.resolve()
if not (stage / 'portal2/gameinfo.txt').is_file():
    p.error('Expected isolated staging tree')
out = stage / 'portal2_material_compat'
if out.exists():
    p.error(f'Refusing to overwrite existing material adapters: {out}')
out.mkdir()
sources = [args.content / 'portal2/pak01_dir.vpk']
sources += [args.content / dlc / 'pak01_dir.vpk' for dlc in ('portal2_dlc1', 'portal2_dlc2')]
materials = {}
for path in sources:
    if path.is_file():
        pak = vpk.open(str(path))
        for key in pak:
            if key.startswith('materials/') and key.endswith('.vmt'):
                materials[key] = pak[key].read().decode('utf-8', errors='replace')
manifest = []
for key, original in materials.items():
    # Ignore comments while identifying shader/flags; retain the original text.
    clean = re.sub(r'//[^\n]*', '', original)
    updated = original
    reasons = []
    if re.match(r'\s*"?LightMappedGeneric"?\s*\{', clean, re.I):
        # These modes are explicitly SKIPped in lightmappedgeneric_ps2x.fxc.
        updated, n = re.subn(r'(?im)^(\s*"?\$detailblendmode"?\s+)"?[2-9]"?(?=\s|$)',
                             r'\g<1>0', updated)
        if n:
            reasons.append('unsupported detail blend -> modulation (approximation)')
    if re.match(r'\s*"?VertexLitGeneric"?\s*\{', clean, re.I) and re.search(r'\$selfillum"?\s+"?1\b', clean, re.I):
        # The compiled shader explicitly excludes tint-by-alpha + selfillum.
        updated, n = re.subn(r'(?im)^(\s*"?\$blendtintbybasealpha"?\s+)"?1"?(?=\s|$)',
                             r'\g<1>0', updated)
        if n:
            reasons.append('preserve selfillum, disable conflicting alpha tint')
    if updated == original:
        continue
    if '..' in Path(key).parts:
        raise ValueError(key)
    path = out / key
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(updated)
    manifest.append(f'{key}: {"; ".join(reasons)}')
(out / 'ADAPTATIONS.txt').write_text('\n'.join(manifest) + '\n')
print(f'Created {len(manifest)} explicit material adapters; originals unchanged: {out}')
