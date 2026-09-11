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
p.add_argument('--pause-menu', action='store_true', help='open the experimental Portal 2 pause menu after loading the map')
p.add_argument('--gameplay', action='store_true', help='run the opt-in gun/portal integration test (use --seconds 90 for a cold launch)')
p.add_argument('--buttons', action='store_true', help='test intro2 pedestal use, Press, Lock/Unlock and portal selection; use --map sp_a1_intro2 --seconds 60')
p.add_argument('--intro-scenes', action='store_true', help='test the vault dialogue chain from its real map trigger (use --seconds 75)')
p.add_argument('--elevator-transition', action='store_true', help='exercise departure, map transition and arrival elevator (use --seconds 90)')
p.add_argument('--intro2-exit', action='store_true', help='exercise departure, map transition and arrival on any intro chain map (intro2..intro5); use --map sp_a1_intro2..intro5 --seconds 120')
p.add_argument('--bink', action='store_true', help='open a Portal 2 Bink video through the ARM64 FFmpeg module')
p.add_argument('--phys-probe', action='store_true',
               help='spawn the weighted cube above the player and verify vphysics settles it (use --map sp_a1_intro2 --seconds 50)')
p.add_argument('--extra-cvar', action='append', default=[], metavar='NAME=VALUE',
               help='append a +NAME VALUE pair to the engine command line (repeatable)')
args = p.parse_args()
if sum((args.gameplay, args.intro_scenes, args.elevator_transition, args.buttons, args.intro2_exit)) > 1:
    p.error('Choose only one opt-in integration test')
if args.intro2_exit and args.map not in ('sp_a1_intro2', 'sp_a1_intro3', 'sp_a1_intro4', 'sp_a1_intro5'):
    p.error('--intro2-exit requires an intro chain map (sp_a1_intro2..sp_a1_intro5)')
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
    command += ['-portal2_gameplay_test']
if args.buttons:
    if args.map != 'sp_a1_intro2':
        p.error('--buttons requires --map sp_a1_intro2')
    command += ['-portal2_buttons_test']
if args.intro_scenes:
    command += ['-portal2_intro_scene_test']
if args.elevator_transition:
    command += ['-portal2_elevator_transition_test']
if args.intro2_exit:
    command += ['-portal2_intro_exit_test']
if args.phys_probe:
    command += ['-portal2_phys_probe']
if args.no_prop_lighting:
    command += ['+r_proplightingfromdisk', '0']
if args.portal_texture:
    command += ['+r_portal_use_stencils', '0']
command += ['+map', args.map]
if args.gameplay:
    command += ['+mat_hdr_level', '+mat_info', '+mat_fullbright', '+mat_phong']
if args.bink:
    command += ['+playvideo', 'attract01.bik']
for entry in args.extra_cvar:
    name, sep, value = entry.partition('=')
    if not sep:
        p.error(f'--extra-cvar expects NAME=VALUE, got {entry!r}')
    command += ['+' + name, value]

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
                  'zoom_in=PASS': 'gun-zoom.png', 'player_model=PASS': 'player-model.png',
                  'portal_visual_camera=blue': 'portal-view-blue.png',
                  'portal_visual_camera=orange': 'portal-view-orange.png'} if args.gameplay and args.screenshot else {}
        pending_captures = {}
        if args.elevator_transition and args.screenshot:
            phases = {'PORTAL2_ARRIVAL started': 'elevator-arrival-start.png',
                      'PORTAL2_ARRIVAL ride=PASS': 'elevator-arrival-end.png',
                      'PORTAL2_ARRIVAL exit=PASS': 'elevator-exit.png'}
        while process.poll() is None and time.monotonic() < deadline:
            time.sleep(0.25)
            if phases and (stage / 'engine.log').is_file():
                progress = (stage / 'engine.log').read_text(errors='replace')
                for marker, name in list(phases.items()):
                    if (marker if args.elevator_transition else f'PORTAL2_GAMEPLAY_TEST {marker}') in progress:
                        pending_captures[name] = time.monotonic() + (2.0 if name in ('gun-carrying.png', 'elevator-arrival-end.png') else 0.6)
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
    passed = passed and 'PORTAL2_GAMEPLAY_TEST portal_visual_camera=orange' in text
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
    passed = passed and 'PORTAL2_ARRIVAL ride=PASS' in text and 'PORTAL2_ARRIVAL ride=FAIL' not in text
    passed = passed and 'PORTAL2_ARRIVAL exit=PASS' in text
if args.bink:
    passed = passed and 'PORTAL2_BINK opened ' in text and 'Unable to play video:' not in text
if args.buttons:
    passed = passed and all(f'PORTAL2_BUTTONS select_{i}=PASS' in text for i in range(1,5))
    passed = passed and not any('PORTAL2_BUTTONS' in line and '=FAIL' in line for line in text.splitlines())
if args.intro2_exit:
    passed = passed and 'PORTAL2_INTRO_EXIT entered departure car in ' in text
    passed = passed and 'PORTAL2_INTRO departure started' in text
    passed = passed and 'Portal2 level transition: ' in text
    passed = passed and ('departure fallback' in text or 'PORTAL2_INTRO departure path reached' in text)
    passed = passed and text.count('PORTAL2_ARRIVAL ride=PASS') >= 2
    passed = passed and 'PORTAL2_ARRIVAL ride=FAIL' not in text
    passed = passed and 'PORTAL2_ARRIVAL exit_map=' in text
if args.phys_probe:
    passed = passed and 'PORTAL2_PHYS PASS settled' in text
    passed = passed and 'PORTAL2_PHYS FAIL' not in text
label = 'GAMEPLAY' if args.gameplay else ('INTRO SCENES' if args.intro_scenes else ('ELEVATOR TRANSITION' if args.elevator_transition else ('BINK' if args.bink else 'MAP LOAD')))
if args.buttons:
    label = 'BUTTONS'
if args.intro2_exit:
    label = 'INTRO2 EXIT'
if args.phys_probe:
    label = 'PHYS PROBE'
print(f'{label} {"PASS" if passed else "FAIL"}: alive={alive}, server_active={activated}, shader_index_error={bad_shader}')
print(f'Artifacts: {out}')
print('Tests are bounded fixtures, not a full campaign playthrough. Gameplay checks gun, traversal and cube/button/door I/O; intro-scenes checks the vault dialogue chain; buttons checks intro2 portal selection and reuse. Inspect map.png for visual quality.')
raise SystemExit(0 if passed else 1)
