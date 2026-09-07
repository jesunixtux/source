#!/bin/sh

# Compile a small, non-linking probe of the private Portal 2 snapshot against
# the public Source tree.  This deliberately does not copy private sources into
# the repository or replace any Steam installation.

set -eu

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
SDK_ROOT=${PORTAL2_PRIVATE_SDK:-"$HOME/Downloads/source-sdk-portal2-private-arm64"}
BUILD_DB="$ROOT/build/compile_commands.json"
OUT_DIR=${TMPDIR:-/tmp}/portal2-private-arm64-probe

if [ ! -d "$SDK_ROOT" ]; then
	echo "ERROR: private SDK not found: $SDK_ROOT" >&2
	echo "Set PORTAL2_PRIVATE_SDK to the local, licensed snapshot." >&2
	exit 2
fi
if [ ! -f "$BUILD_DB" ]; then
	echo "ERROR: $BUILD_DB is missing; configure the ARM64 build first." >&2
	exit 2
fi

mkdir -p "$OUT_DIR"
ROOT="$ROOT" SDK_ROOT="$SDK_ROOT" BUILD_DB="$BUILD_DB" OUT_DIR="$OUT_DIR" \
python3 - <<'PY'
import json
import os
import pathlib
import subprocess
import sys

root = pathlib.Path(os.environ["ROOT"])
sdk = pathlib.Path(os.environ["SDK_ROOT"])
db_path = pathlib.Path(os.environ["BUILD_DB"])
out = pathlib.Path(os.environ["OUT_DIR"])

with db_path.open() as f:
    commands = json.load(f)

names = [
    "prop_button.cpp",
    "prop_floor_button.cpp",
    "prop_linked_portal_door.cpp",
    "prop_testchamber_door.cpp",
    "prop_weightedcube.cpp",
]

def entry_for(name):
    suffix = "/portal2/" + name
    return next((e for e in commands if e["file"].endswith(suffix)), None)

passed = failed = skipped = 0
for name in names:
    source = sdk / "game/server/portal2" / name
    if not source.exists():
        print(f"SKIP {name}: not present in private snapshot")
        skipped += 1
        continue
    entry = entry_for(name)
    if entry is None:
        print(f"SKIP {name}: no matching ARM64 compile command")
        skipped += 1
        continue

    args = list(entry["arguments"])
    source_index = next(i for i, arg in enumerate(args) if arg.endswith("/" + name))
    args[source_index] = str(source)
    output_index = next(i for i, arg in enumerate(args) if arg.startswith("-o"))
    args[output_index] = "-o" + str(out / (name + ".o"))
    private_includes = [
        "-I" + str(sdk / "game/server/portal"),
        "-I" + str(sdk / "game/server/portal2"),
        "-I" + str(sdk / "game/shared/portal"),
        "-I" + str(sdk / "game/shared/portal2"),
    ]
    args[source_index:source_index] = private_includes
    result = subprocess.run(args, cwd=entry["directory"], text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode == 0:
        print(f"PASS {name}")
        passed += 1
    else:
        print(f"FAIL {name}")
        lines = result.stdout.strip().splitlines()
        for line in lines[-8:]:
            print("  " + line)
        failed += 1

print(f"Result: {passed} passed, {failed} failed, {skipped} skipped")
sys.exit(1 if failed else 0)
PY
