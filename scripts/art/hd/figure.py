"""Detailed humanoid hero, 3/4 view facing left (side-view battle layout).

Grid 48x64, feet on y=62, about 4.5 heads tall. Z order (back to front):
cape 1 · back leg 3 · back arm 4 · shield 5 · torso 10 · front leg 12 ·
outfit 14-18 · head 20 · hair 24 · front arm 30 · weapon 35 · hand 36
"""
from sculpt import Sculpt

SKIN = ['#f6d2b8', '#e8b48e', '#c88a5e', '#9a6038', '#6a3e24']
HAIR = ['#3a2a2a', '#7a4a2a', '#d8a050', '#f2e2b0', '#c8402a', '#e8e8f2', '#4a6ae8', '#2a2a40']
EYES = ['#7a4a2a', '#3a8ae8', '#3aa85a', '#9a5ae8']


def base_body(c, skin, pose=0):
    """pose 0 idle, 1 idle breath (shoulders 1px lower), 2 attack lunge."""
    b = 1 if pose == 1 else 0
    lean = -2 if pose == 2 else 0
    # back leg, back arm
    c.capsule((25, 38), (27, 48), 2.9, 2.3, 'skin', z=3)
    c.capsule((27, 48), (28, 58), 2.3, 1.8, 'skin', z=3)
    c.ellipsoid(27, 60, 3.6, 1.9, 'boot', z=3)
    if pose == 2:
        c.capsule((28 + lean, 23 + b), (32, 27), 2.1, 1.7, 'skin', z=4)
        c.capsule((32, 27), (35, 31), 1.7, 1.5, 'skin', z=4)
    else:
        c.capsule((28 + lean, 23 + b), (30, 30 + b), 2.1, 1.7, 'skin', z=4)
        c.capsule((30, 30 + b), (29, 35 + b), 1.7, 1.5, 'skin', z=4)
    # torso
    c.ellipsoid(23 + lean, 26 + b, 6.3, 6.0, 'skin', z=10)
    c.ellipsoid(23 + lean / 2, 32 + b, 5.3, 4.6, 'skin', z=10)
    c.ellipsoid(23, 37, 5.8, 3.4, 'skin', z=10)
    c.capsule((22 + lean, 17 + b), (23 + lean, 21 + b), 2.2, 2.4, 'skin', z=9)
    # front leg
    fx = -3 if pose == 2 else 0
    c.capsule((20, 38), (17 + fx, 48), 3.0, 2.4, 'skin', z=12)
    c.capsule((17 + fx, 48), (15 + fx, 58), 2.4, 1.9, 'skin', z=12)
    c.ellipsoid(13 + fx, 60, 3.8, 2.0, 'boot', z=12)
    # head
    c.ellipsoid(22 + lean, 12 + b, 6.2, 7.0, 'skin', z=20)
    c.ellipsoid(18 + lean, 15.5 + b, 3.2, 2.6, 'skin', z=20.2, flat=0.3)   # jaw towards the viewer
    c.ellipsoid(27 + lean, 13 + b, 1.6, 2.2, 'skin', z=19.5)         # ear


def front_arm(c, pose=0, b=0):
    if pose == 2:      # thrust forward
        c.capsule((18, 23), (12, 25), 2.1, 1.7, 'skin', z=30)
        c.capsule((12, 25), (6, 26), 1.7, 1.5, 'skin', z=30)
        return (5, 26)
    c.capsule((18, 23 + b), (14, 29 + b), 2.1, 1.7, 'skin', z=30)
    c.capsule((14, 29 + b), (11, 33 + b), 1.7, 1.5, 'skin', z=30)
    return (10, 34 + b)


