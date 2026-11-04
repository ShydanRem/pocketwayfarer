"""RPG monsters: archetypes from the v0.1 bestiary plus new fantasy ones.
Species order is read from `enum` in firmware/rpg/core/rpg.h (SP_*)."""
import math

from canvas import Canvas, eye, brow
import foes as F

INK = '#1d1430'


def _c(s, mats):
    return Canvas(s, s, mats)


def wolf(frame, s, fur, belly='#e8e0d0', eyes='#ffd23f'):
    c = _c(s, {'f': fur, 'b': belly, 'n': '#2a2028'})
    k = s / 44
    c.ellipse(30 * k, 30 * k, 11 * k, 7 * k, 'f')                       # body
    c.line([(40 * k, 28 * k), (43 * k, 22 * k)], 'f', width=3 * k)     # tail
    for x in (24, 34):
        c.rect(x * k, 33 * k, 3 * k, 8 * k, 'f', light=0.45)
    c.ellipse(16 * k, 24 * k, 10 * k, 8.5 * k, 'f')                     # head
    c.poly([(9 * k, 19 * k), (8 * k, 9 * k), (14 * k, 16 * k)], 'f')
    c.poly([(21 * k, 18 * k), (24 * k, 9 * k), (17 * k, 16 * k)], 'f')
    c.ellipse(12 * k, 29 * k, 5.5 * k, 3.5 * k, 'b', patch=True)
    c.circle(8.5 * k, 28 * k, 1.6 * k, 'n')
    ang = frame == 'attack'
    for x in (12, 19):
        eye(c, int(x * k) - 1, int(21 * k), 3, 4, 'angry' if ang else 'open', iris=eyes)
    if ang:
        c.pxs(int(9 * k), int(31 * k), ['W.W.W'], {'W': '#ffffff'})
    return c.render()


def skeleton(frame, s, crown=False):
    k = s / 44
    c = _c(s, {'b': '#f0ead8', 'd': '#c8bca0', 'gold': '#ffcc33', 'w': '#9aa0b0'})
    for x in (17, 27):
        c.rect(x * k, 32 * k, 3 * k, 10 * k, 'd', light=0.6)
    c.rect(15 * k, 22 * k, 14 * k, 11 * k, 'd', light=0.55)
    for y in (24, 27, 30):
        c.rect(16 * k, y * k, 12 * k, 1.2 * k, 'b', light=0.9)
    c.line([(15 * k, 23 * k), (9 * k, 32 * k)], 'd', width=2.4 * k)
    c.line([(29 * k, 23 * k), (35 * k, 32 * k)], 'd', width=2.4 * k)
    c.line([(36 * k, 36 * k), (38 * k, 10 * k)], 'w', width=2 * k, light=0.85)   # sword
    c.ellipse(22 * k, 13 * k, 9 * k, 8.5 * k, 'b')
    if crown:
        c.poly([(14 * k, 7 * k), (14 * k, 1 * k), (18 * k, 4 * k), (22 * k, 0), (26 * k, 4 * k), (30 * k, 1 * k), (30 * k, 7 * k)], 'gold', light=0.8)
    glow = '#ff4a4a' if frame == 'attack' else '#7af8ff'
    for x in (17, 24):
        c.pxs(int(x * k), int(11 * k), ['KKK', 'KGK', 'KKK'], {'K': INK, 'G': glow})
    c.pxs(int(19 * k), int(18 * k), ['K.K.K.K'], {'K': INK})
    return c.render()


def ghost(frame, s, body='#c8d8ff', eyes=None):
    k = s / 44
    c = _c(s, {'g': body})
    wob = 2 if frame == 'attack' else 0
    c.ellipse(22 * k, 18 * k, 13 * k, 14 * k, 'g', bulge=0.7)
    c.rect(9 * k, 18 * k, 26 * k, 16 * k, 'g', light=0.55)
    for i, x in enumerate((12, 18.5, 25, 31.5)):
        c.ellipse(x * k, (34 + (wob if i % 2 else 0)) * k, 3.6 * k, 4.5 * k, 'g', light=0.5)
    kind = 'angry' if frame == 'attack' else 'open'
    eye(c, int(15 * k), int(15 * k), 4, 5, kind, ink='#1a1040', iris=eyes or '#1a1040')
    eye(c, int(25 * k), int(15 * k), 4, 5, kind, ink='#1a1040', iris=eyes or '#1a1040', mirror=True)
    c.pxs(int(19 * k), int(24 * k), ['.KKK.', 'K...K', '.KKK.'] if frame == 'attack' else ['.KK.', 'K..K', '.KK.'], {'K': '#1a1040'})
    return c.render()


