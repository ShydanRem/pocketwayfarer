"""Paper-doll hero on a 48x48 grid: every layer is drawn separately and composed
on the device in this order:

  back (cape, quiver, tail, long hair) -> body (race, skin) -> face (eyes, expression)
  -> outfit (family x tier) -> beard -> hair -> beast ears -> hat / helm
  -> weapon (right hand, screen left) -> off-hand (left hand, screen right)
"""
from canvas import Canvas, eye

S = 48
SKINS = ['#ffe0c4', '#f2c29a', '#d89a6a', '#a8673f', '#6e4128']
HAIR = ['#3a2a24', '#7a4a2a', '#d8a050', '#f4e2a0', '#c84a2a', '#e8e8f0', '#4a7ae8', '#e85aa8']
EYES = ['#6a3a1a', '#3a7ad8', '#3a9a4a', '#8a4ad8']
OUTLINE = '#2a1a2a'

# family palettes: (main, second, trim) for tiers 1..3
FAMILY = {
    0: [('#8a94a8', '#c84a3a', '#d8b860'), ('#a8b4c8', '#3a5ab8', '#ffd860'), ('#d8dce8', '#b82a3a', '#ffe070')],  # warrior
    1: [('#4a5ab8', '#8a5ad8', '#e8d070'), ('#3a3a98', '#e8604a', '#ffe070'), ('#5a2a98', '#3ad8e8', '#fff0a0')],  # mage
    2: [('#4a8a3a', '#8a6a3a', '#c8b060'), ('#3a6a3a', '#a87a4a', '#e0d080'), ('#2a5a4a', '#e8e8f0', '#ffe070')],  # archer
    3: [('#3a3a4a', '#8a2a3a', '#a8a8b8'), ('#2a2a3a', '#5a2a6a', '#d8d8e8'), ('#1a1a2a', '#c82a4a', '#ffd860')],  # rogue
    4: [('#e8e8f0', '#d8b04a', '#5a8ae8'), ('#f0f0f8', '#3a6ad8', '#ffd860'), ('#fff8e8', '#e8c040', '#c83a5a')],  # cleric
    5: [('#e8803a', '#3a3a4a', '#ffd860'), ('#c84a2a', '#2a2a3a', '#ffe070'), ('#f4f0e0', '#c82a2a', '#ffd860')],  # monk
    6: [('#8a5a3a', '#5a6a7a', '#d8a050'), ('#6a4a2a', '#4a5a6a', '#e8b860'), ('#3a3a4a', '#d87a2a', '#ffd860')],  # artificer
    7: [('#4a2a5a', '#2a2a3a', '#8aff9a'), ('#3a1a4a', '#c83a5a', '#c8ff8a'), ('#1a0a2a', '#7a3ae8', '#d8ffa0')],  # occultist
}

HEAD = (24, 16, 11.5, 10.5)


def new(materials):
    return Canvas(S, S, materials)


def render(c):
    return c.render(outline=True)


# ---- body -----------------------------------------------------------------
def body(race, skin):
    sk = SKINS[skin]
    c = new({'skin': sk, 'ear': sk})
    short = race == 2
    top = 3 if short else 0
    # legs and feet (covered by the outfit, visible for robes' gap)
    c.rect(18, 37, 5, 8 - top, 'skin', light=0.5)
    c.rect(25, 37, 5, 8 - top, 'skin', light=0.5)
    # torso + arms
    c.ellipse(24, 32 + top / 2, 8, 6.5, 'skin')
    c.ellipse(15, 33 + top / 2, 2.6, 5, 'skin', angle=12)
    c.ellipse(33, 33 + top / 2, 2.6, 5, 'skin', angle=-12)
    c.circle(14, 37 + top / 2, 2.4, 'skin')
    c.circle(34, 37 + top / 2, 2.4, 'skin')
    # ears
    if race == 1:   # elf: long pointy ears
        c.poly([(13, 15), (4, 9), (13, 20)], 'ear', light=0.75)
        c.poly([(35, 15), (44, 9), (35, 20)], 'ear', light=0.75)
    elif race != 3:
        c.ellipse(12.5, 17, 2.4, 3, 'ear')
        c.ellipse(35.5, 17, 2.4, 3, 'ear')
    hx, hy, rx, ry = HEAD
    c.ellipse(hx, hy + top * 0.6, rx, ry, 'skin')
    return render(c)