def face(c, eyes, hair, b=0, lean=0, expr='idle', lashes=False):
    ink = '#2a1a24'
    ex, ey = 17 + lean, 11 + b
    if expr == 'blink':
        c.pxs(ex, ey + 1, ['##'], {'#': ink})
        c.px(ex + 5, ey + 1, ink)
    elif expr == 'hurt':
        c.pxs(ex, ey, ['#.', '.#'], {'#': ink})
        c.pxs(ex + 5, ey, ['#', '#'], {'#': ink})
    else:
        c.pxs(ex - 1, ey, ['###', 'WEW', '.E.'], {'#': ink, 'E': EYES[eyes], 'W': '#f4f0ff'})
        c.px(ex, ey + 1, '#1a1020')
        c.pxs(ex + 4, ey, ['##', 'E.', 'E.'], {'#': ink, 'E': EYES[eyes]})
        if lashes:
            c.px(ex - 1, ey, ink)
    brow = hair
    c.pxs(ex - 1, ey - 2, ['.##'], {'#': brow})
    c.pxs(ex + 4, ey - 2, ['##'], {'#': brow})
    c.px(ex - 2, ey + 3, '#d89070')            # nose tip
    mouth = '#a84a4a' if expr != 'hurt' else '#5a2a2a'
    c.px(ex, ey + 5, mouth)
    c.px(ex + 1, ey + 5, mouth)


def hair_short(c, col, b=0, lean=0):
    L = lean
    c.ellipsoid(23.5 + L, 7.5 + b, 7.0, 5.0, 'hair', z=24, clip=lambda xs, ys: ys < 10 + b)
    c.ellipsoid(26.5 + L, 11 + b, 4.2, 5.8, 'hair', z=24)                     # back of the head
    # locks sweeping back and down, tips to the right
    for (x, y, tx, ty, r) in ((18, 5, 26, 2, 2.0), (22, 4, 31, 4, 2.2), (25, 6, 33, 9, 2.0), (27, 10, 32, 16, 1.8)):
        c.capsule((x + L, y + b), (tx + L, ty + b), r, 0.5, 'hair', z=24.5)
    c.capsule((17 + L, 6 + b), (15.5 + L, 9.5 + b), 1.7, 0.6, 'hair', z=25)  # fringe
    c.capsule((20 + L, 5.5 + b), (19.5 + L, 8.5 + b), 1.4, 0.5, 'hair', z=25)


def hair_long(c, col, b=0, lean=0):
    L = lean
    c.capsule((26 + L, 10 + b), (30, 31 + b), 5.2, 2.6, 'hair', z=8)            # mass down the back
    c.ellipsoid(23.5 + L, 7.5 + b, 7.2, 5.2, 'hair', z=24, clip=lambda xs, ys: ys < 10.5 + b)
    c.ellipsoid(27 + L, 12 + b, 3.8, 6.4, 'hair', z=24)
    c.capsule((17 + L, 6 + b), (15.5 + L, 10 + b), 1.8, 0.7, 'hair', z=25)
    c.capsule((20 + L, 5.5 + b), (19.5 + L, 9 + b), 1.5, 0.6, 'hair', z=25)
    c.capsule((27 + L, 15 + b), (28 + L, 24 + b), 1.8, 1.0, 'hair', z=24.2)     # lock behind the ear


