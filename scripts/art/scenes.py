"""Backgrounds rendered at 156x156 and shown at 3x on the 466 px round display."""
import math

import numpy as np

from canvas import BAYER4, hexrgb, shift

N = 156
C0 = N / 2


def rng(seed):
    return np.random.default_rng(seed)


class Scene:
    def __init__(self):
        self.img = np.zeros((N, N, 3), dtype=np.uint8)
        self.ys, self.xs = np.mgrid[0:N, 0:N]

    def gradient(self, stops, y0=0, y1=N):
        """Vertical dithered gradient between colour stops."""
        cols = [np.array(hexrgb(c)) for c in stops]
        for y in range(y0, y1):
            t = (y - y0) / max(1, y1 - y0 - 1) * (len(cols) - 1)
            for x in range(N):
                tt = t + (BAYER4[y % 4, x % 4] - 0.5) * 0.9
                i = int(max(0, min(len(cols) - 1, round(tt))))
                self.img[y, x] = cols[i]

    def fill(self, mask, color):
        self.img[mask] = hexrgb(color)

    def dither_fill(self, mask, c1, c2, t):
        """t: array 0..1 blend weight toward c2."""
        th = BAYER4[self.ys % 4, self.xs % 4]
        m2 = mask & (t > th)
        self.img[mask & ~m2] = hexrgb(c1)
        self.img[m2] = hexrgb(c2)

    def disc(self, cx, cy, r, color):
        self.fill((self.xs - cx) ** 2 + (self.ys - cy) ** 2 <= r * r, color)

    def ellipse_mask(self, cx, cy, rx, ry):
        return ((self.xs - cx) / rx) ** 2 + ((self.ys - cy) / ry) ** 2 <= 1

    def px(self, x, y, color):
        if 0 <= x < N and 0 <= y < N:
            self.img[int(y), int(x)] = hexrgb(color)

    def ridge(self, base, amp, seed, color, freq=(0.05, 0.11, 0.23), bottom=N):
        r = rng(seed)
        ph = r.uniform(0, 6.28, len(freq))
        x = np.arange(N)
        h = base + sum(amp / (i + 1) * np.sin(x * f + p) for i, (f, p) in enumerate(zip(freq, ph)))
        mask = (self.ys >= h[None, :].repeat(N, 0)) & (self.ys < bottom)
        self.fill(mask, color)
        return h

    def cloud(self, cx, cy, w, color, shade):
        r = rng(int(cx * 7 + cy))
        blobs = [(cx + r.uniform(-w / 2, w / 2), cy + r.uniform(-2, 1), r.uniform(w / 6, w / 3.2)) for _ in range(6)]
        mask = np.zeros((N, N), bool)
        for bx, by, br in blobs:
            mask |= (self.xs - bx) ** 2 + ((self.ys - by) * 1.4) ** 2 <= br * br
        mask &= self.ys <= cy + 3
        low = mask & (self.ys > cy)
        self.fill(mask, color)
        self.fill(low, shade)

    def stars(self, count, seed, ymax, color='#ffffff'):
        r = rng(seed)
        for _ in range(count):
            x, y = r.integers(0, N), r.integers(0, ymax)
            self.px(x, y, color if r.random() > 0.3 else '#a8b8ff')

    def island(self, cx, top, w, grass, grass_hi, dirt, dirt_dark, seed, depth=40):
        """Floating island: grassy top ellipse with a tapering rocky underside."""
        r = rng(seed)
        under = np.zeros((N, N), bool)
        for y in range(int(top), int(top + depth)):
            t = (y - top) / depth
            half = w / 2 * (1 - t) ** 1.3 + r.uniform(-1, 1)
            under[y, int(max(0, cx - half)):int(min(N, cx + half + 1))] = True
        self.fill(under, dirt)
        shade = under & (self.xs > cx + w * 0.08)
        self.fill(shade, dirt_dark)
        for _ in range(int(w / 3)):
            y = int(top + r.uniform(4, depth * 0.7))
            x = int(cx + r.uniform(-w / 3, w / 3))
            if under[min(y, N - 1), x]:
                self.px(x, y, shift(hexrgb(dirt_dark), 0, 0, 0.75))
        topm = self.ellipse_mask(cx, top, w / 2 + 2, 6)
        self.fill(topm, grass)
        self.fill(topm & (self.ys < top - 2), grass_hi)
        # grass fringe hanging over the edge
        for x in range(int(cx - w / 2), int(cx + w / 2), 2):
            yy = top + 5 + (1 if x % 4 else 0)
            self.px(x, yy, grass)
        return topm

    def save_circle_check(self):
        return self.img


