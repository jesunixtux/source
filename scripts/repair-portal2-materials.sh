#!/bin/sh

# Extract only the material/texture assets reported missing by the last run.
# The Steam installation and its VPK files are never modified.

set -eu

USER_HOME_DIR=${HOME:?}
P2_DIR=${PORTAL2_DIR:-"$USER_HOME_DIR/Library/Application Support/Steam/steamapps/common/Portal 2"}
PORTAL_DIR=${PORTAL_DIR:-"$USER_HOME_DIR/Library/Application Support/Steam/steamapps/common/Portal"}
STAGE_DIR=${PORTAL2_STAGE_DIR:-"$P2_DIR/portal2_arm64_test"}
LOG_FILE=${PORTAL2_LOG:-"$STAGE_DIR/bin/console.log"}
PAK_FILE=${PORTAL2_PAK:-"$P2_DIR/portal2/pak01_dir.vpk"}
BASE_PAK=${PORTAL2_BASE_PAK:-"$PORTAL_DIR/hl2/hl2_misc_dir.vpk"}

[ -f "$LOG_FILE" ] || { echo "ERROR: log not found: $LOG_FILE" >&2; exit 2; }
[ -f "$PAK_FILE" ] || { echo "ERROR: Portal 2 VPK not found: $PAK_FILE" >&2; exit 2; }
[ -d "$STAGE_DIR/portal2_override" ] || { echo "ERROR: stage not found: $STAGE_DIR" >&2; exit 2; }

LOG_FILE="$LOG_FILE" PAK_FILE="$PAK_FILE" BASE_PAK="$BASE_PAK" OUT_DIR="$STAGE_DIR/portal2_override" \
python3 - <<'PY'
import os
import re
import vpk

log = open(os.environ["LOG_FILE"], errors="replace").read().lower()
patterns = (
    r"couldn.t find materials/([^\s\"']+)",
    r"materials/([^\s\"']+\.(?:vmt|vtf))",
    r"load sprite material materials/([^\s\"']+)",
)
wanted = set()
for pattern in patterns:
    for match in re.findall(pattern, log):
        value = match if isinstance(match, str) else match[0]
        value = "materials/" + value.lstrip("/").replace("\\", "/")
        if value.endswith((".vmt", ".vtf")):
            wanted.add(value)

archives = [vpk.open(os.environ["PAK_FILE"])]
if os.path.isfile(os.environ["BASE_PAK"]):
    archives.append(vpk.open(os.environ["BASE_PAK"]))
out = os.environ["OUT_DIR"]
written = 0
for key in sorted(wanted):
    for archive in archives:
        if key not in archive:
            continue
        destination = os.path.join(out, key)
        os.makedirs(os.path.dirname(destination), exist_ok=True)
        if not os.path.exists(destination):
            with open(destination, "wb") as stream:
                stream.write(archive[key].read())
            written += 1
        break
print(f"extracted {written} missing material files into {out}")
PY