def warrior(pose=0, expr='idle', skin=1, hair=1, eyes=1):
    c = Sculpt(48, 64, {'skin': SKIN[skin], 'hair': HAIR[hair], 'boot': '#5a3a2a', 'steel': '#a8b4c8',
                        'gold': '#e8b84a', 'cape': '#b8303a', 'cloth': '#3a4a8a', 'leather': '#7a4a2a',
                        'blade': '#d8e0f0', 'shield': '#3a5aa8'}, metal=('steel', 'gold', 'blade'))
    b = 1 if pose == 1 else 0
    lean = -2 if pose == 2 else 0
    c.poly([(19, 20 + b), (30, 20 + b), (36, 52), (33, 55), (24, 54)], 'cape', z=1, folds=1.4, normal=(0.2, -0.2, 1))
    base_body(c, skin, pose)
    # greaves and boots
    c.capsule((27, 49), (28, 58), 2.9, 2.4, 'steel', z=6)
    fx = -3 if pose == 2 else 0
    c.capsule((17 + fx, 49), (15 + fx, 58), 3.0, 2.5, 'steel', z=13)
    c.ellipsoid(13 + fx, 60, 4.0, 2.1, 'steel', z=13)
    c.ellipsoid(27, 60, 3.8, 2.0, 'steel', z=6)
    # trousers
    c.capsule((25, 38), (27, 48), 3.5, 2.9, 'cloth', z=6)
    c.capsule((20, 38), (17 + fx, 48), 3.6, 3.0, 'cloth', z=13)
    # shield on the back arm
    c.ellipsoid(32, 31 + b, 5.2, 7.2, 'shield', z=5, flat=0.2)
    c.ellipsoid(32, 31 + b, 3.4, 5.2, 'gold', z=5.5, flat=0.4)
    c.ellipsoid(32, 31 + b, 2.6, 4.4, 'shield', z=5.6, flat=0.4)
    # breastplate, belt, tabard, pauldrons
    c.ellipsoid(23 + lean, 26 + b, 6.8, 6.5, 'steel', z=14)
    c.ellipsoid(23 + lean / 2, 32 + b, 5.8, 4.2, 'steel', z=14)
    c.poly([(17, 35), (29, 35), (29, 37), (17, 37)], 'leather', z=15)
    c.px(22, 36, '#ffe080')
    c.poly([(18, 37), (24, 37), (23, 46), (19, 46)], 'cape', z=16, folds=2.0)
    c.ellipsoid(28.5 + lean, 22 + b, 3.6, 3.0, 'steel', z=15)
    c.ellipsoid(18 + lean, 22 + b, 4.0, 3.3, 'steel', z=31)
    c.ellipsoid(18 + lean, 21 + b, 3.0, 1.2, 'gold', z=31.5)
    # head
    face(c, eyes, HAIR[hair], b, lean, expr)
    hair_short(c, HAIR[hair], b, lean)
    # front arm with gauntlet and sword
    hand = front_arm(c, pose, b)
    if pose == 2:
        c.capsule((12, 25), (6, 26), 2.4, 2.1, 'steel', z=30.5)
        c.capsule(hand, (-6, 22), 1.5, 0.6, 'blade', z=35)
        c.capsule((hand[0], hand[1] - 3), (hand[0], hand[1] + 3), 1.0, 1.0, 'gold', z=35.5)
    else:
        c.capsule((14, 29 + b), (11, 33 + b), 2.4, 2.1, 'steel', z=30.5)
        c.capsule((hand[0] - 1, hand[1] - 3), (4, 6 + b), 1.6, 0.7, 'blade', z=33)
        c.capsule((hand[0] - 4, hand[1] - 2), (hand[0] + 2, hand[1] - 4), 1.1, 1.1, 'gold', z=35)
        c.capsule(hand, (hand[0] + 1, hand[1] + 3), 1.0, 0.8, 'leather', z=34)
    c.ellipsoid(hand[0], hand[1], 2.0, 1.9, 'steel', z=36)
    return c.render()


def mage(pose=0, expr='idle', skin=0, hair=3, eyes=3):
    c = Sculpt(48, 64, {'skin': SKIN[skin], 'hair': HAIR[hair], 'boot': '#3a2a3a', 'robe': '#3a2c8a',
                        'trim': '#e8c050', 'inner': '#8a7ac8', 'hat': '#3a2e88', 'wood': '#7a4a2a',
                        'orb': '#7ae8ff', 'sash': '#c83a6a'}, metal=('trim',))
    b = 1 if pose == 1 else 0
    lean = -2 if pose == 2 else 0
    hair_long(c, HAIR[hair], b, lean)
    base_body(c, skin, pose)
    # robe flaring to the ankles
    c.poly([(16 + lean, 20 + b), (30 + lean, 20 + b), (35, 59), (11, 59)], 'robe', z=14, folds=1.1, curve=0.6)
    c.poly([(21 + lean, 22 + b), (25 + lean, 22 + b), (27, 59), (19, 59)], 'inner', z=14.5, folds=0.8)
    c.poly([(11, 56), (35, 56), (35, 59), (11, 59)], 'trim', z=15)
    c.poly([(17, 33 + b), (29, 33 + b), (29, 35 + b), (17, 35 + b)], 'sash', z=15.5)
    c.poly([(27, 34 + b), (30, 34 + b), (31, 44 + b), (28, 44 + b)], 'sash', z=15.6, folds=2)
    c.capsule((28 + lean, 23 + b), (31, 33 + b), 3.4, 3.8, 'robe', z=4.5)         # back sleeve
    face(c, eyes, '#c8a070', b, lean, expr, lashes=True)
    # wizard hat
    c.ellipsoid(22.5 + lean, 6 + b, 10.5, 2.6, 'hat', z=26, flat=0.3)
    c.poly([(16 + lean, 6 + b), (29 + lean, 6 + b), (31 + lean, -2), (36, -6), (27 + lean, -3)], 'hat', z=26.5, curve=0.5)
    c.poly([(16 + lean, 4 + b), (29 + lean, 4 + b), (29 + lean, 6 + b), (16 + lean, 6 + b)], 'trim', z=27)
    c.erase(lambda xs, ys: ys < 0)
    # staff in the front hand
    hand = front_arm(c, pose, b)
    c.capsule((hand[0] - 1, 60), (hand[0] - 2, 4 + b) if pose != 2 else (-2, 12), 1.1, 1.0, 'wood', z=33)
    top = (hand[0] - 2, 3 + b) if pose != 2 else (-2, 11)
    c.ellipsoid(top[0], top[1], 2.8, 2.8, 'orb', z=34, bias=0.25)
    c.capsule((14, 29 + b) if pose != 2 else (12, 25), hand, 3.0, 3.4, 'robe', z=30.5)   # front sleeve
    c.ellipsoid(hand[0], hand[1], 1.8, 1.8, 'skin', z=36)
    return c.render()


