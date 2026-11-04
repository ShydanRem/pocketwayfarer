"""RPG backgrounds (156x156, shown at 3x)."""
import math

import numpy as np

from canvas import BAYER4, hexrgb, shift
from scenes import N, Scene, arena as pet_arena, rng, tree, flowers, world_map as pet_map, MAP_ISLANDS


def platforms(s, top, top_hi, shade):
    """Foe stage across the top half, hero stage at the bottom."""
    for cx, cy, rx, ry in ((78, 86, 60, 9), (78, 138, 28, 7)):
        m = s.ellipse_mask(cx, cy, rx, ry)
        s.fill(m, shade)
        s.fill(m & s.ellipse_mask(cx, cy - 2, rx - 2, ry - 2), top)
        s.fill(m & s.ellipse_mask(cx - 6, cy - 4, rx * 0.6, ry - 5), top_hi)


def field(zone):
    """Outdoor arenas: the v0.1 backdrops redrawn with the new stage layout."""
    img = pet_arena(zone)
    s = Scene()
    s.img = img.copy()
    # Paint over the old platforms with the ground colour sampled nearby.
    for cx, cy, rx, ry in ((102, 80, 31, 9), (54, 130, 37, 10)):
        m = s.ellipse_mask(cx, cy, rx, ry)
        ys, xs = np.where(m)
        sample = img[min(N - 1, cy + ry + 2), cx]
        s.img[m] = sample
    looks = [('#8ee86a', '#b8f890', '#4a9a3a'), ('#f8e4a8', '#fff4cc', '#c8a868'), ('#4a8a3a', '#6aaa4a', '#2a5a2a'),
             ('#5a3a3a', '#7a4a42', '#2a1a22'), ('#fff4ff', '#ffffff', '#c8b0e8')]
    platforms(s, *looks[zone])
    return s.img


def bricks(s, base, mortar, dark, seed, y0=0, y1=N):
    s.fill((s.ys >= y0) & (s.ys < y1), base)
    for y in range(y0, y1):
        row = (y - y0) // 7
        if (y - y0) % 7 == 0:
            s.fill((s.ys == y) & (s.xs >= 0), mortar)
            continue
        off = 6 if row % 2 else 0
        for x in range(-off, N, 12):
            if 0 <= x < N:
                s.px(x, y, mortar)
    r = rng(seed)
    for _ in range(60):
        x, y = r.integers(0, N), r.integers(y0, y1)
        s.px(x, y, dark)


def torch(s, x, y, flame, glow):
    m = s.ellipse_mask(x, y - 2, 9, 9)
    s.dither_fill(m, glow if False else tuple(s.img[y, x]), glow, np.full((N, N), 0.25))
    s.fill((s.xs >= x - 1) & (s.xs <= x) & (s.ys >= y) & (s.ys <= y + 6), '#5a3a2a')
    s.fill(s.ellipse_mask(x - 0.5, y - 2, 2, 3.2), flame)
    s.px(x, y - 3, '#ffffff')


