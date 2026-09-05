#!/bin/sh

set -e

git submodule init && git submodule update

# SDL2 is required for the client build. On Apple Silicon the framework
# is installed as a universal binary by Homebrew.
if ! command -v brew >/dev/null 2>&1; then
	echo "Homebrew is required. See https://brew.sh" >&2
	exit 1
fi
brew install sdl2 freetype fontconfig libjpeg libpng libcurl 2>/dev/null || brew install sdl2

# This script targets 64-bit Intel (x86_64). On Apple Silicon this requires
# Rosetta 2 and the x86_64 macOS SDK. The native compiler defaults to arm64,
# so --arch=x86_64 is passed explicitly to force the Intel toolchain.
./waf configure -T debug --disable-warns --arch=x86_64 $* &&
./waf build