def mimic(frame, s):
    k = s / 44
    c = _c(s, {'w': '#a8703a', 'm': '#ffcc33', 't': '#ffffff', 'r': '#c83a4a'})
    open_ = frame == 'attack'
    c.rect(6 * k, 22 * k, 32 * k, 18 * k, 'w', round_=2 * k)
    c.rect(6 * k, 22 * k, 32 * k, 3 * k, 'm', light=0.8)
    c.rect(20 * k, 26 * k, 4 * k, 5 * k, 'm', light=0.9)
    lid_y = 8 if open_ else 14
    c.rect(6 * k, lid_y * k, 32 * k, 9 * k, 'w', round_=3 * k, light=0.75)
    c.rect(6 * k, (lid_y + 6) * k, 32 * k, 2 * k, 'm', light=0.8)
    if open_:
        c.rect(8 * k, 17 * k, 28 * k, 5 * k, 'r', light=0.4)
    for x in range(8, 36, 4):
        c.poly([(x * k, 22 * k), ((x + 2) * k, 18 * k), ((x + 4) * k, 22 * k)], 't', light=0.95)
    c.pxs(int(14 * k), int(26 * k), ['Y.....Y'], {'Y': '#fff04a'})
    return c.render()


def goblin(frame, s, skin='#7ab84a', cloth='#8a4a3a', hat=None, weapon='#c8c8d8'):
    k = s / 44
    c = _c(s, {'s': skin, 'c': cloth, 'h': hat or cloth, 'w': weapon, 'b': '#5a3a2a'})
    c.rect(15 * k, 34 * k, 5 * k, 8 * k, 'b', light=0.5)
    c.rect(24 * k, 34 * k, 5 * k, 8 * k, 'b', light=0.5)
    c.ellipse(22 * k, 30 * k, 9 * k, 7 * k, 'c')
    c.line([(32 * k, 32 * k), (38 * k, 14 * k)], 'w', width=2.4 * k, light=0.9)
    c.circle(32 * k, 32 * k, 2.5 * k, 's')
    c.circle(12 * k, 31 * k, 2.5 * k, 's')
    c.poly([(11 * k, 16 * k), (1 * k, 12 * k), (12 * k, 22 * k)], 's')
    c.poly([(33 * k, 16 * k), (43 * k, 12 * k), (32 * k, 22 * k)], 's')
    c.ellipse(22 * k, 18 * k, 10.5 * k, 9 * k, 's')
    if hat:
        c.ellipse(22 * k, 11 * k, 11 * k, 5 * k, 'h', clip=lambda xs, ys: ys < 12 * k)
    kind = 'angry' if frame == 'attack' else 'open'
    eye(c, int(16 * k), int(15 * k), 3, 4, kind, iris='#ffd23f')
    eye(c, int(25 * k), int(15 * k), 3, 4, kind, iris='#ffd23f', mirror=True)
    c.pxs(int(18 * k), int(22 * k), ['KWKKWK' if frame == 'attack' else 'K....K', '.KKKK.'], {'K': INK, 'W': '#ffffff'})
    return c.render()


def serpent(frame, s, body, belly='#ffe8a0'):
    k = s / 44
    c = _c(s, {'b': body, 'y': belly})
    c.line([(6 * k, 40 * k), (16 * k, 36 * k), (28 * k, 40 * k), (36 * k, 34 * k), (34 * k, 24 * k)], 'b', width=7 * k)
    c.line([(16 * k, 37 * k), (28 * k, 41 * k)], 'y', width=2.5 * k)
    c.ellipse(28 * k, 17 * k, 9 * k, 7 * k, 'b')
    c.poly([(20 * k, 10 * k), (24 * k, 4 * k), (27 * k, 11 * k)], 'b')
    c.poly([(33 * k, 10 * k), (36 * k, 3 * k), (36 * k, 12 * k)], 'b')
    kind = 'angry' if frame == 'attack' else 'open'
    eye(c, int(23 * k), int(14 * k), 3, 4, kind, iris='#ff4a4a')
    eye(c, int(31 * k), int(14 * k), 3, 4, kind, iris='#ff4a4a', mirror=True)
    if frame == 'attack':
        c.line([(27 * k, 23 * k), (26 * k, 28 * k)], 'b', width=1)
        c.pxs(int(25 * k), int(22 * k), ['RR', '.R'], {'R': '#ff4a6a'})
    return c.render()


