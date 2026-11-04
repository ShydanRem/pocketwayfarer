#!/usr/bin/env python3
"""Generates the RPG art (firmware/rpg/ui/art) from scripts/art/rpg_*.py.

  python3 scripts/build_rpg_art.py            # regenerate + previews in docs/img/rpg
  python3 scripts/build_rpg_art.py --check    # fail if the committed output is stale
"""
import argparse
import sys
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts' / 'art'))
sys.path.insert(0, str(ROOT / 'scripts'))

from build_art import Emitter  # noqa: E402
import rpg_hero as H  # noqa: E402
import rpg_icons as I  # noqa: E402
import rpg_monsters as M  # noqa: E402
import rpg_scenes as SC  # noqa: E402
sys.path.insert(0, str(ROOT / 'scripts' / 'art' / 'hd'))
import hd2d  # noqa: E402

OUT = ROOT / 'firmware' / 'rpg' / 'ui' / 'art'
PREVIEW = ROOT / 'docs' / 'img' / 'rpg'
HEADER = ROOT / 'firmware' / 'rpg' / 'core' / 'rpg.h'


class LayerEmitter(Emitter):
    def layer(self, name, img):
        """Cropped sprite + offset; None/empty -> zero-size layer."""
        if img is None or not (img[:, :, 3] > 0).any():
            return '{{0, 0, 0, 0, 0, 0}, 0, 0}'
        ys, xs = np.where(img[:, :, 3] > 0)
        y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
        crop = img[y0:y1, x0:x1]
        return f'{{{self.sprite(name, crop)}, {x0}, {y0}}}'


def table(ctype, name, dims, cells):
    """cells: nested lists of initialisers."""
    def fmt(c):
        if isinstance(c, list):
            return '{' + ', '.join(fmt(x) for x in c) + '}'
        return c
    d = ''.join(f'[{x}]' for x in dims)
    return f'const {ctype} {name}{d} = ' + fmt(cells) + ';'


