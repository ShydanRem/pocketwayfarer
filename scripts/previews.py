#!/usr/bin/env python3
"""Turns the host preview's PPM screenshots into round PNGs and a gallery.

  python3 scripts/previews.py <ppm_dir> <png_dir>
"""
import sys
import re
from pathlib import Path

from PIL import Image, ImageDraw

src, dst = Path(sys.argv[1]), Path(sys.argv[2])
dst.mkdir(parents=True, exist_ok=True)
mask = Image.new('L', (466, 466), 0)
ImageDraw.Draw(mask).ellipse((0, 0, 465, 465), fill=255)
shots = []
stems = {p.stem for p in src.glob('*.ppm')}
for old in dst.glob('*.png'):
    if re.match(r'^\d+-',old.name) and old.stem not in stems:
        old.unlink()
for ppm in sorted(src.glob('*.ppm'),key=lambda p:int(p.stem.split('-')[0])):
    im = Image.open(ppm).convert('RGB')
    out = Image.new('RGBA', (466, 466), (0, 0, 0, 0))
    out.paste(im, (0, 0), mask)
    png = dst / (ppm.stem + '.png')
    out.save(png)
    shots.append(out)
cols = 4
if shots:
    tile = 240
    rows = (len(shots) + cols - 1) // cols
    sheet = Image.new('RGBA', (cols * tile, rows * tile), (18, 18, 28, 255))
    for i, s in enumerate(shots):
        sheet.alpha_composite(s.resize((tile - 10, tile - 10), Image.LANCZOS), ((i % cols) * tile + 5, (i // cols) * tile + 5))
    sheet.save(dst / 'galleria.png')
    if dst.name == 'hero_rig':
        base=sorted(dst.glob('*-studio-??-0-0.png'),key=lambda p:int(p.stem.split('-')[2]))
        if len(base)==8:
            overview=Image.new('RGBA',(1960,980),(16,24,32,255))
            for i,p in enumerate(base):
                overview.alpha_composite(Image.open(p).convert('RGBA'),((i%4)*490+12,(i//4)*490+12))
            overview.save(dst/'otto-avatar-base.png')
            overview.convert('LA').convert('RGBA').save(dst/'otto-avatar-grigio.png')
    if dst.name == 'classes':
        base = sorted(p for p in dst.glob('*-classe-??.png') if int(p.stem[-2:]) < 8)
        if len(base) == 8:
            overview = Image.new('RGBA', (1960,980), (16,24,32,255))
            for i,p in enumerate(base):
                overview.alpha_composite(Image.open(p).convert('RGBA'), ((i%4)*490+12,(i//4)*490+12))
            overview.save(dst / 'otto-classi-base.png')
    if dst.name == 'lantern':
        # Native-size proof of the three representative screens; no concept art.
        names = ['02-borgo', '05-combattimento', '10-reliquia']
        overview = Image.new('RGBA', (3 * 490, 490), (16, 24, 32, 255))
        for i, name in enumerate(names):
            capture = Image.open(dst / (name + '.png')).convert('RGBA')
            overview.alpha_composite(capture, (i * 490 + 12, 12))
        overview.save(dst / 'panoramica.png')
        cast = Image.new('RGBA', (980, 980), (16, 24, 32, 255))
        for i, name in enumerate(['03-assetto', '25-piromante', '26-predone', '14-guardiano']):
            capture = Image.open(dst / (name + '.png')).convert('RGBA')
            cast.alpha_composite(capture, ((i % 2) * 490 + 12, (i // 2) * 490 + 12))
        cast.save(dst / 'assetti-e-guardiano.png')
        for output, names in {
            'guida-e-risonanza': ['32-guida-primo-attacco', '35-parata-perfetta-scudo', '39-risonanza-impatto'],
            'impatto-dei-tre-eroi': ['36-fendente-impatto', '41-brace-impatto', '42-doppio-impatto'],
        }.items():
            if not all((dst / (name + '.png')).exists() for name in names):
                continue
            proof = Image.new('RGBA', (3 * 490, 490), (16, 24, 32, 255))
            for i, name in enumerate(names):
                proof.alpha_composite(Image.open(dst / (name + '.png')).convert('RGBA'), (i * 490 + 12, 12))
            proof.save(dst / (output + '.png'))
print(f'{len(shots)} previews -> {dst}')