def flowers(s, cx, top, w, seed, colors):
    r = rng(seed)
    for _ in range(int(w / 5)):
        x = int(cx + r.uniform(-w / 2.4, w / 2.4))
        y = int(top + r.uniform(-3, 3))
        col = colors[r.integers(0, len(colors))]
        s.px(x, y, col)
        s.px(x, y + 1, '#2f8a3a')


def tree(s, x, ground, h, trunk, leaf, leaf_hi, leaf_dark):
    s.fill((s.xs >= x - 1) & (s.xs <= x + 1) & (s.ys >= ground - h * 0.5) & (s.ys <= ground), trunk)
    for dx, dy, rr in ((0, -h * 0.75, h * 0.32), (-h * 0.2, -h * 0.55, h * 0.26), (h * 0.22, -h * 0.56, h * 0.25)):
        m = s.ellipse_mask(x + dx, ground + dy, rr, rr * 0.85)
        s.fill(m, leaf)
        s.fill(m & ((s.xs - (x + dx)) + (s.ys - (ground + dy)) > rr * 0.4), leaf_dark)
        s.fill(m & ((s.xs - (x + dx)) + (s.ys - (ground + dy)) < -rr * 0.6), leaf_hi)


def home(variant):
    s = Scene()
    if variant == 'day':
        s.gradient(['#4a7fe8', '#5f95f0', '#7fb2f8', '#a8d0ff', '#d4ecff'], 0, 120)
        s.disc(118, 30, 11, '#fff6c8')
        s.disc(118, 30, 8, '#ffffff')
        far, far2 = '#9ec6f4', '#b8d8fa'
        cloud, cloud_sh = '#ffffff', '#d8e8ff'
        grass, grass_hi, dirt, dirt_dk = '#5cc84a', '#8ee86a', '#a8703a', '#7a4a2a'
    elif variant == 'dusk':
        s.gradient(['#3a2a6a', '#7a3a7a', '#d0587a', '#ff9a6a', '#ffd08a'], 0, 120)
        s.disc(112, 96, 14, '#ffd88a')
        s.disc(112, 96, 10, '#fff2c0')
        far, far2 = '#a0507a', '#c0687e'
        cloud, cloud_sh = '#ffb8a0', '#c87090'
        grass, grass_hi, dirt, dirt_dk = '#4aa05a', '#7acb6a', '#8a5a3a', '#5a3a2a'
    else:
        s.gradient(['#0a0a2a', '#141a44', '#1e2a5e', '#2a3a72', '#34487e'], 0, 120)
        s.stars(90, 3, 100)
        s.disc(44, 34, 10, '#fff4c8')
        s.disc(48, 31, 9, '#1a2250')
        far, far2 = '#24305e', '#2c3a6c'
        cloud, cloud_sh = '#3a4880', '#2c3868'
        grass, grass_hi, dirt, dirt_dk = '#2e6a4a', '#3e8a5a', '#4a3450', '#342640'
    s.gradient_done = True
    # Bottom of the sky below the horizon line keeps the last colour.
    s.img[120:] = s.img[119]
    # Far islands
    s.island(26, 82, 30, far, far2, far, far, 11, depth=16)
    s.island(132, 70, 22, far, far2, far, far, 12, depth=12)
    s.cloud(70, 48, 30, cloud, cloud_sh)
    s.cloud(124, 92, 26, cloud, cloud_sh)
    s.cloud(22, 110, 30, cloud, cloud_sh)
    # Main island: pet stands at y=112 (screen y ~336).
    s.island(78, 112, 124, grass, grass_hi, dirt, dirt_dk, 5, depth=44)
    tree(s, 30, 110, 30, '#7a4a2a', grass, grass_hi, shift(hexrgb(grass), 10, 0.05, 0.7))
    flowers(s, 78, 112, 110, 9, ['#ff6a8a', '#ffe066', '#ffffff', '#9a7aff'])
    # Signpost
    s.fill((s.xs >= 126) & (s.xs <= 127) & (s.ys >= 100) & (s.ys <= 110), '#7a4a2a')
    s.fill((s.xs >= 120) & (s.xs <= 133) & (s.ys >= 100) & (s.ys <= 104), '#c88a4a')
    if variant == 'night':
        for x, y in ((50, 96), (100, 88), (64, 70), (140, 100), (20, 60)):
            s.px(x, y, '#e8ff7a')
            s.px(x + 1, y, '#a8d04a')
    return s.img


