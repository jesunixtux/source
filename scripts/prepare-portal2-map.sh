#!/bin/bash
set -euo pipefail

# Extract map-referenced model families into the isolated ARM64 overlay.  The
# legacy macOS filesystem layer can fail to resolve some files inside Portal 2
# VPK chunks; loose files keep the original installation untouched.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
USER_HOME_DIR="$(dscl . -read "/Users/$(id -un)" NFSHomeDirectory | awk '{print $2}')"
P2_DIR="${PORTAL2_DIR:-$USER_HOME_DIR/Library/Application Support/Steam/steamapps/common/Portal 2}"
PORTAL_DIR="${PORTAL_DIR:-$USER_HOME_DIR/Library/Application Support/Steam/steamapps/common/Portal}"
STAGE_DIR="${PORTAL2_STAGE_DIR:-$P2_DIR/portal2_arm64_test}"
MAP_NAME="${1:-sp_a1_intro1}"
BSP_PATH="$P2_DIR/portal2/maps/$MAP_NAME.bsp"
PAK_PATH="$P2_DIR/portal2/pak01_dir.vpk"
BASE_PAK_PATH="$PORTAL_DIR/hl2/hl2_misc_dir.vpk"
OUT_DIR="$STAGE_DIR/portal2_override"

[ -f "$BSP_PATH" ] || { echo "ERROR: map not found: $BSP_PATH" >&2; exit 1; }
[ -f "$PAK_PATH" ] || { echo "ERROR: Portal 2 VPK not found: $PAK_PATH" >&2; exit 1; }
[ -d "$STAGE_DIR" ] || { echo "ERROR: create the staging tree first with scripts/stage-macos-portal2.sh" >&2; exit 1; }

# New staging mounts archives natively. Do not apply the historical extraction
# and UnlitGeneric workaround on top of the repaired renderer.
case "$(<"$STAGE_DIR/portal2/gameinfo.txt")" in
    *renderer_compat*)
        echo "Map available: $BSP_PATH"
        echo "Native VPK staging detected; no extraction or material replacement needed."
        exit 0
        ;;
esac

STAGE_DIR="$STAGE_DIR" BSP_PATH="$BSP_PATH" PAK_PATH="$PAK_PATH" BASE_PAK_PATH="$BASE_PAK_PATH" OUT_DIR="$OUT_DIR" MAP_NAME="$MAP_NAME" \
python3 - <<'PY'
import os
import re
import vpk

bsp = open(os.environ['BSP_PATH'], 'rb').read()
models = sorted({m.decode('ascii').lower() for m in re.findall(rb'models/[A-Za-z0-9_./-]+\.mdl', bsp)})
# BSP texdata stores material names without the materials/ prefix and without
# an extension.  The legacy filesystem can mount the map while failing to
# resolve the same files from VPK chunks, so stage the map's surface materials
# as loose files in the overlay.
surface_names = sorted({m.decode('ascii').lower() for m in re.findall(
    rb'(?i)(?:concrete|dev|glass|metal|plastic|tile)/[A-Za-z0-9_./-]+', bsp)})
map_surface_names = sorted({m.decode('ascii').lower() for m in re.findall(
    rb'(?i)maps/[A-Za-z0-9_.-]+/(?:concrete|dev|glass|metal|plastic|tile)/[A-Za-z0-9_./+-]+', bsp)})
pak_sources = [vpk.open(os.environ['PAK_PATH'])]
base_pak_path = os.environ['BASE_PAK_PATH']
if os.path.isfile(base_pak_path):
    pak_sources.append(vpk.open(base_pak_path))
out = os.environ['OUT_DIR']
pak_files = {}
for source in pak_sources:
    for key in source:
        pak_files.setdefault(key.lower(), (source, key))
keys = list(pak_files)
written = 0
families = 0
runtime_model_prefixes = (
    'models/gibs/',
    'models/anim_wp/',
    'models/props_debris/',
    'models/weapons/',
    'models/player',
    'models/portals/',
    'models/items/',
)
for key in keys:
    # Physics debris, weapon and player models are spawned at runtime and are
    # not always present in the BSP entity lump.
    if any(key.startswith(prefix) for prefix in runtime_model_prefixes) or (key.startswith('models/') and key.count('/') == 1):
        source, original_key = pak_files[key]
        path = os.path.join(out, original_key)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'wb') as stream:
            stream.write(source[original_key].read())
        written += 1
material_prefixes = (
    'materials/vgui/ico_friend_indicator_alone',
    'materials/vgui/ico_friend_indicator_scoreboard',
    'materials/sprites/hud/portal_crosshairs',
    'materials/vgui/touch/',
    'materials/vgui/hud/icon_commentary',
    'materials/console/background_menu',
    'materials/console/startup_loading',
    'materials/vgui/white_additive',
    'materials/vgui/zoom',
    'materials/vgui/white',
    'materials/decals/simpleshadow',
    'materials/debug/debugtranslucentsinglecolor',
    'materials/engine/modulatesinglecolor',
    'materials/engine/writez',
)
for key in keys:
    if not any(key.startswith(prefix) for prefix in material_prefixes):
        continue
    source, original_key = pak_files[key]
    path = os.path.join(out, original_key)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as stream:
        stream.write(source[original_key].read())
    written += 1

