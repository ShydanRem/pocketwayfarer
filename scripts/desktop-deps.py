#!/usr/bin/env python3
"""Acquire pinned SDL2 headers only. The desktop links the installed SDL2 runtime."""
import hashlib
import io
import tarfile
import urllib.request
from pathlib import Path

VERSION = '2.32.4'
SHA256 = 'f15b478253e1ff6dac62257ded225ff4e7d0c5230204ac3450f1144ee806f934'
ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'build/desktop-deps'
ARCHIVE = DEST / f'SDL2-{VERSION}.tar.gz'
URL = f'https://github.com/libsdl-org/SDL/releases/download/release-{VERSION}/SDL2-{VERSION}.tar.gz'
DEST.mkdir(parents=True, exist_ok=True)
data = ARCHIVE.read_bytes() if ARCHIVE.exists() else urllib.request.urlopen(URL, timeout=30).read()
if hashlib.sha256(data).hexdigest() != SHA256:
    raise SystemExit('SDL2 archive checksum mismatch; no headers installed')
ARCHIVE.write_bytes(data)
with tarfile.open(fileobj=io.BytesIO(data)) as archive:
    prefix = f'SDL2-{VERSION}/'
    for member in archive.getmembers():
        if not member.isfile() or not member.name.startswith(prefix):
            continue
        relative = Path(member.name[len(prefix):])
        if '..' in relative.parts or relative.is_absolute():
            raise SystemExit('Unsafe archive member')
        if relative.parts[0] != 'include' and str(relative) != 'LICENSE.txt':
            continue
        output = DEST / relative
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(archive.extractfile(member).read())
print(f'SDL2 {VERSION} development headers verified; runtime library must be installed separately')