def spider(frame, s, body, mark='#ff3a4a', queen=False):
    k = s / 44
    c = _c(s, {'b': body, 'l': body, 'm': mark, 'gold': '#ffcc33'})
    lift = -3 if frame == 'attack' else 0
    for side in (-1, 1):
        for i, (dx, dy) in enumerate(((12, -6), (15, 0), (14, 6), (11, 11))):
            y0 = 26 + i * 2
            c.line([(22 * k, y0 * k), ((22 + side * dx) * k, (y0 + dy - 6 + lift) * k), ((22 + side * (dx + 6)) * k, (y0 + dy + 6) * k)], 'l', width=1.8 * k, light=0.45)
    c.ellipse(22 * k, 30 * k, 10 * k, 8 * k, 'b')
    c.pxs(int(20 * k), int(29 * k), ['.M.', 'MMM', '.M.'], {'M': mark})
    c.ellipse(22 * k, 19 * k, 7 * k, 6 * k, 'b')
    if queen:
        c.poly([(16 * k, 13 * k), (16 * k, 7 * k), (19 * k, 10 * k), (22 * k, 6 * k), (25 * k, 10 * k), (28 * k, 7 * k), (28 * k, 13 * k)], 'gold', light=0.8)
    for x, y in ((18, 17), (25, 17), (20, 20), (23, 20)):
        c.pxs(int(x * k), int(y * k), ['R'], {'R': '#ff4a4a' if frame == 'attack' else '#ffd23f'})
    return c.render()


def imp(frame, s, skin='#d8402a', wing='#6a1a2a', lord=False):
    k = s / 44
    c = _c(s, {'s': skin, 'w': wing, 'h': '#f0e0c0', 'f': '#ffb02e', 'gold': '#ffcc33'})
    up = -4 if frame == 'attack' else 0
    for side in (-1, 1):
        c.poly([((22 + side * 6) * k, 22 * k), ((22 + side * 20) * k, (8 + up) * k), ((22 + side * 18) * k, 26 * k), ((22 + side * 10) * k, 28 * k)], 'w', light=0.45)
    c.line([(28 * k, 34 * k), (38 * k, 38 * k), (40 * k, 33 * k)], 's', width=2 * k)
    c.ellipse(22 * k, 30 * k, 8 * k, 8 * k, 's')
    c.ellipse(22 * k, 17 * k, 9 * k, 8 * k, 's')
    c.poly([(15 * k, 12 * k), (11 * k, 2 * k), (18 * k, 9 * k)], 'h', light=0.85)
    c.poly([(29 * k, 12 * k), (33 * k, 2 * k), (26 * k, 9 * k)], 'h', light=0.85)
    if lord:
        c.flame(22 * k, 8 * k, 6 * k, 8 * k, 'f')
    kind = 'angry' if frame == 'attack' else 'open'
    eye(c, int(17 * k), int(14 * k), 3, 4, kind, iris='#ffe04a')
    eye(c, int(24 * k), int(14 * k), 3, 4, kind, iris='#ffe04a', mirror=True)
    c.pxs(int(18 * k), int(21 * k), ['K.WW.K', '.KKKK.'], {'K': INK, 'W': '#ffffff'})
    return c.render()


def knight(frame, s, armour, cloth, plume, holy=False):
    k = s / 44
    c = _c(s, {'a': armour, 'c': cloth, 'p': plume, 'w': '#e8eef8', 'g': '#ffcc33'})
    c.rect(15 * k, 33 * k, 5 * k, 9 * k, 'a', light=0.5)
    c.rect(24 * k, 33 * k, 5 * k, 9 * k, 'a', light=0.5)
    c.poly([(12 * k, 20 * k), (32 * k, 20 * k), (30 * k, 36 * k), (14 * k, 36 * k)], 'c', light=0.5)
    c.rect(14 * k, 20 * k, 16 * k, 12 * k, 'a', round_=3 * k)
    c.ellipse(12 * k, 21 * k, 4 * k, 3 * k, 'a')
    c.ellipse(32 * k, 21 * k, 4 * k, 3 * k, 'a')
    c.line([(36 * k, 36 * k), (38 * k, 6 * k)], 'w', width=2.5 * k, light=0.9)
    c.line([(34 * k, 30 * k), (40 * k, 30 * k)], 'g', width=1.5 * k)
    c.poly([(4 * k, 22 * k), (14 * k, 22 * k), (14 * k, 32 * k), (9 * k, 37 * k), (4 * k, 32 * k)], 'c', light=0.7)
    c.ellipse(22 * k, 12 * k, 8 * k, 8.5 * k, 'a')
    c.flame(22 * k, 5 * k, 5 * k, 8 * k, 'p')
    glow = '#ff4a4a' if frame == 'attack' else ('#fff8a0' if holy else '#8af8ff')
    c.rect(16 * k, 11 * k, 12 * k, 2 * k, 'c', light=0.2)
    c.pxs(int(17 * k), int(11 * k), ['GG....GG'], {'G': glow})
    return c.render()


