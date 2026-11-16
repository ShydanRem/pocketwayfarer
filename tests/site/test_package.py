"""Reject release/partition errors before an installer can offer any firmware."""
import importlib.util
import json
import os
import shutil
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('package_firmware', ROOT / 'scripts/package-firmware.py')
package = importlib.util.module_from_spec(spec); spec.loader.exec_module(package)
SOURCE = Path(os.environ.get('POCKET_TEST_BUILD', ROOT / 'build/firmware-hd2d')).resolve()


class FirmwarePackage(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(); self.addCleanup(self.temp.cleanup)
        self.build = Path(self.temp.name) / 'build'; self.build.mkdir()
        self.dest = Path(self.temp.name) / 'out'
        for name in ['project_description.json', 'flasher_args.json', 'bootloader/bootloader.bin', 'partition_table/partition-table.bin', 'pocketwayfarer.bin']:
            target = self.build / name; target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(SOURCE / name, target)
    def json_change(self, name, change):
        p = self.build / name; data = json.loads(p.read_text()); change(data); p.write_text(json.dumps(data))
    def rejects(self):
        with self.assertRaises(ValueError): package.build_package(self.build, self.dest, allow_dirty=True)
        self.assertFalse((self.dest / 'manifest.json').exists())
    def test_valid_complete_package(self):
        release = package.build_package(self.build, self.dest, allow_dirty=True)
        self.assertEqual(release['save_partition']['offset'], 0x610000)
        self.assertTrue((self.dest / release['bundle']).is_file())
    def test_wrong_project_version(self):
        self.json_change('project_description.json', lambda d: d.update(project_version='old'))
        self.rejects()
    def test_app_only_build(self):
        self.json_change('flasher_args.json', lambda d: d['flash_files'].pop('0x0'))
        self.rejects()
    def test_save_partition_offset(self):
        p = self.build / 'partition_table/partition-table.bin'; b = bytearray(p.read_bytes()); b[3*32+4] ^= 1; p.write_bytes(b)
        self.rejects()
    def test_system_partition_alignment(self):
        p = self.build / 'partition_table/partition-table.bin'; b = bytearray(p.read_bytes()); b[4] ^= 1; p.write_bytes(b)
        self.rejects()
    def test_wrong_chip(self):
        p = self.build / 'bootloader/bootloader.bin'; b = bytearray(p.read_bytes()); b[12] = 0; p.write_bytes(b)
        self.rejects()
    def test_unpatched_boot_header(self):
        p = self.build / 'bootloader/bootloader.bin'; b = bytearray(p.read_bytes()); b[2] = 0; p.write_bytes(b)
        self.rejects()
    def test_file_outside_build(self):
        self.json_change('flasher_args.json', lambda d: d['flash_files'].update({'0x0': '../outside.bin'}))
        self.rejects()
    def test_embedded_version_mismatch(self):
        p = self.build / 'pocketwayfarer.bin'; b = bytearray(p.read_bytes()); b[48] ^= 1; p.write_bytes(b)
        self.rejects()


if __name__ == '__main__': unittest.main()