def dungeon(zone):
    s = Scene()
    pal = [
        ('#3a3448', '#2a2436', '#241e30', '#7af8a0', '#2e4a3e', '#4a4458', '#5a5468', '#2a2436'),   # crypt
        ('#2a5a6a', '#1e4250', '#18363e', '#8af8ff', '#2a6a7a', '#3a7a7a', '#5aa0a0', '#1e4250'),   # temple
        ('#4a3a2a', '#3a2a1e', '#2e2218', '#c8ff5a', '#4a5a2a', '#5a4a32', '#7a6a42', '#3a2a1e'),   # roots
        ('#4a2228', '#3a181e', '#2e1218', '#ffb03a', '#6a2a1e', '#5a3030', '#7a4038', '#3a181e'),   # forge
        ('#c8c0e8', '#a8a0d0', '#9088c0', '#fff4a0', '#d8d0ff', '#e8e0ff', '#ffffff', '#a8a0d0'),   # citadel
    ][zone]
    wall, mortar, dark, flame, glow, floor, floor_hi, floor_dk = pal
    bricks(s, wall, mortar, dark, zone + 3, 0, 100)
    # floor with perspective bands
    s.gradient([floor_hi, floor, floor_dk], 100, N)
    for y in range(100, N, 9):
        s.fill(s.ys == y, floor_dk)
    for x0 in range(-80, 240, 20):
        for y in range(100, N):
            x = 78 + (x0 - 78) * (y - 60) / 40
            s.px(int(x), y, floor_dk)
    # arch / door in the back
    arch = s.ellipse_mask(78, 62, 18, 22) & (s.ys < 100)
    s.fill(arch, dark)
    s.fill(arch & (s.ys > 70) & (np.abs(s.xs - 78) < 12), '#0a0810' if zone != 4 else '#fff8d0')
    for x in (30, 126):
        torch(s, x, 48, flame, glow)
    if zone == 1:      # water puddles and drips
        for x, y in ((40, 120), (110, 112), (70, 146)):
            s.fill(s.ellipse_mask(x, y, 10, 2.5), '#5ad8e8')
    elif zone == 2:    # roots crossing the wall
        r = rng(9)
        for _ in range(7):
            x = r.integers(0, N)
            pts = [(x + math.sin(t / 6) * 6, t) for t in range(0, 100, 2)]
            for px, py in pts:
                s.fill(s.ellipse_mask(px, py, 2, 2), '#5a4022')
    elif zone == 3:    # lava cracks on the floor
        r = rng(4)
        for _ in range(6):
            x, y = r.integers(10, 146), r.integers(105, 150)
            for i in range(10):
                s.px(x + i, y + (i % 3) - 1, '#ff7a1a')
    elif zone == 4:    # stars through the windows
        s.stars(40, 7, 95, '#ffffff')
    platforms(s, floor_hi, '#ffffff' if zone == 4 else shift(hexrgb(floor_hi), 0, -0.1, 1.2), floor_dk)
    return s.img