def arena(zone):
    s = Scene()
    if zone == 0:   # meadow
        s.gradient(['#5a8ff0', '#7fb2f8', '#b8dcff', '#e0f4ff'], 0, 80)
        s.img[80:] = hexrgb('#78d05a')
        s.ridge(62, 8, 1, '#8ec8e8', bottom=82)
        s.ridge(72, 6, 2, '#6cc05a', bottom=N)
        s.ridge(88, 5, 3, '#5ab84a', bottom=N)
        s.cloud(40, 30, 28, '#ffffff', '#d8e8ff')
        s.cloud(118, 22, 22, '#ffffff', '#d8e8ff')
        plat, plat_hi, plat_sh = '#8ee86a', '#b8f890', '#4a9a3a'
        flowers(s, 78, 120, 140, 4, ['#ff6a8a', '#ffe066', '#ffffff'])
    elif zone == 1:  # shore
        s.gradient(['#4ab0e8', '#7fd0f4', '#c8f0ff'], 0, 66)
        s.gradient(['#2a8ad0', '#3aa8e0', '#5ac8f0'], 66, 96)
        s.img[96:] = hexrgb('#f0d898')
        s.ridge(100, 3, 7, '#e0c080', bottom=N)
        r = rng(4)
        for _ in range(40):
            x, y = r.integers(0, N), r.integers(68, 94)
            for i in range(3):
                s.px(x + i, y, '#bff0ff')
        s.cloud(110, 30, 30, '#ffffff', '#cfe8ff')
        plat, plat_hi, plat_sh = '#f8e4a8', '#fff4cc', '#c8a868'
    elif zone == 2:  # forest
        s.gradient(['#1a3a2a', '#24543a', '#2e6a44', '#3a7a4a'], 0, N)
        for i, x in enumerate((8, 30, 54, 104, 128, 150)):
            w = 6 + (i % 3) * 2
            s.fill((s.xs >= x - w / 2) & (s.xs <= x + w / 2), '#3a2a24' if i % 2 else '#4a3428')
        s.ridge(18, 6, 5, '#1e4a2e', bottom=0) if False else None
        canopy = np.zeros((N, N), bool)
        r = rng(8)
        for _ in range(26):
            cx, cy, rr = r.uniform(0, N), r.uniform(-5, 30), r.uniform(10, 20)
            canopy |= (s.xs - cx) ** 2 + (s.ys - cy) ** 2 <= rr * rr
        s.fill(canopy, '#24603a')
        s.fill(canopy & (s.ys < 12), '#2e7a44')
        for x0 in (40, 80, 116):  # light rays
            ray = (np.abs(s.xs - x0 - (s.ys - 30) * 0.4) < 2.5) & (s.ys > 30) & (s.ys < 130) & ~canopy
            base = s.img.copy()
            s.dither_fill(ray, '#3a7a4a', '#4e9458', np.full((N, N), 0.25))
            keep = ray & (BAYER4[s.ys % 4, s.xs % 4] >= 0.25)
            s.img[keep] = base[keep]
        s.img[118:] = hexrgb('#2a5a34')
        s.ridge(116, 3, 9, '#346a3a', bottom=N)
        plat, plat_hi, plat_sh = '#4a8a3a', '#6aaa4a', '#2a5a2a'
    elif zone == 3:  # cave
        s.gradient(['#24142a', '#341a30', '#4a2232', '#6a2c2c', '#9a4428'], 0, N)
        r = rng(13)
        for _ in range(14):
            x, w, h = r.uniform(0, N), r.uniform(4, 9), r.uniform(10, 30)
            tri = (s.ys <= h * (1 - np.abs(s.xs - x) / w)) & (np.abs(s.xs - x) < w)
            s.fill(tri, '#2a1a2a')
        s.ridge(124, 4, 6, '#3a2028', bottom=N)
        lava = s.ys >= 140
        s.dither_fill(lava, '#ff6a1a', '#ffc83a', (s.ys - 140) / 20.0)
        for x, y in ((20, 60), (130, 50), (90, 80), (40, 100)):
            s.px(x, y, '#ffb03a')
        plat, plat_hi, plat_sh = '#5a3a3a', '#7a4a42', '#2a1a22'
    else:  # sky spires
        s.gradient(['#5a4ab8', '#8a6ad0', '#c890e0', '#ffc8e8'], 0, N)
        s.stars(30, 21, 50)
        for x, h in ((18, 70), (140, 60), (120, 90)):
            spire = (np.abs(s.xs - x) < (s.ys - (N - h)) * 0.12 + 1) & (s.ys > N - h)
            s.fill(spire, '#e8d8ff')
            s.fill(spire & (s.xs > x), '#b8a0f0')
        for cx, cy, w in ((30, 120, 50), (90, 132, 70), (150, 118, 40), (60, 146, 60)):
            s.cloud(cx, cy, w, '#ffffff', '#e8d0f8')
        plat, plat_hi, plat_sh = '#fff4ff', '#ffffff', '#c8b0e8'
    # Platforms: foe (top right) and companion (bottom left).
    for cx, cy, rx, ry in ((102, 80, 30, 8), (54, 130, 36, 9)):
        m = s.ellipse_mask(cx, cy, rx, ry)
        s.fill(m, plat_sh)
        s.fill(m & s.ellipse_mask(cx, cy - 2, rx - 2, ry - 2), plat)
        s.fill(m & s.ellipse_mask(cx - 4, cy - 4, rx - 10, ry - 5), plat_hi)
    return s.img