def fairy(frame, s, body='#8a4ae8', wing='#d8b8ff'):
    k = s / 44
    c = _c(s, {'b': body, 'w': wing, 'h': '#2a1a3a', 'g': '#c8ff8a'})
    up = -3 if frame == 'attack' else 0
    c.ellipse(13 * k, (16 + up) * k, 8 * k, 5 * k, 'w', angle=-30, light=0.9)
    c.ellipse(31 * k, (16 + up) * k, 8 * k, 5 * k, 'w', angle=30, light=0.9)
    c.ellipse(14 * k, 27 * k, 6 * k, 3.5 * k, 'w', angle=20, light=0.9)
    c.ellipse(30 * k, 27 * k, 6 * k, 3.5 * k, 'w', angle=-20, light=0.9)
    c.poly([(18 * k, 24 * k), (26 * k, 24 * k), (29 * k, 38 * k), (15 * k, 38 * k)], 'b', light=0.6)
    c.ellipse(22 * k, 16 * k, 7 * k, 7 * k, '#f0d8e8' if False else 'w')
    c.ellipse(22 * k, 12 * k, 8 * k, 5 * k, 'h', clip=lambda xs, ys: ys < 14 * k)
    kind = 'angry' if frame == 'attack' else 'open'
    eye(c, int(18 * k), int(15 * k), 3, 4, kind, iris='#ff4ae8')
    eye(c, int(24 * k), int(15 * k), 3, 4, kind, iris='#ff4ae8', mirror=True)
    for x, y in ((6, 6), (38, 10), (36, 36)):
        c.px(int(x * k), int(y * k), '#c8ff8a')
    return c.render()


def harpy(frame, s):
    k = s / 44
    c = _c(s, {'f': '#c87a5a', 'w': '#8a5a8a', 's': '#f0c8a8', 'h': '#5a2a5a', 'y': '#ffb03a'})
    up = -5 if frame == 'attack' else 0
    for side in (-1, 1):
        c.poly([((22 + side * 5) * k, 22 * k), ((22 + side * 21) * k, (10 + up) * k), ((22 + side * 19) * k, 30 * k), ((22 + side * 8) * k, 30 * k)], 'w', light=0.5)
    c.ellipse(22 * k, 30 * k, 7 * k, 8 * k, 'f')
    c.line([(19 * k, 37 * k), (17 * k, 42 * k)], 'y', width=1.5 * k)
    c.line([(25 * k, 37 * k), (27 * k, 42 * k)], 'y', width=1.5 * k)
    c.ellipse(22 * k, 16 * k, 7 * k, 7 * k, 's')
    c.ellipse(22 * k, 13 * k, 9 * k, 6 * k, 'h', clip=lambda xs, ys: ys < 15 * k)
    c.ellipse(14 * k, 20 * k, 3 * k, 6 * k, 'h')
    c.ellipse(30 * k, 20 * k, 3 * k, 6 * k, 'h')
    kind = 'angry' if frame == 'attack' else 'open'
    eye(c, int(17 * k), int(15 * k), 3, 4, kind, iris='#ffd23f')
    eye(c, int(24 * k), int(15 * k), 3, 4, kind, iris='#ffd23f', mirror=True)
    return c.render()


def recolor(img, mapping):
    """Hue-shift helper for variants: mapping old rgb -> new rgb is approximate, so
    we rotate hue for all opaque pixels instead."""
    import colorsys
    import numpy as np
    out = img.copy()
    dh, ds, dv = mapping
    h, w = img.shape[:2]
    for y in range(h):
        for x in range(w):
            if out[y, x, 3] == 0:
                continue
            r, g, b = [v / 255 for v in out[y, x, :3]]
            hh, ss, vv = colorsys.rgb_to_hsv(r, g, b)
            if ss < 0.12:      # keep whites/greys and outlines
                continue
            hh = (hh + dh) % 1
            ss = min(1, ss * ds)
            vv = min(1, vv * dv)
            out[y, x, :3] = [round(v * 255) for v in colorsys.hsv_to_rgb(hh, ss, vv)]
    return out