def town(night=False):
    s = Scene()
    if night:
        s.gradient(['#0e1238', '#1a2250', '#2a3468', '#3a4478'], 0, 90)
        s.stars(60, 5, 70)
        s.disc(118, 26, 9, '#fff4c8')
    else:
        s.gradient(['#5a8ff0', '#7fb2f8', '#a8d0ff', '#d8ecff'], 0, 90)
        s.cloud(40, 26, 30, '#ffffff', '#d8e8ff')
        s.cloud(122, 18, 22, '#ffffff', '#d8e8ff')
    s.ridge(70, 6, 3, '#6aa8d8' if not night else '#2a3a6a', bottom=96)
    # cobblestone square
    ground = '#c8b088' if not night else '#5a5068'
    stone = '#a89068' if not night else '#4a4058'
    s.fill(s.ys >= 96, ground)
    r = rng(12)
    for _ in range(260):
        x, y = r.integers(0, N), r.integers(98, N)
        s.px(x, y, stone)
        s.px(x + 1, y, stone)
    # houses
    houses = [(8, 58, 30, '#e8d8b8', '#c84a3a'), (40, 52, 26, '#f0e0c8', '#3a6ab8'),
              (92, 54, 26, '#e0d0b0', '#3a9a5a'), (120, 60, 32, '#ead8c0', '#a85ab8')]
    for x, top, w, wall, roof in houses:
        if night:
            wall = shift(hexrgb(wall), 0, 0, 0.55)
            roof = shift(hexrgb(roof), 0, 0, 0.6)
        h = 96 - top
        s.fill((s.xs >= x) & (s.xs < x + w) & (s.ys >= top) & (s.ys < 96), wall)
        roofm = (s.ys >= top - w // 2) & (s.ys < top + 2) & (np.abs(s.xs - (x + w / 2)) <= (s.ys - (top - w // 2)) + 1)
        s.fill(roofm, roof)
        win = '#ffe07a' if night else '#5a7ab8'
        for wx in range(x + 4, x + w - 5, 9):
            s.fill((s.xs >= wx) & (s.xs < wx + 4) & (s.ys >= top + 6) & (s.ys < top + 11), win)
        s.fill((s.xs >= x + w // 2 - 3) & (s.xs < x + w // 2 + 3) & (s.ys >= 86) & (s.ys < 96), '#6a4a2a')
    # signboards
    for x, col in ((22, '#ffd23f'), (52, '#c8c8d8'), (104, '#e85a5a'), (136, '#8af8ff')):
        s.fill((s.xs >= x - 4) & (s.xs <= x + 4) & (s.ys >= 78) & (s.ys <= 83), '#7a4a2a')
        s.px(x, 80, col); s.px(x - 1, 80, col); s.px(x + 1, 80, col)
    # lanterns
    for x in (34, 122):
        s.fill((s.xs == x) & (s.ys >= 84) & (s.ys < 110), '#3a3a4a')
        s.fill(s.ellipse_mask(x, 84, 2.5, 3), '#ffd23f' if night else '#e8c060')
    # fountain behind the hero
    s.fill(s.ellipse_mask(78, 122, 26, 7), '#9aa0b8' if not night else '#5a5a78')
    s.fill(s.ellipse_mask(78, 121, 22, 5), '#5ac8f0' if not night else '#3a6aa8')
    s.fill((np.abs(s.xs - 78) <= 2) & (s.ys >= 104) & (s.ys < 120), '#9aa0b8' if not night else '#5a5a78')
    s.fill(s.ellipse_mask(78, 104, 6, 2), '#5ac8f0' if not night else '#3a6aa8')
    flowers(s, 78, 142, 120, 3, ['#ff6a8a', '#ffe066', '#ffffff'])
    return s.img


def world():
    img = pet_map()
    s = Scene()
    s.img = img.copy()
    # Lantern Hollow in the middle of the sea
    s.island(78, 76, 24, '#7ee06d', '#a8f08a', '#a8703a', '#7a4a2a', 99, depth=12)
    for x, roof in ((72, '#c84a3a'), (80, '#3a6ab8'), (88, '#3a9a5a')):
        s.fill((s.xs >= x - 3) & (s.xs <= x + 3) & (s.ys >= 66) & (s.ys < 73), '#f0e0c8')
        s.fill((s.ys >= 62) & (s.ys < 67) & (np.abs(s.xs - x) <= (s.ys - 62) + 1), roof)
    return s.img


def title():
    s = Scene()
    s.gradient(['#0a0a2a', '#1a1a4a', '#3a2a6a', '#8a4a7a', '#e8806a'], 0, N)
    s.stars(120, 21, 110)
    s.disc(78, 118, 40, '#ffb06a')
    s.disc(78, 118, 34, '#ffd08a')
    s.ridge(108, 10, 31, '#2a1a3a', bottom=N)
    s.ridge(124, 7, 32, '#1a1028', bottom=N)
    # castle silhouette
    c = '#140a20'
    s.fill((s.xs >= 60) & (s.xs <= 96) & (s.ys >= 92) & (s.ys < 130), c)
    for x in (58, 72, 86, 98):
        s.fill((s.xs >= x - 3) & (s.xs <= x + 3) & (s.ys >= 80) & (s.ys < 130), c)
        s.fill((s.ys >= 72) & (s.ys < 80) & (np.abs(s.xs - x) <= (s.ys - 72) / 2 + 1), c)
    for x, y in ((70, 100), (84, 96), (78, 110)):
        s.px(x, y, '#ffd23f')
    return s.img


def all_scenes():
    out = [('title', title()), ('town', town()), ('town_night', town(True)), ('world', world())]
    for z, name in enumerate(['meadow', 'shore', 'forest', 'caves', 'spires']):
        out.append(('field_' + name, field(z)))
    for z, name in enumerate(['crypt', 'temple', 'roots', 'forge', 'citadel']):
        out.append(('dungeon_' + name, dungeon(z)))
    return out


WORLD_SPOTS = MAP_ISLANDS
