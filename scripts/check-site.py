#!/usr/bin/env python3
"""Validate an assembled, base-path-safe website and its exact firmware artifacts."""
import argparse
import hashlib
import json
import re
import zipfile
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit


class Document(HTMLParser):
    def __init__(self, path):
        super().__init__(); self.path = path; self.links = []; self.ids = set(); self.h1 = 0; self.lang = None
    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        if tag == 'html': self.lang = a.get('lang')
        if tag == 'h1': self.h1 += 1
        if 'id' in a:
            assert a['id'] not in self.ids, f'{self.path}: duplicate ID {a["id"]}'
            self.ids.add(a['id'])
        if tag == 'img': assert 'alt' in a, f'{self.path}: image without alt'
        if tag in ('a', 'link') and a.get('href'): self.links.append(a['href'])
        if tag in ('img', 'script') and a.get('src'): self.links.append(a['src'])


def check(root):
    documents = {}
    for path in root.rglob('*.html'):
        doc = Document(path); doc.feed(path.read_text())
        assert doc.lang == 'it' and doc.h1 == 1, f'{path}: language/H1 missing'
        documents[path.resolve()] = doc
    assert len(documents) == 3, 'Expected landing, manual and notices'
    for path, doc in documents.items():
        for link in doc.links:
            url = urlsplit(link)
            if url.scheme or url.netloc: continue
            assert not url.path.startswith('/'), f'{path}: link breaks GitHub Pages project base: {link}'
            target = (path.parent / unquote(url.path)).resolve() if url.path else path
            assert target.is_relative_to(root), f'{path}: link escapes site: {link}'
            assert target.exists(), f'{path}: missing link {link}'
            if url.fragment and target in documents: assert unquote(url.fragment) in documents[target].ids, f'{path}: missing anchor {link}'
    for css in root.glob('*.css'):
        for name in re.findall(r'url\([\'\"]?([^\)\'\"]+)', css.read_text()):
            assert (css.parent / name).is_file(), f'Missing CSS resource {name}'
    # Browser dependencies are local; every bundled relative module must exist.
    for script in (root / 'vendor/esp-web-tools').glob('*.js'):
        for module in re.findall(r'[\"\'](\./[^\"\']+\.js)[\"\']', script.read_text()):
            assert (script.parent / module).is_file(), f'Missing vendored module {module}'
    firmware = root / 'firmware'
    release = json.loads((firmware / 'release.json').read_text())
    manifest = json.loads((firmware / 'manifest.json').read_text())
    assert release['version'] == manifest['version']
    assert manifest['new_install_prompt_erase'] is True and manifest['new_install_improv_wait_time'] == 0
    assert manifest['builds'][0]['chipFamily'] == 'ESP32-S3'
    assert manifest['builds'][0]['parts'] == [{'path': p['path'], 'offset': p['offset']} for p in release['parts']]
    assert [p['offset'] for p in release['parts']] == [0, 0x8000, 0x10000]
    for part in release['parts']:
        data = (firmware / part['path']).read_bytes()
        assert len(data) == part['size'] and hashlib.sha256(data).hexdigest() == part['sha256']
        assert part['offset'] + part['size'] <= release['save_partition']['offset']
    with zipfile.ZipFile(firmware / release['bundle']) as archive:
        assert set(archive.namelist()) == {'bootloader.bin', 'partitions.bin', 'app.bin', 'manifest.json', 'release.json', 'INSTALL.txt', 'SHA256SUMS.txt'}
        assert not archive.testzip()
        for name in archive.namelist(): assert archive.read(name) == (firmware / name).read_bytes(), name
    assert not list(root.rglob('*.template.html'))
    assert not any(p.name in ('.git', '.env', 'desktop-save.bin') for p in root.rglob('*'))
    print(f'Site: {len(documents)} accessible HTML documents, local links/anchors/assets/modules, complete ZIP and {len(release["parts"])} firmware hashes passed')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--site', type=Path, default=Path('build/site'))
    check(parser.parse_args().site.resolve())
