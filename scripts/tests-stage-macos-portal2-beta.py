#!/usr/bin/env python3
"""Small VPK extraction regression tests; no installed game or build required."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest
import zlib

spec = importlib.util.spec_from_file_location('stage_beta', Path(__file__).with_name('stage-macos-portal2-beta.py'))
stage = importlib.util.module_from_spec(spec)
spec.loader.exec_module(stage)


class ExtractTests(unittest.TestCase):
    def archive(self, root, version=2, directory='resource', external=False):
        payload = b'"Scheme" { "test" "value" }'
        preload = payload[:3]
        tree = (b'res\0' + directory.encode() + b'\0SourceScheme\0' +
                struct.pack('<IHHIIH', zlib.crc32(payload), len(preload),
                            0 if external else 0x7fff, 0, len(payload) - len(preload), 0xffff) +
                preload + b'\0\0\0')
        header = struct.pack('<III', 0x55AA1234, version, len(tree))
        if version == 2:
            header += struct.pack('<4I', 0 if external else len(payload) - len(preload), 0, 0, 0)
        path = root / 'pak_dir.vpk'
        path.write_bytes(header + tree + (b'' if external else payload[len(preload):]))
        if external:
            (root / 'pak_000.vpk').write_bytes(payload[len(preload):])
        return path, payload

    def test_versions_preload_and_external_data(self):
        for version, external in ((1, False), (2, False), (2, True)):
            with self.subTest(version=version, external=external), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                path, payload = self.archive(root, version, external=external)
                self.assertEqual(stage.vpk_extract(path, lambda key: key.endswith('.res'), root / 'out'), 1)
                self.assertEqual((root / 'out/resource/sourcescheme.res').read_bytes(), payload)

    def test_root_path_sentinel(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path, payload = self.archive(root, directory=' ')
            stage.vpk_extract(path, lambda key: True, root / 'out')
            self.assertEqual((root / 'out/sourcescheme.res').read_bytes(), payload)

    def test_unsafe_names_and_corrupt_data(self):
        for directory in ('../escape', '/escape', '..\\escape'):
            with self.subTest(directory=directory), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                path, _ = self.archive(root, directory=directory)
                with self.assertRaises(ValueError):
                    stage.vpk_extract(path, lambda key: True, root / 'out')
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path, _ = self.archive(root)
            path.write_bytes(path.read_bytes()[:-1])
            with self.assertRaises(ValueError):
                stage.vpk_extract(path, lambda key: True, root / 'out')

    def test_output_symlink_escape(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            path, _ = self.archive(root)
            (root / 'out').mkdir()
            (root / 'other').mkdir()
            (root / 'out/resource').symlink_to(root / 'other', target_is_directory=True)
            with self.assertRaises(ValueError):
                stage.vpk_extract(path, lambda key: True, root / 'out')

    def test_beta_vgui_assets_are_not_replaced(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            content = root / 'content'
            (content / 'resource').mkdir(parents=True)
            (content / 'materials/console').mkdir(parents=True)
            names = ('SourceScheme.res', 'ClientScheme.res', 'basemodui_scheme.res', 'GameMenu.res')
            for name in names:
                (content / 'resource' / name).write_bytes(('beta-' + name).encode())
            for aspect in ('', '_widescreen'):
                for suffix in ('.vmt', '.vtf'):
                    (content / f'materials/console/background01{aspect}{suffix}').write_bytes(b'beta-background')
            output = root / 'override'
            stage.stage_beta_vgui(content, output)
            for name in names:
                self.assertEqual((output / 'resource' / name.lower()).read_bytes(),
                                 (content / 'resource' / name).read_bytes())
            self.assertFalse((output / 'resource/clientscheme_override.res').exists())
            self.assertEqual(len((output / 'resource/VGUI_ORIGIN.txt').read_text().splitlines()), 4)
            self.assertEqual((output / 'materials/console/background_menu_widescreen.vtf').read_bytes(), b'beta-background')


if __name__ == '__main__':
    unittest.main()
