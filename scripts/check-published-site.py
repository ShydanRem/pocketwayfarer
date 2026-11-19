#!/usr/bin/env python3
"""Smoke-check the public Pages documents and complete firmware download."""
import argparse
import hashlib
import io
import json
import re
import time
import zipfile
from pathlib import Path
from urllib.error import URLError
from urllib.parse import urlsplit
from urllib.request import Request, urlopen


def check(base, commit):
    def get(path):
        request = Request(base + path + '?release=' + commit,
                          headers={'User-Agent': 'PocketWayfarer-Release-Check/1.0'})
        with urlopen(request, timeout=30) as response:
            assert response.status == 200, f'{path}: HTTP {response.status}'
            return response.read(), response.headers.get('Content-Type', '')

    release_bytes, _ = get('firmware/release.json')
    release = json.loads(release_bytes)
    assert release['source_commit'] == commit and release['source_dirty'] is False, 'Stale release'
    assert release['hardware_tested'] is False and release['channel'] == 'preview'
    assert re.fullmatch(r'[0-9A-Za-z.+-]+', release['version'])
    assert release['bundle'] == f'pocketwayfarer-{release["version"]}.zip'
    assert [p['path'] for p in release['parts']] == ['bootloader.bin', 'partitions.bin', 'app.bin']
    assert [p['offset'] for p in release['parts']] == [0, 0x8000, 0x10000]
    files = {'release.json': release_bytes}
    for part in release['parts']:
        data, _ = get('firmware/' + part['path'])
        assert len(data) == part['size'], part['path'] + ': size mismatch'
        assert hashlib.sha256(data).hexdigest() == part['sha256'], part['path'] + ': SHA-256 mismatch'
        files[part['path']] = data
    for name in ['manifest.json', 'INSTALL.txt', 'SHA256SUMS.txt']:
        files[name], _ = get('firmware/' + name)
    manifest = json.loads(files['manifest.json'])
    assert manifest['version'] == release['version']
    assert manifest['new_install_prompt_erase'] is True and manifest['new_install_improv_wait_time'] == 0
    assert manifest['builds'][0]['chipFamily'] == 'ESP32-S3'
    assert manifest['builds'][0]['parts'] == [{'path': p['path'], 'offset': p['offset']} for p in release['parts']]
    bundle, _ = get('firmware/' + release['bundle'])
    with zipfile.ZipFile(io.BytesIO(bundle)) as archive:
        assert set(archive.namelist()) == set(files), 'Incomplete public ZIP'
        assert archive.testzip() is None
        for name, data in files.items():
            assert archive.read(name) == data, name + ': public ZIP differs from installer'
    for name in ['index.html', 'guide.html', 'notices.html','roadmap.html']:
        data, mime = get(name)
        assert 'text/html' in mime and '<html lang="it">' in data.decode(), name + ': missing document'
    for name in ['js/install.mjs', 'vendor/esp-web-tools/install-button.js']:
        data, mime = get(name)
        assert data and 'javascript' in mime, name + ': invalid module response'
    source_root=Path(__file__).resolve().parents[1]
    for name,source in [('borgo','docs/img/lantern/02-borgo.png'),('classi','docs/img/classes/otto-classi-base.png'),('avatar-base','docs/img/hero_rig/otto-avatar-base.png'),('avatar-avanzate-a','docs/img/hero_rig/otto-avatar-avanzate-a.png'),('classi-avanzate-a','docs/img/classes/otto-classi-avanzate-a.png'),('avatar-avanzate-b','docs/img/hero_rig/otto-avatar-avanzate-b.png'),('classi-avanzate-b','docs/img/classes/otto-classi-avanzate-b.png'),('avatar-maestre-a','docs/img/hero_rig/otto-avatar-maestre-a.png'),('classi-maestre-a','docs/img/classes/otto-classi-maestre-a.png'),('avatar-maestre-b','docs/img/hero_rig/otto-avatar-maestre-b.png'),('classi-maestre-b','docs/img/classes/otto-classi-maestre-b.png'),('interfaccia-classico','docs/img/hero_rig/interfaccia-classico.png')]:
        data,mime=get('assets/screens/'+name+'.png')
        assert data.startswith(b'\x89PNG\r\n\x1a\n') and 'image/png' in mime, 'Missing native screenshot'
        assert hashlib.sha256(data).digest()==hashlib.sha256((source_root/source).read_bytes()).digest(), 'Stale native screenshot'
    print(f'Public site passed: {base}, commit {commit}, {release["version"]}; '
          '4 HTML documents, JavaScript modules, 12 exact native screenshots, 3 firmware SHA-256 hashes and complete ZIP')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base-url', required=True)
    parser.add_argument('--source-commit', required=True)
    args = parser.parse_args()
    url = urlsplit(args.base_url)
    assert url.scheme == 'https' and url.hostname == 'shydanrem.github.io' and url.path == '/pocketwayfarer/'
    assert not url.query and not url.fragment and not url.username and not url.port
    assert re.fullmatch(r'[0-9a-f]{40}', args.source_commit)
    for attempt in range(6):
        try:
            check(args.base_url, args.source_commit)
            break
        except (AssertionError, URLError, TimeoutError) as error:
            if attempt == 5:
                raise
            print(f'Public release not ready ({error}); retrying in 10 seconds', flush=True)
            time.sleep(10)