def face(eye_color, expr, lashes=False):
    """Only the face pixels (drawn over the head)."""
    c = new({'x': '#000000'})
    ink = '#2a1a2a'
    ec = EYES[eye_color]
    kind = {'idle': 'open', 'blink': 'blink', 'hurt': 'sad', 'happy': 'happy'}[expr]
    eye(c, 17, 15, 4, 5, kind, iris=ec, ink=ink)
    eye(c, 27, 15, 4, 5, kind, iris=ec, ink=ink, mirror=True)
    if lashes and expr in ('idle', 'blink'):
        c.px(16, 15, ink); c.px(31, 15, ink)
    if expr == 'hurt':
        c.pxs(17, 13, ['##', '..'], {'#': ink}); c.pxs(29, 13, ['##'], {'#': ink})
        c.pxs(22, 23, ['.###.', '#...#'], {'#': ink})
    elif expr == 'happy':
        c.pxs(21, 22, ['.###.', '#RRR#', '.###.'], {'#': ink, 'R': '#e8506a'})
    else:
        c.pxs(22, 22, ['#...#', '.###.'], {'#': ink})
    for dx in range(3):
        c.px(14 + dx, 21, '#ff9aaa')
        c.px(31 + dx, 21, '#ff9aaa')
    return c.render(outline=False)


# ---- hair ------------------------------------------------------------------
STYLES = ['short', 'spiky', 'long', 'ponytail', 'bob', 'braids']


def hair_front(style, color):
    c = new({'h': HAIR[color]})
    hx, hy, rx, ry = HEAD
    # cap of hair over the top of the head
    c.ellipse(hx, hy - 3, rx + 1, ry - 2, 'h', clip=lambda xs, ys: ys < hy - 1)
    if style == 'short':
        c.poly([(13, 12), (20, 9), (19, 15)], 'h'); c.poly([(35, 12), (28, 9), (29, 15)], 'h')
        c.poly([(20, 8), (24, 15), (28, 8)], 'h')
    elif style == 'spiky':
        for x, a in ((14, -40), (19, -15), (24, 0), (29, 15), (34, 40)):
            c.flame(x, 9, 7, 9, 'h', angle=a)
        c.poly([(17, 10), (21, 15), (25, 10)], 'h'); c.poly([(25, 10), (29, 14), (32, 10)], 'h')
    elif style in ('long', 'braids'):
        c.ellipse(12, 19, 3, 9, 'h'); c.ellipse(36, 19, 3, 9, 'h')
        c.poly([(16, 9), (24, 14), (32, 9)], 'h')
        if style == 'braids':
            for y in (28, 32, 36):
                c.circle(12, y, 2.2, 'h'); c.circle(36, y, 2.2, 'h')
    elif style == 'ponytail':
        c.poly([(14, 11), (24, 13), (34, 11), (30, 8), (18, 8)], 'h')
        c.ellipse(12.5, 17, 2.2, 5, 'h'); c.ellipse(35.5, 17, 2.2, 5, 'h')
    elif style == 'bob':
        c.ellipse(13, 18, 3.2, 7, 'h'); c.ellipse(35, 18, 3.2, 7, 'h')
        c.rect(14, 8, 20, 5, 'h', light=0.7)
    return render(c)


def hair_back(style, color):
    c = new({'h': HAIR[color]})
    if style == 'long':
        c.rect(12, 14, 24, 20, 'h', round_=6, light=0.45)
    elif style == 'ponytail':
        c.ellipse(38, 18, 4, 10, 'h', angle=-25, light=0.5)
    else:
        return None
    return render(c)


def beast_ears(color):
    c = new({'h': HAIR[color], 'in': '#ffb0c0'})
    c.poly([(13, 10), (12, 0), (20, 6)], 'h', light=0.7)
    c.poly([(35, 10), (36, 0), (28, 6)], 'h', light=0.7)
    c.poly([(14.5, 7), (14, 3), (17.5, 6)], 'in', light=0.85)
    c.poly([(33.5, 7), (34, 3), (30.5, 6)], 'in', light=0.85)
    return render(c)


