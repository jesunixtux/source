#!/usr/bin/env python3
"""Convert MP3-in-WAV dialogue used by intro1 to isolated PCM WAV copies."""
import argparse
import hashlib
import importlib.util
from pathlib import Path
import re
import shutil
import struct
import subprocess
import vpk


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_dir', type=Path)
    parser.add_argument('overlay', type=Path)
    args = parser.parse_args()
    ffmpeg = shutil.which('ffmpeg')
    if not ffmpeg:
        parser.error('ffmpeg is required for Portal 2 MP3-in-WAV dialogue conversion')
    spec = importlib.util.spec_from_file_location('intro_tables', Path(__file__).with_name('prepare-portal2-intro-script.py'))
    tables = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(tables)
    archive = vpk.open(str(args.game_dir / 'pak01_dir.vpk'))
    names = {name.lower(): name for name in archive}
    bsp = (args.game_dir / 'maps/sp_a1_intro1.bsp').read_bytes()
    offset, size = struct.unpack_from('<2I', bsp, 8)
    entities = bsp[offset:offset + size].decode('cp1252')
    data = (args.overlay / 'scripts/portal2_intro1_scenes.txt').read_text()
    scene_paths = set(re.findall(r'"(scenes/[^"\r\n]+\.(?:vcd|wav))"', entities + data, re.I))
    sounds = set()
    for path in sorted(scene_paths):
        path = str(Path(path).with_suffix('.vcd')).lower()
        if path not in names:
            raise ValueError('Missing source scene: ' + path)
        scene = archive[names[path]].read().decode('cp1252')
        for event in re.finditer(r'event\s+speak\s+"[^"]*"\s*\{', scene):
            block = tables.block(scene, event.end() - 1)
            sounds.update(s.lower() for s in re.findall(r'\bparam\s+"([^"]+)"', block))
    waves = set()
    found = set()
    # The installed sound scripts retain original spelling; VPK lookups use
    # lowercase names. Keep that spelling in the loose overlay for this branch.
    for script in args.game_dir.glob('scripts/game_sounds*.txt'):
        text = tables.uncomment(script.read_text(encoding='cp1252'))
        for match in re.finditer(r'"([^"\r\n]+)"\s*\{', text):
            if match[1].lower() not in sounds:
                continue
            found.add(match[1].lower())
            body = tables.block(text, match.end() - 1)
            waves.update(re.findall(r'"wave"\s+"[\*#@<>^)}$!?~+%&]*([^"\r\n]+)"', body, re.I))
    missing = sounds - found
    if missing:
        raise ValueError('Missing sound entries: ' + ', '.join(sorted(missing)))
    report = []
    for wave in sorted(waves):
        relative = Path('sound') / wave.replace('\\', '/')
        if '..' in relative.parts or relative.is_absolute():
            raise ValueError('Invalid wave path: ' + wave)
        source = archive[names[str(relative).lower()]].read()
        digest = hashlib.sha256(source).hexdigest()
        target = args.overlay / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        if source[:4] == b'RIFF':
            target.write_bytes(source)
            mode = 'RIFF copy'
        else:
            # A real file output is necessary: a WAV on stdout has unknown
            # RIFF/data sizes, which the old Source reader does not accept.
            subprocess.run([ffmpeg, '-hide_banner', '-loglevel', 'error', '-y',
                            '-f', 'mp3', '-i', 'pipe:0', '-c:a', 'pcm_s16le',
                            '-ar', '44100', str(target)], input=source, check=True)
            mode = 'MP3 -> PCM16/44100'
        report.append(f'{digest}  {relative}  {mode}')
    (args.overlay / 'INTRO_AUDIO_ORIGIN.txt').write_text('\n'.join(report) + '\n')
    print(f'Intro audio: {len(waves)} sounds from {len(scene_paths)} scenes -> {args.overlay}')


if __name__ == '__main__':
    main()