def build():
    e = LayerEmitter()
    t = []
    L = 'wf_layer'
    t.append(table(L, 'rpg_body', ['RACE_COUNT', '5'],
                   [[e.layer(f'body{r}_{s}', H.body(r, s)) for s in range(5)] for r in range(4)]))
    exprs = ['idle', 'blink', 'hurt', 'happy']
    t.append(table(L, 'rpg_face', ['4', '4', '2'],
                   [[[e.layer(f'face{c}_{x}_{l}', H.face(c, ex, bool(l))) for l in range(2)] for x, ex in enumerate(exprs)]
                    for c in range(4)]))
    t.append(table(L, 'rpg_hair_front', ['6', '8'],
                   [[e.layer(f'hairf{s}_{c}', H.hair_front(st, c)) for c in range(8)] for s, st in enumerate(H.STYLES)]))
    t.append(table(L, 'rpg_hair_back', ['6', '8'],
                   [[e.layer(f'hairb{s}_{c}', H.hair_back(st, c)) for c in range(8)] for s, st in enumerate(H.STYLES)]))
    t.append(table(L, 'rpg_ears', ['8'], [e.layer(f'ears{c}', H.beast_ears(c)) for c in range(8)]))
    t.append(table(L, 'rpg_tail', ['8'], [e.layer(f'tail{c}', H.tail(c)) for c in range(8)]))
    t.append(table(L, 'rpg_beard', ['8'], [e.layer(f'beard{c}', H.beard(c)) for c in range(8)]))
    t.append(table(L, 'rpg_outfit', ['FAM_COUNT', '3'],
                   [[e.layer(f'outfit{f}_{k}', H.outfit(f, k)) for k in range(3)] for f in range(8)]))
    t.append(table(L, 'rpg_cape', ['FAM_COUNT', '3'],
                   [[e.layer(f'cape{f}_{k}', H.cape(f, k)) for k in range(3)] for f in range(8)]))
    t.append(table(L, 'rpg_hat', ['FAM_COUNT', '3'],
                   [[e.layer(f'hat{f}_{k}', H.class_hat(f, k)) for k in range(3)] for f in range(8)]))
    t.append(table(L, 'rpg_helm', ['3', 'RPG_TIERS'],
                   [[e.layer(f'helm{l}_{k}', H.helm(l, k)) for k in range(5)] for l in range(3)]))
    t.append(table(L, 'rpg_weapon', ['9', 'RPG_TIERS'],
                   [[e.layer(f'weap{w}_{k}', H.weapon(w, k)) for k in range(5)] for w in range(9)]))
    t.append(table(L, 'rpg_offhand', ['4', 'RPG_TIERS'],
                   [[e.layer(f'off{w}_{k}', H.offhand(w, k)) for k in range(5)] for w in range(4)]))
    order = M.species_order(HEADER.read_text())
    t.append(table('wf_sprite', 'rpg_monsters', ['SP_COUNT', '2'],
                   [[e.sprite(f'mon{n}_{k}', M.PAINTERS[name](fr)) for k, fr in enumerate(['idle', 'attack'])]
                    for n, name in enumerate(order)]))
    t.append(table('wf_sprite', 'rpg_item_icons', ['IT_TYPE_COUNT', 'RPG_TIERS'],
                   [[e.sprite(f'it{n}_{k}', f(k).render()) for k in range(5)] for n, f in enumerate(I.ITEM_TYPES)]))
    t.append(table('wf_sprite', 'rpg_cons_icons', ['CO_COUNT'],
                   [e.sprite(f'co{n}', f().render()) for n, f in enumerate(I.CONSUMABLES)]))
    t.append(table('wf_sprite', 'rpg_room_icons', ['ROOM_KIND_COUNT'],
                   ['{0, 0, 0, 0, 0, 0}' if f is None else e.sprite(f'room{n}', f().render()) for n, f in enumerate(I.ROOMS)]))
    t.append(table('wf_sprite', 'rpg_element_icons', ['EL_COUNT'],
                   [e.sprite(f'el{n}', I.element(n).render()) for n in range(7)]))
    t.append(table('wf_sprite', 'rpg_skill_icons', ['11'],
                   [e.sprite(f'sk{n}', f().render()) for n, f in enumerate(I.SKILL_KINDS)]))
    t.append(table('wf_sprite', 'rpg_ui_icons', ['RI_COUNT'],
                   [e.sprite(f'ui_{name}', f().render()) for name, f in I.UI]))
    from PIL import Image
    hd = hd2d.all_scenes()
    scenes = []
    for n, img, meta in hd:
        q = Image.fromarray(img, 'RGB').quantize(colors=250, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG)
        scenes.append((n, np.array(q.convert('RGB')), meta))
    t.append(table('wf_sprite', 'rpg_scenes', ['RS_COUNT'], [e.sprite(f'scene_{n}', img) for n, img, _ in scenes]))
    fx_rows = []
    for n, img, meta in scenes:
        lights = ', '.join(f'{{{x}, {y}, {r}, 0x{col.lstrip("#")}, {fl}}}' for x, y, r, col, fl in meta['glows'][:12])
        col = '0x%02x%02x%02x' % tuple(meta['color'])
        fx_rows.append(f'    {{{meta["particles"]}, {col}, {min(12, len(meta["glows"]))}, {{{lights}}}}}')
    t.append('const rpg_scene_fx rpg_scene_fxs[RS_COUNT] = {\n' + ',\n'.join(fx_rows) + '\n};')

    h = ['/* Generated by scripts/build_rpg_art.py — do not edit. */', '#pragma once', '#include "sprite.h"',
         '#include "rpg.h"', '',
         '#include "fx.h"',
         'typedef struct { uint8_t particles; uint32_t color; uint8_t nlights; fx_light lights[12]; } rpg_scene_fx;',
         '/* Hero layer: cropped sprite drawn at (x, y) on the 48x48 hero grid. */',
         'typedef struct { wf_sprite s; int8_t x, y; } wf_layer;', '',
         'enum {'] + [f'    RI_{n.upper()},' for n, _ in I.UI] + ['    RI_COUNT', '};', 'enum {'] + \
        [f'    RS_{n.upper()},' for n, _, _m in scenes] + ['    RS_COUNT', '};', '',
                                                     '/* Map spots in screen pixels: five zones, then the town. */',
                                                     'static const uint16_t rpg_world_spots[6][2] = {' +
                                                     ', '.join(f'{{{x * 2}, {y * 2}}}' for x, y in hd2d.ISLANDS + [hd2d.TOWN_SPOT]) + '};', '',
                                                     'extern const wf_layer rpg_body[RACE_COUNT][5];',
                                                     'extern const wf_layer rpg_face[4][4][2];',
                                                     'extern const wf_layer rpg_hair_front[6][8];',
                                                     'extern const wf_layer rpg_hair_back[6][8];',
                                                     'extern const wf_layer rpg_ears[8];',
                                                     'extern const wf_layer rpg_tail[8];',
                                                     'extern const wf_layer rpg_beard[8];',
                                                     'extern const wf_layer rpg_outfit[FAM_COUNT][3];',
                                                     'extern const wf_layer rpg_cape[FAM_COUNT][3];',
                                                     'extern const wf_layer rpg_hat[FAM_COUNT][3];',
                                                     'extern const wf_layer rpg_helm[3][RPG_TIERS];',
                                                     'extern const wf_layer rpg_weapon[9][RPG_TIERS];',
                                                     'extern const wf_layer rpg_offhand[4][RPG_TIERS];',
                                                     'extern const wf_sprite rpg_monsters[SP_COUNT][2];',
                                                     'extern const wf_sprite rpg_item_icons[IT_TYPE_COUNT][RPG_TIERS];',
                                                     'extern const wf_sprite rpg_cons_icons[CO_COUNT];',
                                                     'extern const wf_sprite rpg_room_icons[ROOM_KIND_COUNT];',
                                                     'extern const wf_sprite rpg_element_icons[EL_COUNT];',
                                                     'extern const wf_sprite rpg_skill_icons[11];',
                                                     'extern const wf_sprite rpg_ui_icons[RI_COUNT];',
                                                     'extern const wf_sprite rpg_scenes[RS_COUNT];',
                                                     'extern const rpg_scene_fx rpg_scene_fxs[RS_COUNT];', '']
    src = ['/* Generated by scripts/build_rpg_art.py — do not edit. */', '#include "rpg_art.h"', ''] + e.body + [''] + t + ['']
    return '\n'.join(h), '\n'.join(src), scenes, order


