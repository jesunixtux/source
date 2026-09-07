#!/bin/bash
set -euo pipefail

# Isolated L4D Lite tree: ARM64 HL2 gameplay with L4D resources.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$SOURCE_DIR/build}"
USER_HOME_DIR="$(dscl . -read "/Users/$(id -un)" NFSHomeDirectory | awk '{print $2}')"
L4D_DIR="${L4D_DIR:-$USER_HOME_DIR/Library/Application Support/Steam/steamapps/common/left 4 dead}"
STAGE_DIR="${L4D_LITE_DIR:-$L4D_DIR/l4d_lite}"

[ -d "$L4D_DIR/left4dead" ] || { echo "ERROR: L4D content not found: $L4D_DIR" >&2; exit 1; }
[ -f "$BUILD_DIR/launcher_main/hl2_launcher" ] || { echo "ERROR: build l4d_lite first" >&2; exit 1; }
[ ! -e "$STAGE_DIR" ] || { echo "ERROR: stage already exists: $STAGE_DIR" >&2; exit 1; }

mkdir -p "$STAGE_DIR/bin" "$STAGE_DIR/l4d_lite"
cp -f "$BUILD_DIR/launcher_main/hl2_launcher" "$STAGE_DIR/hl2_osx"
chmod +x "$STAGE_DIR/hl2_osx"
ln -s "$L4D_DIR/hl2" "$STAGE_DIR/hl2"
ln -s "$L4D_DIR/left4dead" "$STAGE_DIR/left4dead"
while IFS= read -r f; do
	base="$(basename "$f")"
	ln -s "$f" "$STAGE_DIR/bin/$base"
done < <(find "$BUILD_DIR" -type f -name '*.dylib' -print)

# Link all game resources without copying or altering the Steam installation.
for item in "$L4D_DIR/left4dead"/*; do
	base="$(basename "$item")"
	[ "$base" = bin ] || [ "$base" = gameinfo.txt ] && continue
	ln -s "$item" "$STAGE_DIR/l4d_lite/$base"
done

cat > "$STAGE_DIR/l4d_lite/gameinfo.txt" <<'EOF'
"GameInfo"
{
    game "Left 4 Dead Lite"
    title "Left 4 Dead Lite"
    type singleplayer_only
    FileSystem
    {
        SteamAppId 500
        SearchPaths
        {
            Game |gameinfo_path|.
            Game left4dead
            Game hl2
        }
    }
}
EOF
printf '500\n' > "$STAGE_DIR/steam_appid.txt"
echo "L4D Lite staged at: $STAGE_DIR"
echo "Run: cd \"$STAGE_DIR\" && ./hl2_osx -game l4d_lite -windowed -w 1280 -h 720 -novid -condebug +map l4d_hospital01_apartment"
