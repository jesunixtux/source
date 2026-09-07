#!/usr/bin/env python3
"""Extract the opening map's scene data from the installed scripts, without executing Squirrel."""
import argparse
from pathlib import Path
import re

def uncomment(text):
    return re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*|/\*[\s\S]*?\*/',
                  lambda m: m[0] if m[0].startswith('"') else '', text)

def block(text, start):
    depth, quoted, escaped = 0, False, False
    for i in range(start, len(text)):
        c = text[i]
        if quoted:
            if escaped: escaped = False
            elif c == '\\': escaped = True
            elif c == '"': quoted = False
        elif c == '"': quoted = True
        elif c == '{': depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0: return text[start + 1:i]
    raise ValueError('Unterminated table')

def scalar(text, key, default=''):
    m = re.search(r'\b' + key + r'\s*=\s*("(?:\\.|[^"\\])*"|[-+.\d]+|true|false|null)', text)
    if not m: return default
    value = m[1]
    if value == 'null': return ''
    return value[1:-1] if value.startswith('"') else value

def quote(s):
    return '"' + str(s).replace('\\', '\\\\').replace('"', '\\"') + '"'

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--game-dir', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    scripts = a.game_dir / 'scripts/vscripts/choreo'
    automatic = uncomment((scripts / 'glados_scenetable_include.nut').read_text(encoding='cp1252'))
    manual = uncomment((scripts / 'glados_scenetable_include_manual.nut').read_text(encoding='cp1252'))
    start = manual.index('if (curMapName=="sp_a1_intro1" || curMapName=="sp_a1_intro1_no_sound" )')
    intro = block(manual, manual.index('{', start))
    lookup = dict(re.findall(r'SceneTableLookup\[(-?\d+)\]\s*<-\s*"([^"]+)"', automatic + manual))
    tables = {}
    for text in (automatic, intro):
        for m in re.finditer(r'SceneTable\["([^"]+)"\]\s*<-\s*\{', text):
            body = block(text, m.end() - 1)
            scene = re.search(r'CreateSceneEntity\(\s*"([^"]+)"', body)
            if scene: tables[m[1]] = (body, scene[1])
    roots = ['0', '7', '9', '359', '360', '361', '553', '630'] + [str(i) for i in (-600,-601,-602,-603,-604,-607,-608)]
    pending = [lookup[i] for i in roots]
    selected = {}
    while pending:
        name = pending.pop()
        if name in selected: continue
        if name not in tables: raise ValueError('Missing scene table: ' + name)
        body, path = tables[name]
        selected[name] = (body, path)
        nxt = scalar(body, 'next')
        if nxt: pending.append(nxt)
    # Nags are randomized in the original VM; retain all candidates for cycling.
    for name, value in tables.items():
        if name.startswith('-601_'): selected[name] = value
    lines = ['"Portal2IntroScenes"', '{', ' "lookup"', ' {']
    lines += [f'  {quote(i)} {quote(lookup[i])}' for i in roots]
    lines += [' }', ' "scenes"', ' {']
    for name, (body, path) in sorted(selected.items()):
        lines += [f'  {quote(name)}', '  {', f'   "vcd" {quote(path)}']
        for key in ('next', 'char', 'predelay', 'postdelay', 'settarget1', 'talkover', 'queue'):
            lines.append(f'   {quote(key)} {quote(scalar(body,key))}')
        fires = []
        for m in re.finditer(r'\{\s*entity\s*=', body):
            item = block(body, m.start())
            fires.append({key: scalar(item,key) for key in ('entity','input','parameter','delay','fireatstart')})
        lines += ['   "fires"', '   {']
        for i, fire in enumerate(fires):
            lines += [f'    "{i}"', '    {'] + [f'     {quote(k)} {quote(v)}' for k,v in fire.items()] + ['    }']
        lines += ['   }','  }']
    lines += [' }','}']
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text('\n'.join(lines) + '\n')
    print(f'Intro scene data: {len(selected)} original scene entries -> {a.output}')

if __name__ == '__main__':
    main()
