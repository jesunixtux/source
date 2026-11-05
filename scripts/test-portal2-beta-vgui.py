#!/usr/bin/env python3
"""Bounded beta-resource menu test; the screenshot still requires visual review."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import signal
import subprocess
import os

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('stage', type=Path)
args = parser.parse_args()
stage = args.stage.resolve()
resource = stage / 'portal2_override/resource'
origins = resource / 'VGUI_ORIGIN.txt'
if not origins.is_file() or not (stage / 'Menu-Portal2-Beta.command').is_file():
    parser.error('Expected a beta VGUI stage created by stage-macos-portal2-beta.py')
for line in origins.read_text().splitlines():
    digest, name, original = line.split('  ', 2)
    if any(hashlib.sha256(path.read_bytes()).hexdigest() != digest for path in
           (resource / name.lower(), Path(original))):
        parser.error(f'Original beta resource mismatch: {name}')

out = stage / 'diagnostics' / datetime.datetime.now().strftime('%Y%m%d-%H%M%S-vgui')
out.mkdir(parents=True)
if (stage / 'engine.log').exists():
    (stage / 'engine.log').rename(out / 'previous-engine.log')
helper = out / 'window-id'
subprocess.run(['clang', '-fobjc-arc', '-framework', 'Foundation', '-framework', 'CoreGraphics',
                str(Path(__file__).with_name('source-window-id.m')), '-o', str(helper)], check=True)
report = {'beta_resources_unchanged': True, 'alive_after_20s': False, 'screenshot': False}
with (out / 'stdout.log').open('w') as log:
    process = subprocess.Popen(['./Menu-Portal2-Beta.command'], cwd=stage,
                               stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    try:
        report['early_exit'] = process.wait(timeout=20)
    except subprocess.TimeoutExpired:
        report['alive_after_20s'] = True
        try:
            window = subprocess.check_output([str(helper), str(process.pid)], text=True, timeout=5).strip()
            subprocess.run(['screencapture', '-x', '-o', '-l', window, str(out / 'menu.png')], check=True, timeout=8)
            report['screenshot'] = True
        except (OSError, subprocess.SubprocessError) as error:
            report['capture_error'] = str(error)
    finally:
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=4)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=4)
for relative in ('engine.log', 'portal2_override/console.log'):
    path = stage / relative
    if path.is_file():
        shutil.copy2(path, out / path.name)
text = (out / 'engine.log').read_text(errors='replace') if (out / 'engine.log').exists() else ''
report['engine_log_present'] = bool(text)
report['keyvalues_errors'] = text.count('KeyValues Error:')
report['missing_background'] = 'Missing Vgui material console/background_menu' in text
passed = (report['alive_after_20s'] and report['screenshot'] and report['engine_log_present'] and
          report['keyvalues_errors'] == 0 and not report['missing_background'])
(out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
print(f'Artifacts: {out}')
print('RESOURCE LOAD ' + ('PASS' if passed else 'FAIL') + '; inspect menu.png. This is not a BaseModUI interaction test.')
raise SystemExit(0 if passed else 1)
