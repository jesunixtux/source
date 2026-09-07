#!/usr/bin/env python3
"""Bounded map-load test; a live process is NOT proof of correct rendering."""
import argparse
import datetime
import os
from pathlib import Path
import shutil
import signal
import subprocess
import time

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('stage', type=Path)
p.add_argument('--map', default='sp_a1_intro1')
p.add_argument('--seconds', type=int, default=20)
p.add_argument('--screenshot', action='store_true')
p.add_argument('--no-prop-lighting', action='store_true')
p.add_argument('--pause-menu', action='store_true', help='open the experimental Portal 2 pause menu after loading the map')
p.add_argument('--gameplay', action='store_true', help='run the opt-in gun/portal integration test (use --seconds 60)')
p.add_argument('--intro-scenes', action='store_true', help='test the vault dialogue chain from its real map trigger (use --seconds 75)')
args = p.parse_args()
if args.gameplay and args.intro_scenes:
    p.error('Choose --gameplay or --intro-scenes, not both')
if not 10 <= args.seconds <= 120:
    p.error('--seconds must be between 10 and 120')
stage = args.stage.resolve()
if not (stage / 'hl2_osx').is_file():
    p.error('Missing staging executable')
out = stage / 'diagnostics' / datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
out.mkdir(parents=True)
if args.screenshot:
    subprocess.run(['clang', '-fobjc-arc', '-framework', 'Foundation', '-framework', 'CoreGraphics',
                    str(Path(__file__).with_name('source-window-id.m')), '-o', str(out / 'window-id')], check=True)
command = ['./hl2_osx', '-game', 'portal2', '-novid', '-nosoundcachewrite', '-nomouse', '-windowed', '-w', '800', '-h', '600',
           '+sv_cheats', '1', '+mat_fullbright', '0', '+mat_disable_bloom', '1',
           '+mat_colorcorrection', '0']
if args.pause_menu:
    command += ['-portal2_pausemenu']
if args.gameplay:
    command += ['-portal2_gameplay_test']
if args.intro_scenes:
    command += ['-portal2_intro_scene_test']
if args.no_prop_lighting:
    command += ['+r_proplightingfromdisk', '0']
command += ['+map', args.map]
with (out / 'stdout.log').open('w') as log:
    process = subprocess.Popen(command, cwd=stage, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    alive = False
    try:
        deadline = time.monotonic() + args.seconds
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(0.25)
        alive = process.poll() is None
        if alive and args.screenshot:
            try:
                window = subprocess.check_output([str(out / 'window-id'), str(process.pid)], text=True).strip()
                subprocess.run(['screencapture', '-x', '-o', '-l', window, str(out / 'map.png')], check=True, timeout=10)
            except (subprocess.SubprocessError, OSError) as error:
                print(f'Screenshot unavailable (window hidden or capture failed): {error}')
    finally:
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=4)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait(timeout=4)
        if (stage / 'engine.log').is_file():
            shutil.copy2(stage / 'engine.log', out / 'engine.log')
text = (out / 'engine.log').read_text(errors='replace') if (out / 'engine.log').exists() else ''
activated = 'SV_ActivateServer: setting tickrate' in text
bad_shader = any(s in text for s in ("Couldn't load combo", 'Invalid VCS', 'Invalid dynamic shader', 'Using invalid shader combo'))
passed = alive and activated and not bad_shader
if args.gameplay:
    passed = passed and all(f'PORTAL2_GAMEPLAY_TEST {check}=PASS' in text
                           for check in ('output_formats', 'physics_filter', 'reentrant_cancel', 'placement', 'crossing',
                                         'cube_pickup', 'cube_button_opens_door', 'cube_button_closes_door'))
if args.intro_scenes:
    passed = passed and all(f'PORTAL2_INTRO scene completed PreHub01RelaxationVaultIntro0{i}' in text for i in range(1, 5))
    passed = passed and not any('Failed to load sound "vo/announcer/' in line for line in text.splitlines())
label = 'GAMEPLAY' if args.gameplay else ('INTRO SCENES' if args.intro_scenes else 'MAP LOAD')
print(f'{label} {"PASS" if passed else "FAIL"}: alive={alive}, server_active={activated}, shader_index_error={bad_shader}')
print(f'Artifacts: {out}')
print('Tests are bounded fixtures, not a full campaign playthrough. Gameplay checks gun, traversal and cube/button/door I/O; intro-scenes checks the vault dialogue chain. Inspect map.png for visual quality.')
raise SystemExit(0 if passed else 1)
