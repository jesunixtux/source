#!/usr/bin/env python3
"""Stage shaders matching this Orange Box/Anniversary renderer, not Portal 2's ABI."""
import argparse
from pathlib import Path
import struct
import vpk

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('stage', type=Path)
parser.add_argument('archive', type=Path, help='Portal or HL2 hl2_misc_dir.vpk')
args = parser.parse_args()
stage = args.stage.resolve()
if not (stage / 'portal2/gameinfo.txt').is_file():
    parser.error('Expected an isolated Portal 2 staging directory')
archive = vpk.open(str(args.archive))
header = struct.unpack('<7I', archive['shaders/fxc/lightmappedgeneric_ps20b.vcs'].read()[:28])
if header[0] != 6 or header[2] != 288:
    parser.error('Cache does not match current generated lightmappedgeneric headers (VCS 6, 288 dynamic combos)')
output = stage / 'renderer_compat'
count = 0
for key in archive:
    # Postprocessing parameters/proxies also belong to the compiled renderer.
    if not (key.startswith('shaders/') or
            (key.startswith('materials/cable/') and key.endswith('.vmt')) or key in (
        'materials/dev/bloomadd.vmt', 'materials/dev/lumcompare.vmt',
        'materials/dev/engine_post.vmt')):
        continue
    path = output / key
    if '..' in Path(key).parts:
        raise ValueError(key)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(archive[key].read())
    count += 1
# These are staging-owned aliases. Preserve every prior target for rollback.
for alias in (stage / 'shaders', stage / 'portal2/shaders', stage / 'portal2_override/shaders'):
    target = output / 'shaders'
    if alias.is_symlink() and alias.resolve() == target.resolve():
        continue
    if alias.exists() or alias.is_symlink():
        backup = alias.with_name(alias.name + '.before-renderer-compat')
        if backup.exists() or backup.is_symlink():
            raise RuntimeError(f'Backup already exists: {backup}')
        alias.rename(backup)
    alias.symlink_to(target, target_is_directory=True)
print(f'Staged {count} matching shader files in {output}; previous aliases backed up.')
