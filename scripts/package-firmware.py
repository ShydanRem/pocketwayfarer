#!/usr/bin/env python3
"""Validate an ESP-IDF build and make an installable, checksummed browser bundle."""
import argparse
import hashlib
import json
import re
import struct
import subprocess
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BOARD = 'Waveshare ESP32-S3-Touch-AMOLED-1.75'
FLASH_BYTES = 16 * 1024 * 1024
APP_OFFSET, APP_SIZE = 0x10000, 0x600000
SAVE_OFFSET, SAVE_SIZE = 0x610000, 0x40000


def require(value, message):
    if not value:
        raise ValueError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def partition_entries(data):
    entries = {}
    for start in range(0, len(data), 32):
        chunk = data[start:start + 32]
        if len(chunk) != 32 or chunk[:2] != b'\xaa\x50':
            break
        _, kind, subtype, offset, size, label, flags = struct.unpack('<HBBII16sI', chunk)
        name = label.split(b'\0', 1)[0].decode('ascii')
        require(name not in entries, 'Duplicate partition label')
        entries[name] = dict(type=kind, subtype=subtype, offset=offset, size=size, flags=flags)
    return entries


def image_header(data, name):
    require(len(data) >= 32 and data[0] == 0xe9, f'{name}: invalid ESP image')
    require(struct.unpack_from('<H', data, 12)[0] == 9, f'{name}: expected ESP32-S3 chip ID')