def write_key(key):
    """Write one archive entry, preserving its canonical VPK path."""
    nonlocal_written = False
    pair = pak_files.get(key.lower())
    if pair is None:
        return False
    source, original_key = pair
    path = os.path.join(out, original_key)
    if os.path.exists(path):
        return False
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as stream:
        stream.write(source[original_key].read())
    return True

# Extract each surface material and recursively stage texture references from
# its VMT (basetexture, bumpmap, detail and envmapmask).  This keeps the
# overlay small while making world geometry renderable.
pending = []
for name in surface_names:
    if name.startswith(('tools/', 'models/')):
        continue
    # Strings from the BSP may include a compiled extension or a map-specific
    # lightmap suffix.  Only enqueue candidates that actually exist in a VPK;
    # this avoids producing names such as *.vmt.vmt.
    name = re.sub(r'\.(?:vmt|vtf|vmtpk)$', '', name, flags=re.I)
    for ext in ('.vmt', '.vtf'):
        candidate = 'materials/' + name + ext
        if candidate in pak_files:
            pending.append(candidate)
seen = set()
while pending:
    key = pending.pop()
    key = key.lower()
    if key in seen:
        continue
    seen.add(key)
    if write_key(key):
        written += 1
    pair = pak_files.get(key)
    if pair is None or not key.endswith('.vmt'):
        continue
    source, original_key = pair
    try:
        text = source[original_key].read().decode('utf-8', 'ignore')
    except Exception:
        continue
    for ref in re.findall(r'\$(?:basetexture|bumpmap|normalmap|detail|envmapmask)\s+"?([^"\s]+)', text, re.I):
        ref = ref.replace('\\', '/').lower()
        if ref.startswith(('http://', 'https://')):
            continue
        if ref.startswith('materials/'):
            ref = ref[10:]
        pending.extend(('materials/' + ref + '.vtf', 'materials/' + ref + '.vmt'))

# BSP material names retain the capitalization used by the original Windows
# build (for example materials/PLASTIC/PLASTICWALL001A.vmt).  On a
# case-sensitive APFS volume the include lookup is otherwise different from
# the lower-case VPK key.  Add small loose aliases for the exact spelling.
for name in surface_names:
    base = re.sub(r'\.(?:vmt|vtf|vmtpk)$', '', name, flags=re.I)
    lower_base = os.path.join(out, 'materials', base)
    upper_base = os.path.join(out, 'materials', base.upper())
    for ext in ('.vmt', '.vtf'):
        source_path = lower_base + ext
        alias_path = upper_base + ext
        if os.path.isfile(source_path) and not os.path.exists(alias_path):
            os.makedirs(os.path.dirname(alias_path), exist_ok=True)
            with open(source_path, 'rb') as src, open(alias_path, 'wb') as dst:
                dst.write(src.read())
            written += 1

# Portal 2's BSP contains generated per-map patch materials.  This older
# branch recognises the material path but cannot apply the patch shader, which
# turns every face into the magenta error material.  Emit a conservative
# UnlitGeneric replacement in the overlay, retaining the original texture.
for material in map_surface_names:
    material = re.sub(r'\.(?:vmt|vtf|vmtpk)$', '', material, flags=re.I)
    match = re.match(r'(maps/[^/]+/)(concrete|dev|glass|metal|plastic|tile)/(.+)$', material, re.I)
    if not match:
        continue
    stem = re.sub(r'_[+-]?\d+_[+-]?\d+_[+-]?\d+$', '', match.group(3))
    texture = match.group(2).lower() + '/' + stem
    # Only use the fallback where its source texture is present in the VPK or
    # already staged; otherwise leave normal material resolution untouched.
    if ('materials/' + texture + '.vtf') not in pak_files and not os.path.isfile(os.path.join(out, 'materials', texture + '.vtf')):
        continue
    path = os.path.join(out, 'materials', material + '.vmt')
    if os.path.exists(path):
        continue
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as stream:
        stream.write('UnlitGeneric\\n{\\n    $basetexture "' + texture + '"\\n    $vertexcolor 0\\n    $nolod 1\\n}\\n')
    written += 1
for model in models:
    base = model[:-4]
    matches = [key for key in keys if key == model or key.startswith(base + '.')]
    if not matches:
        continue
    families += 1
    for key in matches:
        source, original_key = pak_files[key]
        path = os.path.join(out, original_key)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'wb') as stream:
            stream.write(source[original_key].read())
        written += 1
print(f'extracted {written} files for {families}/{len(models)} model families into {out}')
PY

echo "Prepared Portal 2 map: $MAP_NAME"
echo "Run: cd \"$STAGE_DIR\" && ./hl2_osx -game portal2 -windowed -w 1280 -h 720 -novid -condebug +map $MAP_NAME"
