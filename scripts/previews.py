#!/usr/bin/env python3
"""Turns the host preview's PPM screenshots into round PNGs and a gallery.

  python3 scripts/previews.py <ppm_dir> <png_dir>
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

src, dst = Path(sys.argv[1]), Path(sys.argv[2])
dst.mkdir(parents=True, exist_ok=True)
mask = Image.new('L', (466, 466), 0)
ImageDraw.Draw(mask).ellipse((0, 0, 465, 465), fill=255)
shots = []
for ppm in sorted(src.glob('*.ppm')):
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
print(f'{len(shots)} previews -> {dst}')
