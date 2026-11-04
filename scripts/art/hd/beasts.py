"""Detailed monsters facing right (towards the party on the right)."""
from sculpt import Sculpt


def dire_wolf(pose=0):
    c = Sculpt(64, 48, {'fur': '#5a5a72', 'belly': '#a8a8b8', 'dark': '#2e2e40', 'claw': '#e8e0d0',
                        'eye': '#ff5a3a', 'mouth': '#7a1a2a'})
    up = -2 if pose else 0
    # far legs
    c.capsule((22, 30), (20, 40), 2.6, 1.8, 'dark', z=1)
    c.capsule((20, 40), (21, 45), 1.8, 1.6, 'dark', z=1)
    c.capsule((44, 30), (47, 39), 2.8, 1.9, 'dark', z=1)
    c.capsule((47, 39), (46, 45), 1.9, 1.6, 'dark', z=1)
    # tail
    c.capsule((14, 24), (5, 18), 3.2, 1.6, 'fur', z=2)
    c.capsule((5, 18), (3, 12), 1.6, 0.8, 'fur', z=2)
    # body
    c.ellipsoid(30, 27, 15, 8.5, 'fur', z=5)
    c.ellipsoid(42, 25 + up, 8, 8.5, 'fur', z=6)                    # shoulders
    c.ellipsoid(31, 32, 11, 3.6, 'belly', z=5.5, flat=0.4)
    # ruff of fur
    for (x, y, tx, ty) in ((38, 18, 33, 14), (42, 17, 39, 12), (46, 19, 45, 13), (35, 21, 28, 18), (24, 20, 18, 17)):
        c.capsule((x, y + up), (tx, ty + up), 2.6, 0.6, 'fur', z=6.5)
    # near legs
    c.capsule((26, 30), (24, 40), 3.2, 2.2, 'fur', z=8)
    c.capsule((24, 40), (25, 45), 2.2, 1.8, 'fur', z=8)
    c.ellipsoid(26.5, 45.5, 2.8, 1.4, 'fur', z=8)
    c.capsule((42, 31 + up), (45, 40), 3.4, 2.3, 'fur', z=9)
    c.capsule((45, 40), (44, 45), 2.3, 1.9, 'fur', z=9)
    c.ellipsoid(46, 45.5, 3.0, 1.4, 'fur', z=9)
    for x in (27, 28, 47, 48):
        c.px(x + 1, 46, '#e8e0d0')
    # head
    c.ellipsoid(50, 18 + up, 6.5, 6.0, 'fur', z=12)
    c.capsule((52, 20 + up), (60, 21 + up), 3.4, 2.4, 'fur', z=12.5)   # muzzle
    c.ellipsoid(60.5, 20 + up, 1.6, 1.3, 'dark', z=13)                # nose
    c.capsule((47, 13 + up), (45, 5 + up), 2.4, 0.6, 'fur', z=11.5)    # ears
    c.capsule((51, 13 + up), (51, 5 + up), 2.4, 0.6, 'fur', z=12.6)
    if pose:
        c.capsule((53, 23 + up), (60, 26 + up), 1.6, 1.2, 'mouth', z=12.8)
        c.pxs(54, 22 + up, ['W.W.W'], {'W': '#f4f0e0'})
    c.pxs(52, 16 + up, ['EE', 'E.'], {'E': '#ff6a3a'})
    c.px(53, 15 + up, '#2a1a1a')
    return c.render()


def skeleton_knight(pose=0):
    c = Sculpt(44, 64, {'bone': '#d8d0b8', 'iron': '#6a6a80', 'rust': '#8a4a3a', 'cloth': '#4a2a4a',
                        'blade': '#b8c0d0', 'glow': '#7affc8', 'gold': '#c8a050'},
               metal=('iron', 'blade', 'gold'))
    lunge = 3 if pose else 0
    # far limbs (character faces right: far side is the left)
    c.capsule((19, 38), (17, 49), 1.6, 1.3, 'bone', z=2)
    c.capsule((17, 49), (16, 59), 1.3, 1.1, 'bone', z=2)
    c.ellipsoid(17, 61, 3.0, 1.4, 'iron', z=2)
    c.capsule((17, 22), (13, 30), 1.5, 1.2, 'bone', z=3)
    c.capsule((13, 30), (12, 36), 1.2, 1.0, 'bone', z=3)
    # round shield on the far arm
    c.ellipsoid(11, 34, 6.0, 7.5, 'iron', z=4, flat=0.2)
    c.ellipsoid(11, 34, 2.0, 2.0, 'gold', z=4.5)
    # torn cloak
    c.poly([(16, 20), (28, 20), (30, 48), (26, 45), (22, 50), (18, 46), (14, 50)], 'cloth', z=5, folds=1.5)
    # ribcage and spine
    c.capsule((22, 20), (22, 37), 1.2, 1.2, 'bone', z=8)
    for i, y in enumerate((23, 26, 29, 32)):
        w = 6 - i * 0.6
        c.capsule((22 - w, y), (22 + w, y), 1.0, 1.0, 'bone', z=8.5)
    c.ellipsoid(22, 21, 7.5, 2.8, 'iron', z=9)                        # pauldron bar
    c.ellipsoid(22, 39, 6.0, 3.0, 'bone', z=8)                        # pelvis
    c.poly([(16, 38), (28, 38), (29, 46), (15, 46)], 'rust', z=9, folds=2)   # tattered tabard
    # near leg
    c.capsule((24, 40), (26, 50), 1.8, 1.4, 'bone', z=10)
    c.capsule((26, 50), (27 + lunge, 59), 1.4, 1.2, 'bone', z=10)
    c.ellipsoid(28 + lunge, 61, 3.4, 1.5, 'iron', z=10)
    c.capsule((25, 49), (26, 51), 2.2, 2.2, 'iron', z=10.5)          # knee guard
    # skull with helmet
    c.ellipsoid(23, 12, 6.0, 6.5, 'bone', z=14)
    c.ellipsoid(27, 16, 3.4, 2.4, 'bone', z=14.5)                     # jaw
    c.ellipsoid(22, 8, 7.0, 5.0, 'iron', z=15, clip=lambda xs, ys: ys < 10)
    c.capsule((14, 9), (30, 9), 1.0, 1.0, 'gold', z=15.5)
    c.capsule((22, 3), (17, -2), 1.6, 0.6, 'rust', z=15.2)            # crest
    c.erase(lambda xs, ys: ys < 0)
    c.pxs(25, 11, ['KK', 'KG'], {'K': '#1a1420', 'G': '#7affc8'})
    c.pxs(26, 17, ['K.K.K'], {'K': '#3a2a2a'})
    # near arm with a notched sword
    c.capsule((26, 22), (30, 29), 1.6, 1.3, 'bone', z=20)
    hand = (34 + lunge, 32 - lunge)
    c.capsule((30, 29), hand, 1.3, 1.1, 'bone', z=20)
    tip = (44, 14) if not pose else (48, 26)
    c.capsule(hand, tip, 1.5, 0.6, 'blade', z=21)
    c.capsule((hand[0] - 2, hand[1] + 2), (hand[0] + 2, hand[1] - 2), 1.0, 1.0, 'gold', z=21.5)
    c.ellipsoid(hand[0], hand[1], 1.8, 1.6, 'iron', z=22)
    return c.render(rim_color=(140, 255, 200))
