#!/bin/sh
# Link the regression against the same KeyValues library used by the engine.
set -eu
SOURCE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$SOURCE_DIR"
TEST_DIR=$(mktemp -d "${TMPDIR:-/tmp}/source-keyvalues.XXXXXX")
trap 'rm -f "$TEST_DIR/test"; rmdir "$TEST_DIR"' EXIT HUP INT TERM
clang++ -std=c++11 -DPOSIX=1 -DOSX=1 -D_OSX=1 -DGNUC -DPLATFORM_64BITS=1 \
  -Ipublic -Ipublic/tier0 -Ipublic/tier1 scripts/tests-keyvalues-condition.cpp \
  build/tier1/libtier1.a build/tier0/libtier0.dylib build/vstdlib/libvstdlib.dylib \
  -liconv -o "$TEST_DIR/test"
"$TEST_DIR/test" "$@"