def tail(color):
    c = new({'h': HAIR[color]})
    c.line([(30, 40), (38, 38), (42, 32), (41, 26)], 'h', width=3.2, light=0.55)
    c.circle(41, 25, 2.2, 'h')
    return render(c)


def beard(color):
    c = new({'h': HAIR[color]})
    c.ellipse(24, 27, 6.5, 4.5, 'h', clip=lambda xs, ys: ys > 23.5)
    c.ellipse(18, 23, 2, 2.5, 'h'); c.ellipse(30, 23, 2, 2.5, 'h')
    return render(c)


# ---- outfits -----------------------------------------------------------------
def outfit(fam, tier):
    main, second, trim = FAMILY[fam][tier]
    c = new({'m': main, 's': second, 't': trim, 'boot': '#5a3a2a', 'skin': SKINS[1]})
    robe = fam in (1, 4, 7)
    if robe:
        c.poly([(16, 27), (32, 27), (35, 45), (13, 45)], 'm', light=0.6)
        c.rect(21, 27, 6, 18, 's', light=0.6)
        c.rect(13, 43, 22, 2, 't', light=0.8)
    else:
        c.rect(17, 36, 6, 7, 's', light=0.5)
        c.rect(25, 36, 6, 7, 's', light=0.5)
        c.rect(16.5, 42, 7, 4, 'boot', round_=1, light=0.5)
        c.rect(24.5, 42, 7, 4, 'boot', round_=1, light=0.5)
        c.ellipse(24, 31.5, 8.5, 6.5, 'm')
        c.rect(16, 35, 16, 2, 't', light=0.8)            # belt
    # sleeves (monks keep bare arms)
    if fam != 5:
        c.ellipse(15, 31.5, 3, 4, 'm', angle=12)
        c.ellipse(33, 31.5, 3, 4, 'm', angle=-12)
    if fam == 0:   # pauldrons + plate
        c.ellipse(15, 28, 4, 3, 's'); c.ellipse(33, 28, 4, 3, 's')
        c.rect(21, 28, 6, 6, 't', light=0.8)
    elif fam == 2:  # leather vest
        c.poly([(19, 27), (24, 34), (29, 27)], 's', light=0.6)
    elif fam == 3:  # scarf
        c.rect(17, 26, 14, 3, 's', round_=1, light=0.7)
        c.rect(27, 27, 3, 6, 's', light=0.6)
    elif fam == 4:  # emblem
        c.pxs(22, 29, ['.T.', 'TTT', '.T.', '.T.'], {'T': trim})
    elif fam == 5:  # sash
        c.poly([(17, 27), (31, 34), (31, 36), (17, 29)], 's', light=0.7)
    elif fam == 6:  # apron with pocket
        c.rect(19, 29, 10, 12, 's', light=0.6)
        c.rect(21, 33, 6, 3, 't', light=0.7)
    elif fam == 7:  # glowing runes
        for y in (31, 35, 39):
            c.px(23, y, trim); c.px(25, y, trim)
    if tier >= 1:   # shoulder trim for advanced classes
        c.ellipse(15, 28.5, 3, 1.6, 't', light=0.85)
        c.ellipse(33, 28.5, 3, 1.6, 't', light=0.85)
    return render(c)


def cape(fam, tier):
    if tier == 0:
        return None
    main, second, trim = FAMILY[fam][tier]
    c = new({'c': second, 't': trim})
    c.poly([(15, 27), (33, 27), (38, 45), (10, 45)], 'c', light=0.45)
    c.rect(10, 43, 28, 2, 't', light=0.7)
    return render(c)