S, B = 44, 60
PAINTERS = {
    'SLIME': lambda f: F.slime(f, S, '#7ee06d'),
    'SHROOM': lambda f: F.shroom(f, S, '#e85050'),
    'WOLF': lambda f: wolf(f, S, '#8a8a9a'),
    'WISP': lambda f: F.wisp(f, S, '#ff8a3a', '#ffe066'),
    'SKELETON': lambda f: skeleton(f, S),
    'BAT': lambda f: F.bat(f, S, '#7a5cc4', '#4b3590'),
    'GHOST': lambda f: ghost(f, S),
    'MIMIC': lambda f: mimic(f, S),
    'SLIME_KING': lambda f: F.slime(f, B, '#5fd06a', crown=True),
    'BONE_KING': lambda f: skeleton(f, B, crown=True),
    'CRAB': lambda f: F.crab(f, S, '#ff6f5e'),
    'JELLY': lambda f: F.jelly(f, S, '#d08cff'),
    'BLUE_SLIME': lambda f: F.slime(f, S, '#58a8ff'),
    'GOBLIN': lambda f: goblin(f, S, cloth='#3a5ab8', hat='#c83a3a'),
    'SERPENT': lambda f: serpent(f, S, '#2a9ab8'),
    'DEEP_GHOST': lambda f: ghost(f, S, '#7ad8c8', '#0a3a4a'),
    'CRAB_KING': lambda f: F.crab(f, B, '#e8484a', boss=True),
    'LEVIATHAN': lambda f: recolor(F.dragon(f, B, '#3a7ab8', '#8ae8ff', '#2a4a8a'), (0, 1, 1)),
    'TREANT': lambda f: F.golem(f, S, '#8b6a3e', '#9dff7a', leaves=True),
    'OWL': lambda f: F.owl(f, S, '#6f7fb5', '#c8d4f0', moon=True),
    'DIRE_WOLF': lambda f: wolf(f, S, '#3a3448', '#8a8aa0', '#ff4a4a'),
    'SPIDER': lambda f: spider(f, S, '#5a4a3a'),
    'TOXIC_SHROOM': lambda f: F.shroom(f, S, '#9a4ae8', '#e8ffd0'),
    'DARK_FAIRY': lambda f: fairy(f, S),
    'ELDER_STUMP': lambda f: F.golem(f, B, '#6e5232', '#c8ff5a', leaves=True, boss=True),
    'SPIDER_QUEEN': lambda f: spider(f, B, '#2a2a3a', '#c83ae8', queen=True),
    'SALAMANDER': lambda f: F.lizard(f, S, '#ff6a3d', '#ffd23f'),
    'LAVA_GOLEM': lambda f: F.golem(f, S, '#5b4a58', '#ffcf3f', accent='#ff7a1a'),
    'FIRE_BAT': lambda f: F.bat(f, S, '#4a3040', '#c83a2a', flame=True),
    'GOBLIN_MINER': lambda f: goblin(f, S, skin='#8ab84a', cloth='#6a5a3a', hat='#ffcc33', weapon='#8a8a9a'),
    'IMP': lambda f: imp(f, S),
    'HELL_KNIGHT': lambda f: knight(f, S, '#3a2a3a', '#8a1a2a', '#ff4a1a'),
    'ASH_DRAGON': lambda f: F.dragon(f, B, '#6c6478', '#ff8a45', '#a83a3a'),
    'DEMON_LORD': lambda f: imp(f, B, '#a81a3a', '#2a0a1a', lord=True),
    'STORM_WISP': lambda f: F.wisp(f, S, '#6ab8ff', '#e8f8ff'),
    'GRIFFIN': lambda f: F.bird(f, S, '#e8c060', '#a87a3a'),
    'CRYSTAL': lambda f: F.crystal(f, S, '#b48cff'),
    'HARPY': lambda f: harpy(f, S),
    'SKY_KNIGHT': lambda f: knight(f, S, '#e8eef8', '#3a6ad8', '#ffd23f', holy=True),
    'CRYSTAL_GOLEM': lambda f: F.golem(f, S, '#8ad8e8', '#3a4ad8'),
    'GUARDIAN': lambda f: F.crystal(f, B, '#fff0b0', halo=True),
    'STAR_DRAGON': lambda f: F.dragon(f, B, '#e8e0ff', '#ffd860', '#8a7ae8'),
}


def species_order(header_text):
    import re
    body = header_text[header_text.index('SP_SLIME,'):header_text.index('SP_COUNT')]
    return re.findall(r'SP_([A-Z_]+)', body)
