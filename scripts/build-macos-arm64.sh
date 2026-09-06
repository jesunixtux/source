#!/bin/sh

set -eu

usage() {
	cat <<'EOF'
Usage: ./scripts/build-macos-arm64.sh [portal|hl2|l4d|stanley] [waf configure options]

Environment variables:
  SOURCE_GAME  Game to build when no positional game is supplied (default: portal)
  BUILD_TYPE   debug, release or none (default: debug)

Examples:
  ./scripts/build-macos-arm64.sh portal
  ./scripts/build-macos-arm64.sh hl2
  ./scripts/build-macos-arm64.sh l4d       # experimental: Orange Box engine/game ABI
  ./scripts/build-macos-arm64.sh stanley   # experimental: Portal ABI for Stanley content
  BUILD_TYPE=release ./scripts/build-macos-arm64.sh portal
EOF
}

# Homebrew arm64 (native, fully supported on Apple Silicon)
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
export HOMEBREW_PREFIX="/opt/homebrew"

# pkg-config search path: include keg-only libraries (jpeg, openal-soft)
export PKG_CONFIG_PATH="/opt/homebrew/opt/jpeg/lib/pkgconfig:/opt/homebrew/opt/openal-soft/lib/pkgconfig:${PKG_CONFIG_PATH:-}"

# Portal and Half-Life 2 have different client/server modules. Reconfigure
# before switching games so an old module cannot be deployed by mistake.
SOURCE_GAME="${SOURCE_GAME:-portal}"
BUILD_TYPE="${BUILD_TYPE:-debug}"
REQUESTED_GAME="$SOURCE_GAME"

case "${1:-}" in
	portal|hl2|l4d|stanley)
		SOURCE_GAME="$1"
		REQUESTED_GAME="$1"
		shift
		;;
	-h|--help)
		usage
		exit 0
		;;
esac

case "$SOURCE_GAME" in
	portal|hl2) ;;
	stanley)
		# Stanley's public game binaries are i386; use the closest available
		# Portal ABI while keeping the target explicitly experimental.
		SOURCE_GAME="portal"
		;;
	l4d)
	# L4D has an isolated compatibility target. Its current VPC sources are an
	# Orange Box shell until native L4D sources are integrated.
		;;
	*)
		echo "ERROR: unsupported game '$SOURCE_GAME'. Tested values: portal, hl2, l4d, stanley (experimental)." >&2
		usage >&2
		exit 2
		;;
esac

case "$BUILD_TYPE" in
	debug|release|none) ;;
	*)
		echo "ERROR: invalid BUILD_TYPE '$BUILD_TYPE' (debug, release or none)." >&2
		exit 2
		;;
esac

if [ "$REQUESTED_GAME" = l4d ]; then
	echo "WARN: Left 4 Dead sources are not present; building the HL2/Orange Box ABI for experimental L4D rendering."
fi
if [ "$REQUESTED_GAME" = stanley ]; then
	echo "WARN: Stanley game modules are i386; building the Portal-compatible ARM64 shell for resource testing."
fi
echo "==> Configuring macOS arm64 build for $REQUESTED_GAME (WAF target: $SOURCE_GAME, $BUILD_TYPE)"
python3 ./waf configure -T "$BUILD_TYPE" --disable-warns --build-games="$SOURCE_GAME" "$@"
python3 ./waf build
