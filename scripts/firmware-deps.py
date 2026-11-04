#!/usr/bin/env python3
"""Fetch the pinned public sources used by the firmware and the host previews.

Same approach as PocketD-D: sparse, shallow checkouts at fixed commits, no
component-registry resolution. Output goes to firmware/.deps (git-ignored).
"""
import json
import subprocess
from pathlib import Path

root = Path(__file__).resolve().parents[1] / 'firmware'
for name, dep in json.loads((root / 'dependencies.json').read_text()).items():
    dest = root / '.deps' / name
    dest.mkdir(parents=True, exist_ok=True)

    def git(*args):
        return subprocess.check_output(['git', '-C', str(dest), *args], text=True).strip()

    if not (dest / '.git').exists():
        git('init', '-q')
        git('remote', 'add', 'origin', dep['url'])
        git('sparse-checkout', 'init', '--cone')
        git('sparse-checkout', 'set', *dep['paths'])
    try:
        current = git('rev-parse', '--verify', '-q', 'HEAD')
    except subprocess.CalledProcessError:
        current = ''
    if current != dep['commit']:
        git('fetch', '--depth=1', '--filter=blob:none', 'origin', dep['commit'])
        git('checkout', '-q', '--detach', 'FETCH_HEAD')
    if git('rev-parse', '--verify', '-q', 'HEAD') != dep['commit']:
        raise SystemExit(f'{name}: unexpected revision')
    print(f'{name}: {dep["commit"]}')

# LVGL's IDF CMake exports these include directories even when unused.
for name in ['examples', 'demos']:
    (root / '.deps' / 'lvgl' / name).mkdir(exist_ok=True)