def previews(scenes, order):
    from PIL import Image, ImageDraw
    PREVIEW.mkdir(parents=True, exist_ok=True)
    bg = (28, 30, 52, 255)
    W = [0, 2, 3, 4, 5, 6, 7, 8]
    OFF = [0, 2, 1, 3, 0, 3, 0, 2]
    sheet = Image.new('RGBA', (8 * 52, 3 * 52), bg)
    for f in range(8):
        for k in range(3):
            im = H.hero_preview(race=(f + k) % 4, skin=(f + k) % 5, eyes=f % 4, hair_style=(f + k) % 6,
                                hair_color=(f * 3 + k) % 8, fam=f, tier=k, weapon_kind=W[f], off=OFF[f])
            sheet.alpha_composite(Image.fromarray(im, 'RGBA'), (f * 52 + 2, k * 52 + 2))
    sheet.resize((sheet.width * 3, sheet.height * 3), Image.NEAREST).save(PREVIEW / 'eroi.png')
    sheet = Image.new('RGBA', (7 * 64, 6 * 64), bg)
    for n, name in enumerate(order):
        im = Image.fromarray(M.PAINTERS[name]('idle'), 'RGBA')
        sheet.alpha_composite(im, ((n % 7) * 64 + (64 - im.width) // 2, (n // 7) * 64 + (64 - im.height) // 2))
    sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST).save(PREVIEW / 'mostri.png')
    sheet = Image.new('RGB', (5 * 240, 3 * 240), (0, 0, 0))
    mask = Image.new('L', (233, 233), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, 232, 232), fill=255)
    for n, (_, img, _m) in enumerate(scenes):
        sheet.paste(Image.fromarray(img, 'RGB'), ((n % 5) * 240 + 3, (n // 5) * 240 + 3), mask)
    sheet.save(PREVIEW / 'scenari.png')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    ap.add_argument('--no-preview', action='store_true')
    args = ap.parse_args()
    header, source, scenes, order = build()
    files = {OUT / 'rpg_art.h': header, OUT / 'rpg_art.c': source}
    stale = [p for p, txt in files.items() if not p.exists() or p.read_text() != txt]
    if args.check:
        if stale:
            sys.exit('stale rpg art: ' + ', '.join(str(p.relative_to(ROOT)) for p in stale))
        print('rpg art: up to date')
        return
    OUT.mkdir(parents=True, exist_ok=True)
    for p in stale:
        p.write_text(files[p])
    if not args.no_preview:
        previews(scenes, order)
    for p in files:
        print(f'{p.relative_to(ROOT)}: {p.stat().st_size // 1024} KB')


if __name__ == '__main__':
    main()
