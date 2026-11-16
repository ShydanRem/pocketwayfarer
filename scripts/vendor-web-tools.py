#!/usr/bin/env python3
"""Vendor the official, integrity-pinned browser bundle; no CDN at runtime."""
import argparse
import base64
import hashlib
import io
import json
import re
import tarfile
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SPEC = json.loads((ROOT / 'web/dependencies.json').read_text())['esp-web-tools']
DEST = ROOT / 'web/vendor/esp-web-tools'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if args.check:
        receipt = json.loads((DEST / 'receipt.json').read_text())
        assert receipt['integrity'] == SPEC['integrity']
        actual = {p.relative_to(DEST).as_posix() for p in DEST.rglob('*') if p.is_file() and p.name != 'receipt.json'}
        assert actual == set(receipt['files']), 'Vendored file set differs'
        for name, digest in receipt['files'].items():
            assert hashlib.sha256((DEST / name).read_bytes()).hexdigest() == digest, name
        print(f"ESP Web Tools {SPEC['version']}: {len(actual)} local files verified")
        return
    cache = ROOT / 'build/web-deps' / f"esp-web-tools-{SPEC['version']}.tgz"
    cache.parent.mkdir(parents=True, exist_ok=True)
    data = cache.read_bytes() if cache.exists() else urllib.request.urlopen(SPEC['url'], timeout=30).read()
    digest = base64.b64encode(hashlib.sha512(data).digest()).decode()
    if 'sha512-' + digest != SPEC['integrity']:
        raise SystemExit('Official npm archive integrity mismatch; nothing installed')
    files = {}
    with tarfile.open(fileobj=io.BytesIO(data), mode='r:gz') as archive:
        for entry in archive.getmembers():
            if not entry.isfile():
                continue
            if entry.name.startswith('package/dist/web/'):
                relative = Path(entry.name.removeprefix('package/dist/web/'))
            elif entry.name == 'package/LICENSE':
                relative = Path('LICENSE')
            else:
                continue
            if relative.is_absolute() or '..' in relative.parts:
                raise SystemExit('Unsafe archive path')
            payload = archive.extractfile(entry).read()
            files[relative.as_posix()] = payload
    assert 'install-button.js' in files and 'LICENSE' in files
    dependencies = json.loads((ROOT / 'web/runtime-notices.json').read_text())
    notices = ['# ESP Web Tools — runtime dependency notices', '',
               'Notices retained from the official npm archives pinned by the release lock.', '']
    for dependency in dependencies['packages']:
        name, version = dependency['name'], dependency['version']
        archive_name = name.replace('/', '_').replace('@', '') + '-' + version + '.tgz'
        archive_path = cache.parent / archive_name
        package = archive_path.read_bytes() if archive_path.exists() else urllib.request.urlopen(dependency['resolved'], timeout=30).read()
        require_digest = 'sha512-' + base64.b64encode(hashlib.sha512(package).digest()).decode()
        if require_digest != dependency['integrity']:
            raise SystemExit(f'Dependency archive integrity mismatch: {name}')
        texts = []
        with tarfile.open(fileobj=io.BytesIO(package), mode='r:gz') as archive:
            for entry in archive.getmembers():
                if entry.isfile() and Path(entry.name).name.upper().split('.')[0] in ('LICENSE', 'LICENCE', 'NOTICE', 'COPYRIGHT', 'COPYING'):
                    texts.append((entry.name, archive.extractfile(entry).read().decode('utf-8')))
            headers = set()
            for entry in archive.getmembers():
                if entry.isfile() and entry.name.endswith('.js'):
                    source = archive.extractfile(entry).read().decode('utf-8')
                    for header in re.findall(r'/\*.*?\*/', source, re.S):
                        if '@license' in header and header not in headers:
                            texts.append((entry.name + ' — original license header', header)); headers.add(header)
        if not texts:
            raise SystemExit(f'Dependency license missing: {name}')
        archive_path.write_bytes(package)
        notices.extend([f'## {name} {version}', '', f'Source: {dependency["resolved"]}', ''])
        for path, text in texts:
            notices.extend([f'### {path}', '', '```text', text.strip(), '```', ''])
    files['THIRD_PARTY_NOTICES.md'] = '\n'.join(notices).encode('utf-8')
    DEST.mkdir(parents=True, exist_ok=True)
    for relative, payload in files.items():
        out = DEST / relative
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(payload)
    receipt = dict(SPEC, runtime_lock_source=dependencies['source'], files={name: hashlib.sha256(value).hexdigest() for name, value in sorted(files.items())})
    (DEST / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    cache.write_bytes(data)
    print(f"Vendored ESP Web Tools {SPEC['version']}: {len(files)} files, with license")


if __name__ == '__main__':
    main()