# ---- headgear ------------------------------------------------------------------
def class_hat(fam, tier):
    main, second, trim = FAMILY[fam][tier]
    c = new({'m': main, 's': second, 't': trim, 'metal': '#b8c0d0', 'lens': '#7ae8ff'})
    if fam == 0:      # helmet with plume for tier 2+
        c.ellipse(24, 9, 12, 7, 'metal', clip=lambda xs, ys: ys < 12)
        c.rect(12, 10, 24, 3, 't', light=0.8)
        if tier:
            c.flame(24, 4, 5, 8, 's')
    elif fam == 1:    # wizard hat
        c.ellipse(24, 10, 15, 3.4, 'm')
        c.poly([(15, 10), (28, -2) if tier < 2 else (31, -3), (33, 10)], 'm', light=0.6)
        c.rect(16, 8, 17, 2, 't', light=0.85)
    elif fam == 2:    # hood
        c.ellipse(24, 9, 13, 8, 'm', clip=lambda xs, ys: ys < 12)
        c.poly([(24, 0), (20, 4), (28, 4)], 'm')
        if tier:
            c.px(32, 6, trim); c.px(33, 5, trim); c.px(34, 4, trim)   # feather
    elif fam == 3:    # bandana
        c.rect(12, 8, 24, 4, 's', round_=1, light=0.7)
        c.poly([(35, 9), (41, 12), (38, 14)], 's', light=0.6)
    elif fam == 4:    # mitre / circlet
        if tier == 0:
            c.rect(13, 8, 22, 2, 't', light=0.9)
        else:
            c.poly([(17, 9), (24, -1), (31, 9)], 'm', light=0.85)
            c.pxs(23, 2, ['.T.', 'TTT', '.T.'], {'T': trim})
    elif fam == 5:    # headband
        c.rect(12, 9, 24, 3, 's', light=0.7)
        c.poly([(12, 10), (6, 14), (9, 15)], 's', light=0.6)
    elif fam == 6:    # goggles
        c.rect(12, 10, 24, 2, '#3a3a3a' if False else 's', light=0.5)
        c.circle(19, 10, 3, 'metal'); c.circle(29, 10, 3, 'metal')
        c.circle(19, 10, 1.8, 'lens', light=0.9); c.circle(29, 10, 1.8, 'lens', light=0.9)
    elif fam == 7:    # horned hood
        c.ellipse(24, 9, 13, 8, 'm', clip=lambda xs, ys: ys < 12)
        c.poly([(14, 6), (9, -1), (17, 4)], 't', light=0.8)
        c.poly([(34, 6), (39, -1), (31, 4)], 't', light=0.8)
    return render(c)


def helm(look, tier):
    metal = ['#c88a4a', '#9aa0b0', '#c8d0e0', '#9ad8e8', '#f4e0a0'][tier]
    c = new({'m': metal, 'p': '#c83a3a', 'g': '#7ae8ff', 'h': '#5a4a3a'})
    if look == 0:
        c.ellipse(24, 10, 12.5, 8, 'm', clip=lambda xs, ys: ys < 14)
        c.rect(23, 10, 2, 8, 'm', light=0.8)
        c.flame(24, 3, 4, 7, 'p')
    elif look == 1:
        c.ellipse(24, 9, 13, 8, 'h', clip=lambda xs, ys: ys < 12)
    else:
        c.rect(13, 8, 22, 2, 'm', light=0.9)
        c.circle(24, 8, 2, 'g', light=0.95)
    return render(c)


# ---- weapons (right hand at screen-left, around (14, 37)) ---------------------------
TIER_METAL = ['#c88a4a', '#9aa0b0', '#c8d0e0', '#9ad8e8', '#f4e0a0']
TIER_WOOD = ['#a8703a', '#c89a5a', '#8a4a2a', '#4a3a3a', '#d8d8ff']


