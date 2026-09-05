#!/bin/sh

set -e

# Homebrew arm64 (native, fully supported on Apple Silicon)
export PATH="/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"
export HOMEBREW_PREFIX="/opt/homebrew"

# pkg-config search path: include keg-only libraries (jpeg, openal-soft)
export PKG_CONFIG_PATH="/opt/homebrew/opt/jpeg/lib/pkgconfig:/opt/homebrew/opt/openal-soft/lib/pkgconfig:$PKG_CONFIG_PATH"

# This script builds the Source engine natively for arm64 (Apple Silicon).
# Portal is the safe default because its client/server modules are not
# interchangeable with the Half-Life 2 modules. Override with SOURCE_GAME=hl2
# (or pass a later --build-games option) when building another game.
SOURCE_GAME="${SOURCE_GAME:-portal}"
python3 ./waf configure -T debug --disable-warns --build-games="$SOURCE_GAME" "$@" &&
python3 ./waf build
