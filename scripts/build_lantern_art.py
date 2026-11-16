#!/usr/bin/env python3
"""Import the approved generated atlases into deterministic indexed C assets.

Mechanical cell slicing, pixel-grid resampling and palette conversion only;
the original PNGs and generation prompts stay intact under assets/lantern.
Run with --check in CI. There is no network or provider call in this importer.
"""
import argparse
import hashlib
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from build_art import Emitter

SOURCE = ROOT / 'assets/lantern/source'
OUTPUT = ROOT / 'firmware/rpg/ui/art'


def font_source(path, size, name):
    """Rasterize licensed font bytes at native size, including Italian accents."""
    font = ImageFont.truetype(str(path), size)
    chars = list(range(32, 127)) + list(range(160, 256)) + [0x2013, 0x2014, 0x2019, 0x2022, 0x2026]
    bitmap = []
    glyphs = ['{0}']
    for code in chars:
        ch = chr(code)
        left, top, right, bottom = font.getbbox(ch, anchor='ls')
        w, h = right - left, bottom - top
        glyphs.append(f'{{.bitmap_index={len(bitmap)},.adv_w={round(font.getlength(ch)*16)},.box_w={w},.box_h={h},.ofs_x={left},.ofs_y={-bottom}}}')
        if w and h:
            image = Image.new('L', (w, h))
            ImageDraw.Draw(image).text((-left, -top), ch, font=font, fill=255, anchor='ls')
            pixels = (np.array(image).flatten() // 17).tolist()
            if len(pixels) % 2: pixels.append(0)
            bitmap.extend((pixels[i] << 4) | pixels[i+1] for i in range(0, len(pixels), 2))
    data = ',\n'.join(','.join(str(v) for v in bitmap[i:i+40]) for i in range(0, len(bitmap), 40))
    offsets = ','.join(str(c - 160) for c in chars[95:])
    return (f'/* Generated from licensed DejaVu font; see assets/lantern/fonts/LICENSE.txt. */\n#include "lvgl.h"\n'
            f'static const uint8_t bitmap[] = {{\n{data}\n}};\n'
            'static const lv_font_fmt_txt_glyph_dsc_t glyphs[] = {\n' + ',\n'.join(glyphs) + '\n};\n'
            f'static const uint16_t extra[] = {{{offsets}}};\n'
            'static const lv_font_fmt_txt_cmap_t cmaps[] = {\n'
            '{.range_start=32,.range_length=95,.glyph_id_start=1,.type=LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY},\n'
            f'{{.range_start=160,.range_length={chars[-1]-159},.glyph_id_start=96,.unicode_list=extra,.list_length={len(chars)-95},.type=LV_FONT_FMT_TXT_CMAP_SPARSE_TINY}}\n}};\n'
            'static const lv_font_fmt_txt_dsc_t dsc = {.glyph_bitmap=bitmap,.glyph_dsc=glyphs,.cmaps=cmaps,.cmap_num=2,.bpp=4};\n'
            f'const lv_font_t {name} = {{.get_glyph_dsc=lv_font_get_glyph_dsc_fmt_txt,.get_glyph_bitmap=lv_font_get_bitmap_fmt_txt,.line_height={size+6},.base_line=5,.dsc=&dsc}};\n')


def indexed(im, size, transparent=False):
    im = im.convert('RGBA').resize(size, Image.Resampling.NEAREST)
    rgba = np.array(im)
    mask = rgba[:, :, 3] >= 128
    if transparent:
        assert 0.05 < float(mask.mean()) < 0.8, 'unexpected cell opacity/empty cell'
        # Ignore invisible RGB when selecting colors; reserve index zero for alpha.
        rgba[~mask, :3] = 0
    rgb = Image.fromarray(rgba[:, :, :3]).quantize(colors=240, method=Image.Quantize.MEDIANCUT,
                                                dither=Image.Dither.NONE).convert('RGB')
    pixels = np.array(rgb)
    if transparent:
        pixels = np.dstack((pixels, np.where(mask, 255, 0).astype(np.uint8)))
    return pixels


def atlas_rows(path, count):
    """One shared scale and foot anchor per row; never resize individual poses."""
    sheet = Image.open(path).convert('RGBA')
    result = []
    for r in range(count):
        crops = [sheet.crop((round(c * sheet.width / 4), round(r * sheet.height / count),
                             round((c + 1) * sheet.width / 4), round((r + 1) * sheet.height / count)))
                 for c in range(4)]
        boxes = []
        for im in crops:
            alpha = np.array(im)[:, :, 3] >= 128
            ys, xs = np.where(alpha)
            assert len(xs), 'empty generated pose'
            boxes.append((int(xs.min()), int(ys.min()), int(xs.max()) + 1, int(ys.max()) + 1))
        width = max(max(im.width / 2 - b[0], b[2] - im.width / 2) * 2 for im, b in zip(crops, boxes))
        top = min(b[1] for b in boxes)
        foot = max(b[3] for b in boxes)
        scale = min(80 / width, 80 / (foot - top))
        frames = []
        for im in crops:
            w, h = round(im.width * scale), round(im.height * scale)
            canvas = Image.new('RGBA', (88, 88))
            canvas.paste(im.resize((w, h), Image.Resampling.NEAREST),
                         (44 - round(im.width / 2 * scale), 84 - round(foot * scale)))
            frames.append(indexed(canvas, (88, 88), True))
        result.append(frames)
    return result


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()
    e = Emitter()
    env = Image.open(SOURCE / 'environments.png')
    assert env.width == env.height * 2, 'environment atlas must contain two squares'
    scenes = [e.sprite(f'ln_scene_{i}', indexed(env.crop((i * env.height, 0, (i + 1) * env.height, env.height)), (233, 233)))
              for i in range(2)]
    actors = Image.open(SOURCE / 'actors.png')
    assert actors.width % 4 == 0 and actors.height % 3 == 0
    w, h = actors.width // 4, actors.height // 3
    assert w == h, 'actor cells must have a common square anchor grid'
    rows = []
    for r in range(3):
        cells = []
        for c in range(4):
            crop = actors.crop((c * w, r * h, (c + 1) * w, (r + 1) * h))
            cells.append(e.sprite(f'ln_actor_{r}_{c}', indexed(crop, (88, 88), True)))
        rows.append('    {' + ','.join(cells) + '}')
    loadouts = []
    bestiary = []
    for name, count, destination in [('loadouts', 3, loadouts), ('bestiary', 5, bestiary)]:
        for r, frames in enumerate(atlas_rows(SOURCE / f'{name}.png', count)):
            cells = [e.sprite(f'ln_{name}_{r}_{c}', pixels) for c, pixels in enumerate(frames)]
            destination.append('    {' + ','.join(cells) + '}')
    relics = Image.open(SOURCE / 'relics.png')
    assert relics.width % 3 == 0 and relics.height % 2 == 0
    w, h = relics.width // 3, relics.height // 2
    rewards = []
    for r in range(2):
        for c in range(3):
            crop = relics.crop((c * w, r * h, (c + 1) * w, (r + 1) * h))
            rewards.append(e.sprite(f'ln_relic_{r * 3 + c}', indexed(crop, (88, 88), True)))
    header = ('/* Generated by scripts/build_lantern_art.py. */\n#pragma once\n#include "sprite.h"\n'
              'extern const wf_sprite ln_scenes[2];\nextern const wf_sprite ln_actors[3][4];\n'
              'extern const wf_sprite ln_relics[6];\n'
              'extern const wf_sprite ln_loadouts[3][4];\nextern const wf_sprite ln_bestiary[5][4];\n')
    source = ('/* Generated by scripts/build_lantern_art.py; see assets/lantern/README.md. */\n'
              '#include "lantern_art.h"\n' + '\n'.join(e.body) + '\n'
              'const wf_sprite ln_scenes[2] = {' + ','.join(scenes) + '};\n'
              'const wf_sprite ln_actors[3][4] = {\n' + ',\n'.join(rows) + '\n};\n'
              'const wf_sprite ln_relics[6] = {' + ','.join(rewards) + '};\n'
              'const wf_sprite ln_loadouts[3][4] = {\n' + ',\n'.join(loadouts) + '\n};\n'
              'const wf_sprite ln_bestiary[5][4] = {\n' + ',\n'.join(bestiary) + '\n};\n')
    manifest = {'source_hashes': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                 for p in sorted(SOURCE.glob('*.png'))},
                'font_hashes': {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in sorted((ROOT / 'assets/lantern/fonts').glob('*.ttf'))},
                'source_tool': 'OpenAI ImageGen', 'runtime_format': '8-bit indexed RGB565; binary alpha',
                'backgrounds': {'count': 2, 'grid': [233, 233], 'display_scale': 2},
                'actors': {'original': 3, 'loadouts': 3, 'additional_enemies': 5, 'frames_each': 4, 'grid': [88, 88], 'display_scale': 2},
                'relics': {'count': 6, 'grid': [88, 88], 'display_scale': 2},
                'transforms': ['equal-cell extraction', 'nearest-neighbor grid resampling',
                               'alpha threshold 128', '240-color palette without dithering', 'RGB565 encoding'],
                'status': 'Imported static art; run the native UI smoke check for integration evidence'}
    outputs = {OUTPUT / 'lantern_art.h': header, OUTPUT / 'lantern_art.c': source,
               ROOT / 'assets/lantern/manifest.json': json.dumps(manifest, indent=2) + '\n'}
    for role, size in [('body', 20), ('title', 22)]:
        outputs[OUTPUT / f'lantern_font_{role}.c'] = font_source(ROOT / f'assets/lantern/fonts/{role}.ttf', size, f'ln_font_{role}')
    stale = []
    for path, data in outputs.items():
        if args.check:
            if not path.exists() or path.read_text() != data: stale.append(str(path.relative_to(ROOT)))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(data)
    if stale:
        sys.exit('stale lantern art: ' + ', '.join(stale))
    print(f'lantern art: {e.count} assets ' + ('up to date' if args.check else 'imported'))


if __name__ == '__main__':
    main()