def weapon(kind, tier):
    m, w = TIER_METAL[tier], TIER_WOOD[tier]
    c = new({'m': m, 'w': w, 'g': '#7ae8ff', 'r': '#e84a6a', 'grip': '#5a3a2a', 'gold': '#ffd23f'})
    if kind == 0:     # sword
        c.line([(14, 37), (7, 14)], 'm', width=3, light=0.9)
        c.line([(11, 33), (17, 31)], 'gold', width=1.8)
        c.line([(14, 37), (15, 40)], 'grip', width=2)
    elif kind == 1:   # axe
        c.line([(14, 40), (11, 14)], 'w', width=2)
        c.ellipse(9, 17, 5, 6, 'm', angle=10)
        c.clear(lambda xs, ys: (xs > 11) & (xs < 13) & (ys > 11) & (ys < 23))
    elif kind == 2:   # staff
        c.line([(13, 45), (11, 8)], 'w', width=2.2)
        c.circle(11, 6, 3.6, 'g', light=0.95)
        c.circle(11, 6, 1.6, 'gold', light=1)
    elif kind == 3:   # bow
        c.ellipse(12, 30, 5, 15, 'w', clip=lambda xs, ys: xs < 12.5, light=0.6)
        c.clear(lambda xs, ys: ((xs - 12) / 3.2) ** 2 + ((ys - 30) / 13) ** 2 < 1)
        c.line([(12, 15), (12, 45)], 'grip', width=1, light=0.9)
    elif kind == 4:   # dagger
        c.line([(14, 38), (10, 27)], 'm', width=2.2, light=0.9)
        c.line([(12, 35), (16, 34)], 'gold', width=1.5)
    elif kind == 5:   # mace
        c.line([(14, 40), (11, 22)], 'grip', width=2)
        c.circle(10.5, 18, 4.5, 'm')
        for dx, dy in ((-5, 0), (5, 0), (0, -5)):
            c.px(10 + dx, 18 + dy, m)
    elif kind == 6:   # knuckles on both hands
        c.circle(14, 37, 3, 'm', light=0.8)
        c.circle(34, 37, 3, 'm', light=0.8)
        c.px(13, 36, '#ffffff'); c.px(33, 36, '#ffffff')
    elif kind == 7:   # hammer
        c.line([(14, 42), (11, 18)], 'w', width=2)
        c.rect(5, 12, 12, 7, 'm', round_=1)
    elif kind == 8:   # scythe
        c.line([(14, 45), (10, 8)], 'grip', width=2)
        c.ellipse(16, 9, 9, 4, 'm', angle=15, clip=lambda xs, ys: ys < 10)
    return render(c)


def offhand(kind, tier):
    m = TIER_METAL[tier]
    c = new({'m': m, 'f': '#3a5ab8', 'b': '#8a3a2a', 'p': '#f4e8c8', 'g': '#c88aff', 'q': '#8a5a3a', 'a': '#e8e8e8'})
    if kind == 0:     # shield
        c.poly([(30, 28), (42, 28), (42, 36), (36, 44), (30, 36)], 'f', light=0.6)
        c.poly([(32, 30), (40, 30), (40, 35), (36, 41), (32, 35)], 'm', light=0.8, patch=True)
    elif kind == 1:   # quiver on the back, arrows above the shoulder
        c.rect(32, 18, 5, 16, 'q', round_=1, light=0.5)
        for x in (33, 35):
            c.line([(x, 18), (x + 2, 12)], 'a', width=1)
    elif kind == 2:   # tome
        c.rect(31, 33, 9, 10, 'b', round_=1, light=0.6)
        c.rect(32, 34, 2, 8, 'p', light=0.9)
        c.px(36, 37, '#ffd23f')
    elif kind == 3:   # charm
        c.circle(36, 39, 3, 'g', light=0.9)
        c.line([(34, 37), (36, 34)], 'a', width=1)
    return render(c)


def hero_preview(race=0, skin=1, eyes=0, hair_style=0, hair_color=0, fam=0, tier=0, weapon_kind=0, off=0, expr='idle'):
    """Composes a full hero (for previews only; the device composes at runtime)."""
    import numpy as np
    layers = []
    cp = cape(fam, tier)
    if cp is not None: layers.append(cp)
    if off == 1: layers.append(offhand(1, tier))
    if race == 3: layers.append(tail(hair_color))
    hb = hair_back(STYLES[hair_style], hair_color)
    if hb is not None: layers.append(hb)
    layers += [body(race, skin), face(eyes, expr), outfit(fam, tier)]
    if race == 2: layers.append(beard(hair_color))
    layers.append(hair_front(STYLES[hair_style], hair_color))
    if race == 3: layers.append(beast_ears(hair_color))
    layers.append(class_hat(fam, tier))
    layers.append(weapon(weapon_kind, tier * 2))
    if off != 1: layers.append(offhand(off, tier * 2))
    out = np.zeros((S, S, 4), np.uint8)
    for l in layers:
        m = l[:, :, 3] > 0
        out[m] = l[m]
    return out