MAP_ISLANDS = [(78, 122), (34, 92), (44, 42), (106, 30), (124, 82)]


def world_map():
    s = Scene()
    s.gradient(['#1a3a7a', '#24508a', '#2a64a0'], 0, N)
    r = rng(17)
    for _ in range(120):
        x, y = r.integers(0, N), r.integers(0, N)
        s.px(x, y, '#3a7ab8')
        s.px(x + 1, y, '#3a7ab8')
    # dotted path between islands
    for (x1, y1), (x2, y2) in zip(MAP_ISLANDS, MAP_ISLANDS[1:]):
        n = int(math.hypot(x2 - x1, y2 - y1))
        for i in range(0, n, 4):
            s.px(x1 + (x2 - x1) * i / n, y1 + (y2 - y1) * i / n, '#f0e0b0')
    looks = [('#7ee06d', '#a8703a'), ('#f8e4a8', '#c8a868'), ('#2e8a44', '#5a3a2a'), ('#6a3a3a', '#3a2028'),
             ('#fff0ff', '#b8a0f0')]
    for (x, y), (top, rock) in zip(MAP_ISLANDS, looks):
        s.island(x, y, 22, top, shift(hexrgb(top), 0, -0.1, 1.15), rock, shift(hexrgb(rock), 0, 0, 0.7), x + y, depth=12)
    # compass rose in the middle
    for i in range(-5, 6):
        s.px(80 + i, 70, '#f0e0b0')
        s.px(80, 70 + i, '#f0e0b0')
    s.px(80, 64, '#ff6a6a')
    return s.img


def all_scenes():
    out = [('home_day', home('day')), ('home_dusk', home('dusk')), ('home_night', home('night'))]
    for z, name in enumerate(['meadow', 'shore', 'forest', 'cave', 'sky']):
        out.append(('arena_' + name, arena(z)))
    out.append(('map', world_map()))
    return out