def build_package(build, dest, allow_dirty=False):
    version = (ROOT / 'VERSION').read_text().strip()
    require(re.fullmatch(r'\d+\.\d+\.\d+(?:-[a-zA-Z0-9.]+)?', version), 'Invalid VERSION')
    description = json.loads((build / 'project_description.json').read_text())
    require(description['project_name'] == 'pocketwayfarer' and description['target'] == 'esp32s3', 'Wrong build/project target')
    require(description['project_version'] == version, 'Rebuild firmware: binary project version does not match VERSION')
    args = json.loads((build / 'flasher_args.json').read_text())
    require(args['extra_esptool_args']['chip'] == 'esp32s3', 'Wrong flasher chip')
    require(args['flash_settings'] == {'flash_mode': 'dio', 'flash_size': '16MB', 'flash_freq': '80m'}, 'Unexpected flash settings')
    expected = {0: ('bootloader.bin', 'bootloader'), 0x8000: ('partitions.bin', 'partition-table'), APP_OFFSET: ('app.bin', 'app')}
    files = {int(offset, 0): name for offset, name in args['flash_files'].items()}
    require(set(files) == set(expected), 'Unexpected flash offsets; refusing to invent Arduino boot_app0 or NVS files')
    payloads = {}
    for offset, file in files.items():
        source = (build / file).resolve()
        require(source.is_relative_to(build.resolve()), 'Build file escapes build directory')
        data = source.read_bytes()
        require(data and offset + len(data) <= FLASH_BYTES, 'Image outside flash')
        require(offset + len(data) <= SAVE_OFFSET or offset >= SAVE_OFFSET + SAVE_SIZE, 'Image overlaps saved games')
        payloads[offset] = data
    for offset in (0, APP_OFFSET):
        image_header(payloads[offset], expected[offset][0])
    boot = payloads[0]
    require(boot[2] == 2 and boot[3] == 0x4f, 'Bootloader must embed DIO / 16MB / 80MHz for browser flashing; rebuild with the supported config')
    app = payloads[APP_OFFSET]
    require(struct.unpack_from('<I', app, 32)[0] == 0xabcd5432, 'ESP app descriptor missing')
    embedded_version = app[48:80].split(b'\0', 1)[0].decode('ascii')
    require(embedded_version == version, 'Embedded app version does not match VERSION')
    require(len(app) <= APP_SIZE, 'Application exceeds the factory partition')
    partitions = partition_entries(payloads[0x8000])
    require(set(partitions) == {'nvs', 'phy_init', 'factory', 'wf_nvs'}, 'Unexpected partition set')
    require(partitions.get('nvs') == dict(type=1, subtype=2, offset=0x9000, size=0x6000, flags=0), 'System NVS partition mismatch')
    require(partitions.get('phy_init') == dict(type=1, subtype=1, offset=0xf000, size=0x1000, flags=0), 'PHY partition mismatch')
    require(partitions.get('factory') == dict(type=0, subtype=0, offset=APP_OFFSET, size=APP_SIZE, flags=0), 'Application partition mismatch')
    require(partitions.get('wf_nvs') == dict(type=1, subtype=2, offset=SAVE_OFFSET, size=SAVE_SIZE, flags=0), 'Save partition mismatch')
    ordered = sorted(payloads)
    for a, b in zip(ordered, ordered[1:]):
        require(a + len(payloads[a]) <= b, 'Flash images overlap')
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    dirty = bool(subprocess.check_output(['git', 'status', '--porcelain', '--untracked-files=normal'], cwd=ROOT, text=True).strip())
    require(allow_dirty or not dirty, 'Source has uncommitted changes; use --allow-dirty only for a local preview')
    dest.mkdir(parents=True, exist_ok=True)
    parts = []
    for offset in ordered:
        name, role = expected[offset]
        data = payloads[offset]
        (dest / name).write_bytes(data)
        parts.append(dict(path=name, role=role, offset=offset, size=len(data), sha256=digest(data)))
    manifest = dict(name='Pocket Wayfarer', version=version, new_install_prompt_erase=True,
                    new_install_improv_wait_time=0,
                    builds=[dict(chipFamily='ESP32-S3', parts=[dict(path=p['path'], offset=p['offset']) for p in parts])])
    save_header = (ROOT / 'firmware/rpg/core/rpg.h').read_text()
    save_format = int(re.search(r'^#define RPG_SAVE_VERSION (\d+)u$', save_header, re.MULTILINE)[1])
    metadata = dict(schema=1, product='Pocket Wayfarer', version=version, channel='preview', board=BOARD,
                    chip_family='ESP32-S3', flash_size=FLASH_BYTES, save_format=save_format,
                    source_commit=commit, source_dirty=dirty, hardware_tested=False,
                    flash_settings=args['flash_settings'], parts=parts,
                    save_partition=dict(offset=SAVE_OFFSET, size=SAVE_SIZE), app_partition=dict(offset=APP_OFFSET, size=APP_SIZE),
                    manifest='manifest.json', bundle=f'pocketwayfarer-{version}.zip')
    (dest / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (dest / 'release.json').write_text(json.dumps(metadata, indent=2) + '\n')
    instructions = f'''Pocket Wayfarer {version} — PREVIEW
Board: {BOARD}. ESP32-S3, 16MB flash, 8MB OPI PSRAM.
Built source: {commit}{' (local changes)' if dirty else ''}.
Hardware validation: pending. Native desktop/core tests are documented in the source.

Recommended: use the GitHub Pages installer with Chrome/Edge and a USB data cable.
For updates from Pocket Wayfarer with this partition layout, leave Erase device OFF.
Erasing the entire device removes saved games. Back up before changing firmware/layout.

Manual installation (Python esptool 4.x):
python -m esptool --chip esp32s3 --port PORT --baud 460800 write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 app.bin

Save-partition backup for this layout only:
python -m esptool --chip esp32s3 --port PORT read_flash 0x610000 0x40000 pocketwayfarer-save.bin

Do not use app.bin alone for a first installation. manifest.json lists all required parts.
No SD card, Wi-Fi credentials or external sprite package is needed for this build.
See docs/INSTALLAZIONE.md and docs/SALVATAGGI.md in the source repository.
'''
    (dest / 'INSTALL.txt').write_text(instructions)
    hashes = '\n'.join(f"{p['sha256']}  {p['path']}" for p in parts) + '\n'
    (dest / 'SHA256SUMS.txt').write_text(hashes)
    # Fixed timestamps and file modes make packaging independent of ZIP host metadata.
    members = [p['path'] for p in parts] + ['manifest.json', 'release.json', 'INSTALL.txt', 'SHA256SUMS.txt']
    with zipfile.ZipFile(dest / metadata['bundle'], 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for name in sorted(members):
            info = zipfile.ZipInfo(name, (2026, 1, 1, 0, 0, 0)); info.external_attr = 0o100644 << 16
            archive.writestr(info, (dest / name).read_bytes(), compress_type=zipfile.ZIP_DEFLATED)
    print(f"Firmware {version}: {len(parts)} verified parts, {len(app)} app bytes, save partition excluded")
    return metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--allow-dirty', action='store_true')
    args = parser.parse_args()
    try:
        build_package(args.build.resolve(), args.output.resolve(), args.allow_dirty)
    except (ValueError, KeyError, OSError, json.JSONDecodeError) as exc:
        raise SystemExit(f'Cannot package firmware: {exc}') from exc


if __name__ == '__main__':
    main()
