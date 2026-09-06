#!/bin/bash
set -euo pipefail

# Build an isolated ARM64 experiment using The Stanley Parable resources.
# The Steam installation is never modified; the game DLLs are replaced in the
# staging tree with the already-built Portal ARM64 modules for ABI testing.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$SOURCE_DIR/build}"
USER_HOME_DIR="$(dscl . -read "/Users/$(id -un)" NFSHomeDirectory | awk '{print $2}')"
GAME_DIR="${STANLEY_DIR:-$USER_HOME_DIR/Library/Application Support/Steam/steamapps/common/The Stanley Parable}"
PORTAL_DIR="${PORTAL_DIR:-$USER_HOME_DIR/Library/Application Support/Steam/steamapps/common/Portal}"
STAGE_DIR="${STANLEY_STAGE_DIR:-$GAME_DIR/stanley_arm64_test}"

[ -d "$GAME_DIR/thestanleyparable" ] || { echo "ERROR: Stanley content not found: $GAME_DIR" >&2; exit 1; }
[ -x "$PORTAL_DIR/hl2_osx" ] || { echo "ERROR: ARM64 Portal launcher not found: $PORTAL_DIR/hl2_osx" >&2; exit 1; }
[ -f "$PORTAL_DIR/bin/libengine.dylib" ] || { echo "ERROR: deploy Portal ARM64 first" >&2; exit 1; }
[ ! -e "$STAGE_DIR" ] || { echo "ERROR: stage already exists: $STAGE_DIR (remove it manually to recreate)" >&2; exit 1; }

mkdir -p "$STAGE_DIR"
cp -f "$PORTAL_DIR/hl2_osx" "$STAGE_DIR/hl2_osx"
chmod +x "$STAGE_DIR/hl2_osx"
ln -s "$PORTAL_DIR/bin" "$STAGE_DIR/bin"
ln -s "$PORTAL_DIR/platform" "$STAGE_DIR/platform"

# Link Stanley resources, but provide ARM64 Portal game modules under bin.
for item in "$GAME_DIR/thestanleyparable"/*; do
	base="$(basename "$item")"
	[ "$base" = bin ] && continue
	ln -s "$item" "$STAGE_DIR/$base"
done
mkdir -p "$STAGE_DIR/thestanleyparable"
for item in "$GAME_DIR/thestanleyparable"/*; do
	base="$(basename "$item")"
	[ "$base" = bin ] && continue
	ln -s "$item" "$STAGE_DIR/thestanleyparable/$base"
done
ln -s "$PORTAL_DIR/portal/bin" "$STAGE_DIR/thestanleyparable/bin"

# The Portal-compatible shell expects this legacy-cased menu resource. Stanley
# ships only gameui_*.txt, so borrow the neutral Portal menu for the smoke test.
mkdir -p "$STAGE_DIR/thestanleyparable/resource"
ln -s "$PORTAL_DIR/portal/resource/gamemenu.res" \
	"$STAGE_DIR/thestanleyparable/resource/GameMenu.res"

printf '221910\n' > "$STAGE_DIR/steam_appid.txt"
echo "Staged ARM64 Stanley test tree: $STAGE_DIR"
echo "Run: cd \"$STAGE_DIR\" && ./hl2_osx -game thestanleyparable -windowed -w 1280 -h 720 -novid -condebug"
