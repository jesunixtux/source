#!/usr/bin/env python3
"""Write the small Portal 2 ARM64 menu catalog into an isolated stage only."""
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('gameplay_dir', type=Path)
args = parser.parse_args()
root = args.gameplay_dir.resolve()
if root.name != 'portal2_gameplay_compat' or not (root / 'ORIGIN.txt').is_file():
    parser.error('Expected portal2_gameplay_compat from an isolated stage')

catalogs = {
    'english': ('Experimental menu', '1  Resume', '2  Restart map', '3  Disconnect', '4  Quit', '5  Language: %s1'),
    'spanish': ('Menu experimental', '1  Continuar', '2  Reiniciar mapa', '3  Desconectar', '4  Salir', '5  Idioma: %s1'),
    'brazilian': ('Menu experimental', '1  Continuar', '2  Reiniciar mapa', '3  Desconectar', '4  Sair', '5  Idioma: %s1'),
    'french': ('Menu experimental', '1  Reprendre', '2  Redemarrer la carte', '3  Deconnecter', '4  Quitter', '5  Langue : %s1'),
    'german': ('Experimentelles Menu', '1  Fortsetzen', '2  Karte neu starten', '3  Trennen', '4  Beenden', '5  Sprache: %s1'),
    'italian': ('Menu sperimentale', '1  Continua', '2  Riavvia mappa', '3  Disconnetti', '4  Esci', '5  Lingua: %s1'),
    'russian': ('Eksperimentalnoe menyu', '1  Prodolzhit', '2  Perezapustit kartu', '3  Otsoedinit', '4  Vyiti', '5  Yazyk: %s1'),
    'polish': ('Menu eksperymentalne', '1  Kontynuuj', '2  Uruchom mape ponownie', '3  Rozlacz', '4  Wyjdz', '5  Jezyk: %s1'),
}
language_names = {
    'english': 'English', 'spanish': 'Espanol', 'brazilian': 'Portugues (Brasil)',
    'french': 'Francais', 'german': 'Deutsch', 'italian': 'Italiano',
    'russian': 'Russkiy', 'polish': 'Polski',
}

resource = root / 'resource'
resource.mkdir(exist_ok=True)
for language, values in catalogs.items():
    lines = ['"lang"', '{', f'    "Language" "{language}"', '    "Tokens"', '    {',
             '        "P2ARM64_TITLE" "PORTAL 2 ARM64"',
             f'        "P2ARM64_SUBTITLE" "{values[0]}"',
             f'        "P2ARM64_RESUME" "{values[1]}"',
             f'        "P2ARM64_RESTART" "{values[2]}"',
             f'        "P2ARM64_DISCONNECT" "{values[3]}"',
             f'        "P2ARM64_QUIT" "{values[4]}"',
             f'        "P2ARM64_LANGUAGE_ACTION" "{values[5]}"']
    for name, label in language_names.items():
        lines.append(f'        "P2ARM64_LANGUAGE_{name.upper()}" "{label}"')
    lines += ['    }', '}']
    # Valve's ILocalize parser requires UCS-2 little-endian with a BOM.
    (resource / f'portal2_arm64_{language}.txt').write_bytes(('\ufeff' + '\n'.join(lines) + '\n').encode('utf-16-le'))
print(f'Wrote {len(catalogs)} isolated Portal 2 ARM64 localization catalogs: {resource}')