def archer(pose=0, expr='idle', skin=2, hair=4, eyes=2):
    c = Sculpt(48, 64, {'skin': SKIN[skin], 'hair': HAIR[hair], 'boot': '#5a3a2a', 'tunic': '#3a7a4a',
                        'leather': '#8a5a3a', 'hood': '#2e5a3e', 'wood': '#9a6a3a', 'string': '#e8e0d0',
                        'quiver': '#6a3a2a', 'feather': '#e8e8f0', 'buckle': '#e8c050'}, metal=('buckle',))
    b = 1 if pose == 1 else 0
    lean = -2 if pose == 2 else 0
    c.poly([(19, 19 + b), (30, 19 + b), (34, 46), (22, 46)], 'hood', z=1, folds=1.6)       # short cloak
    c.capsule((31, 16 + b), (34, 34 + b), 2.4, 2.4, 'quiver', z=2)
    for x in (31, 33, 35):
        c.capsule((x, 16 + b), (x + 1, 11 + b), 0.6, 0.6, 'feather', z=2.5)
    base_body(c, skin, pose)
    fx = -3 if pose == 2 else 0
    c.capsule((27, 50), (28, 58), 2.9, 2.4, 'leather', z=6)
    c.capsule((17 + fx, 50), (15 + fx, 58), 3.0, 2.5, 'leather', z=13)
    c.ellipsoid(27, 60, 3.8, 2.0, 'leather', z=6)
    c.ellipsoid(13 + fx, 60, 4.0, 2.1, 'leather', z=13)
    c.ellipsoid(23 + lean, 26 + b, 6.6, 6.3, 'tunic', z=14)
    c.ellipsoid(23 + lean / 2, 32 + b, 5.6, 4.4, 'tunic', z=14)
    c.poly([(17, 34), (29, 34), (31, 45), (15, 45)], 'tunic', z=14.5, folds=1.6)
    c.poly([(17, 34), (29, 34), (29, 36), (17, 36)], 'leather', z=15)
    c.px(23, 35, '#ffe080')
    c.capsule((26, 20 + b), (18, 34 + b), 1.0, 1.0, 'leather', z=15.5)               # quiver strap
    face(c, eyes, HAIR[hair], b, lean, expr)
    hair_short(c, HAIR[hair], b, lean)
    c.ellipsoid(26 + lean, 9 + b, 5.0, 6.0, 'hood', z=23, clip=lambda xs, ys: xs > 21 + lean)   # hood folded back
    hand = front_arm(c, pose, b)
    # bow held vertically in front
    bx = hand[0] - 2
    y0 = 12 + b if pose != 2 else 4
    pts = [(bx - 5 * (1 - (2 * (i / 16) - 1) ** 2), y0 + i / 16 * 44) for i in range(17)]
    for p1, p2 in zip(pts, pts[1:]):
        c.capsule(p1, p2, 1.0, 1.0, 'wood', z=33)
    c.capsule((bx, y0), (bx, y0 + 44), 0.4, 0.4, 'string', z=33.5)
    c.ellipsoid(hand[0], hand[1], 2.0, 1.9, 'leather', z=36)
    return c.render()
