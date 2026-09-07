#!/bin/sh

# Smoke test for the isolated ARM64 Portal 2 staging tree.  It starts the
# launcher, lets the UI initialize, then exits through the engine console.

set -eu

USER_HOME_DIR=${HOME:?}
P2_DIR=${PORTAL2_DIR:-"$USER_HOME_DIR/Library/Application Support/Steam/steamapps/common/Portal 2"}
STAGE_DIR=${PORTAL2_STAGE_DIR:-"$P2_DIR/portal2_arm64_test"}

[ -x "$STAGE_DIR/hl2_osx" ] || {
	echo "ERROR: staging launcher not found: $STAGE_DIR/hl2_osx" >&2
	echo "Run scripts/stage-macos-portal2.sh first." >&2
	exit 2
}

STAGE_DIR="$STAGE_DIR" python3 - <<'PY'
import os
import pathlib
import signal
import subprocess

stage = pathlib.Path(os.environ["STAGE_DIR"])
command = [
    "./hl2_osx", "-game", "portal2", "-novid", "-windowed",
    "-w", "640", "-h", "480", "-condebug", "+quit",
]
process = subprocess.Popen(
    command, cwd=stage, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    text=True, start_new_session=True,
)
try:
    output, _ = process.communicate(timeout=15)
except subprocess.TimeoutExpired as error:
    # The engine was asked to quit. A timeout is a hang, not success.
    os.killpg(process.pid, signal.SIGTERM)
    output = (error.output or "") if isinstance(error.output, str) else (error.output or b"").decode(errors="replace")
    try:
        tail, _ = process.communicate(timeout=3)
        output += tail or ""
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGKILL)
        tail, _ = process.communicate()
        output += tail or ""
    print("SMOKE FAIL: launcher did not honor +quit within 15 seconds")
    raise SystemExit(1)

if process.returncode != 0:
    print(f"SMOKE FAIL: launcher exited with {process.returncode}")
    print("\n".join(output.splitlines()[-20:]))
    raise SystemExit(1)
print("SMOKE PASS: launcher initialized and exited cleanly")
PY
