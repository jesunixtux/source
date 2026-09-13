#!/usr/bin/env python3
"""Stage a Source-2007-compatible Portal 2 sound manifest.

Portal 2's shipped manifest has a PS3-era marker entry used as a comment.  The
older sound emitter in this branch treats that entry as a malformed file type
and stops reporting a clean manifest load.  The actual scripts and wave files
remain in the original Portal 2 search path; this only supplies a sanitized
loose manifest earlier in the isolated experiment's search order.
"""
import argparse
from pathlib import Path
import re


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("game_dir", type=Path)
parser.add_argument("overlay", type=Path)
args = parser.parse_args()

source = args.game_dir / "scripts/game_sounds_manifest.txt"
if not source.is_file():
    parser.error(f"Missing Portal 2 sound manifest: {source}")

text = source.read_text(encoding="cp1252")
clean, removed = re.subn(
    r'^\s*"new_sound_scripts_must_go_below_here"\s+""\s*(?://.*)?\r?\n',
    "",
    text,
    flags=re.MULTILINE,
)
if removed != 1:
    parser.error(f"Expected one Portal 2 manifest marker, found {removed}")

scripts = re.findall(r'"precache_file"\s+"(scripts/[^"\r\n]+\.txt)"', clean, re.I)
if not scripts:
    parser.error("Sanitized Portal 2 manifest contains no sound scripts")
missing = [name for name in scripts if not (args.game_dir / name).is_file()]
if missing:
    parser.error("Manifest references missing sound scripts: " + ", ".join(missing))

target = args.overlay / "scripts/game_sounds_manifest.txt"
target.parent.mkdir(parents=True, exist_ok=True)
target.write_text(clean, encoding="cp1252")
(args.overlay / "SOUND_MANIFEST_ORIGIN.txt").write_text(
    "Portal 2 game_sounds_manifest.txt with only the unsupported PS3 marker removed.\n"
    f"{len(scripts)} Portal 2 sound scripts remain mounted from the original game.\n"
)
print(f"Portal 2 sound manifest: {len(scripts)} scripts -> {target}")
