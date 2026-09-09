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
p.add_argument('--portal-texture', action='store_true', help='compare the render-to-texture portal path against stencil rendering')
p.add_argument('--portal-mask-debug', action='store_true', help='diagnostic magenta stencil fill, not a visual acceptance test')
p.add_argument('--pause-menu', action='store_true', help='open the experimental Portal 2 pause menu after loading the map')
p.add_argument('--gameplay', action='store_true', help='run the opt-in gun/portal integration test (use --seconds 90 for a cold launch)')
p.add_argument('--intro-scenes', action='store_true', help='test the vault dialogue chain from its real map trigger (use --seconds 75)')
p.add_argument('--elevator-transition', action='store_true', help='exercise intro1\'s authored elevator I/O and map transition (use --seconds 30)')
p.add_argument('--bink', action='store_true', help='open a Portal 2 Bink video through the ARM64 FFmpeg module')
args = p.parse_args()
if sum((args.gameplay, args.intro_scenes, args.elevator_transition)) > 1:
    p.error('Choose only one opt-in integration test')
if not 10 <= args.seconds <= 120:
    p.error('--seconds must be between 10 and 120')
stage = args.stage.resolve()
if not (stage / 'hl2_osx').is_file():
    p.error('Missing staging executable')
out = stage / 'diagnostics' / datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
out.mkdir(parents=True)
# A previous run's markers must never satisfy this run's assertions or trigger
# its screenshots before the engine has opened its own log. Preserve the log.
if (stage / 'engine.log').is_file():
    (stage / 'engine.log').rename(out / 'previous-engine.log')
if args.screenshot:
    subprocess.run(['clang', '-fobjc-arc', '-framework', 'Foundation', '-framework', 'CoreGraphics',
                    str(Path(__file__).with_name('source-window-id.m')), '-o', str(out / 'window-id')], check=True)
command = ['./hl2_osx', '-game', 'portal2', '-novid', '-nosoundcachewrite', '-nomouse', '-windowed', '-w', '800', '-h', '600',
           '+sv_cheats', '1', '+mat_fullbright', '0', '+mat_disable_bloom', '1',
           '+mat_colorcorrection', '0']
if args.pause_menu:
    command += ['-portal2_pausemenu']
if args.gameplay:
    command += ['-portal2_gameplay_test', '+developer', '1']
if args.intro_scenes:
    command += ['-portal2_intro_scene_test']
if args.elevator_transition:
    command += ['-portal2_elevator_transition_test']
if args.no_prop_lighting:
    command += ['+r_proplightingfromdisk', '0']
if args.portal_texture:
    command += ['+r_portal_use_stencils', '0']
if args.portal_mask_debug:
    command += ['+portal2_debug_stencil_mask', '1']
command += ['+map', args.map]
if args.gameplay:
    command += ['+mat_hdr_level', '+mat_info', '+mat_fullbright', '+mat_phong']
if args.bink:
    command += ['+playvideo', 'attract01.bik']

def capture(name, pid):
    try:
        window = subprocess.check_output([str(out / 'window-id'), str(pid)], text=True).strip()
        subprocess.run(['screencapture', '-x', '-o', '-l', window, str(out / name)], check=True, timeout=10)
    except (subprocess.SubprocessError, OSError) as error:
        print(f'Screenshot {name} unavailable (window hidden or capture failed): {error}')

with (out / 'stdout.log').open('w') as log:
    process = subprocess.Popen(command, cwd=stage, stdout=log, stderr=subprocess.STDOUT, start_new_session=True)
    alive = False
    try:
        deadline = time.monotonic() + args.seconds
        # Capture the actual test phases, not only the final wall after traversal.
        phases = {'attack=blue': 'gun-blue.png', 'attack=orange': 'gun-orange.png',
                  'cube_pickup=PASS': 'gun-carrying.png',
                  'zoom_in=PASS': 'gun-zoom.png', 'player_model=PASS': 'player-model.png'} if args.gameplay and args.screenshot else {}
        pending_captures = {}
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(0.25)
            if phases and (stage / 'engine.log').is_file():
                progress = (stage / 'engine.log').read_text(errors='replace')
                for marker, name in list(phases.items()):
                    if f'PORTAL2_GAMEPLAY_TEST {marker}' in progress:
                        pending_captures[name] = time.monotonic() + (2.0 if name == 'gun-carrying.png' else 0.6)
                        del phases[marker]
            for name, due in list(pending_captures.items()):
                if time.monotonic() >= due:
                    capture(name, process.pid)
                    del pending_captures[name]
        alive = process.poll() is None
        if alive and args.screenshot:
            capture('map.png', process.pid)
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
                                         'cube_pickup', 'carrying_idle', 'cube_button_opens_door', 'cube_button_closes_door',
                                         'zoom_in', 'zoom_out', 'player_model'))
    passed = passed and "User Msg 'EntityPortalled':" not in text
    passed = passed and not any(f'Unable to read particle definition particles/{name}.pcf!' in text
                               for name in ('portalgun', 'portal_projectile', 'portals'))
if args.intro_scenes:
    passed = passed and all(f'PORTAL2_INTRO scene completed PreHub01RelaxationVaultIntro0{i}' in text for i in range(1, 5))
    passed = passed and not any('Failed to load sound "vo/announcer/' in line for line in text.splitlines())
if args.elevator_transition:
    passed = passed and 'PORTAL2_ELEVATOR_TEST started authored elevator I/O' in text
    passed = passed and any(
        f'PORTAL2_INTRO elevator changing level ({route}): sp_a1_intro1 -> sp_a1_intro2' in text
        for route in ('authored trigger', 'fallback'))
    passed = passed and text.count('SV_ActivateServer: setting tickrate') >= 2
if args.bink:
    passed = passed and 'PORTAL2_BINK opened ' in text and 'Unable to play video:' not in text
label = 'GAMEPLAY' if args.gameplay else ('INTRO SCENES' if args.intro_scenes else ('ELEVATOR TRANSITION' if args.elevator_transition else ('BINK' if args.bink else 'MAP LOAD')))
print(f'{label} {"PASS" if passed else "FAIL"}: alive={alive}, server_active={activated}, shader_index_error={bad_shader}')
print(f'Artifacts: {out}')
print('Tests are bounded fixtures, not a full campaign playthrough. Gameplay checks gun, traversal and cube/button/door I/O; intro-scenes checks the vault dialogue chain. Inspect map.png for visual quality.')
raise SystemExit(0 if passed else 1)
