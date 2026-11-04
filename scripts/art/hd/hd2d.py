"""HD-2D style environment painter.

Scenes are painted in floating point at 233x233 (shown at 2x on the 466 px
display) as depth layers: sky, hazy distance, blurred mid-ground, crisp pixel
ground, out-of-focus foreground, then light (sun glow, shafts, bloom),
vignette and colour grading. Dynamic light and particles are added on the
device by common/ui/fx.c using the metadata returned with each scene.
"""
import math

import numpy as np

N = 233
LAST = None
YS, XS = np.mgrid[0:N, 0:N].astype(float)


def C(h):
    h = h.lstrip('#')
    return np.array([int(h[i:i + 2], 16) for i in (0, 2, 4)], float)


def mix(a, b, t):
    t = np.asarray(t, float)
    if t.ndim:
        t = t[..., None]
    return a * (1 - t) + b * t


def blur(img, sigma):
    if sigma <= 0:
        return img
    r = max(1, int(sigma * 3))
    k = np.exp(-(np.arange(-r, r + 1) ** 2) / (2 * sigma * sigma))
    k /= k.sum()
    out = img
    for axis in (0, 1):
        pad = [(0, 0)] * out.ndim
        pad[axis] = (r, r)
        p = np.pad(out, pad, mode='edge')
        acc = np.zeros_like(out)
        for i, w in enumerate(k):
            sl = [slice(None)] * out.ndim
            sl[axis] = slice(i, i + out.shape[axis])
            acc += p[tuple(sl)] * w
        out = acc
    return out


class Paint:
    def __init__(self, seed):
        self.img = np.zeros((N, N, 3))
        self.rng = np.random.default_rng(seed)
        self.glows = []          # (x, y, r, rgb, flicker) in screen pixels
        self.particles = 0
        self.particle_color = (255, 240, 180)

    # -- layers -----------------------------------------------------------
    def layer(self, mask, color, sigma=0.0, alpha=1.0):
        """Composite `color` (rgb or HxWx3) where mask (bool/float) with optional depth blur."""
        m = mask.astype(float) * alpha
        col = color if np.ndim(color) == 3 else np.broadcast_to(np.asarray(color, float), (N, N, 3))
        if sigma:
            pre = blur(col * m[..., None], sigma)
            m = np.clip(blur(m, sigma), 0, 1)
            col = np.where(m[..., None] > 1e-4, pre / np.maximum(m[..., None], 1e-4), 0)
        self.img = self.img * (1 - m[..., None]) + col * m[..., None]

    def add(self, light):
        self.img = self.img + light

    def sky(self, stops, y0=0, y1=N):
        t = np.clip((YS - y0) / max(1, y1 - y0), 0, 1)
        cols = [C(s) for s in stops]
        seg = t * (len(cols) - 1)
        i = np.clip(seg.astype(int), 0, len(cols) - 2)
        f = seg - i
        out = np.zeros((N, N, 3))
        for k in range(len(cols) - 1):
            sel = i == k
            out[sel] = mix(cols[k], cols[k + 1], f[sel])
        self.img = out

    def glow(self, x, y, r, color, strength=1.0, screen=True, flicker=0):
        g = np.exp(-((XS - x) ** 2 + (YS - y) ** 2) / (2 * r * r))
        self.img = self.img + g[..., None] * C(color) * strength
        if screen:
            self.glows.append((int(x * 2), int(y * 2), int(r * 2.2), color, flicker))

    def stars(self, n, ymax, bright=1.0):
        for _ in range(n):
            x, y = self.rng.uniform(0, N), self.rng.uniform(0, ymax)
            s = self.rng.uniform(0.3, 1.0) * bright
            self.img += np.exp(-((XS - x) ** 2 + (YS - y) ** 2) / 0.5)[..., None] * C('#e8f0ff') * s

    def ridge(self, base, amp, color, sigma, seed, freqs=((0.012, 1), (0.031, 0.5), (0.08, 0.22), (0.19, 0.08))):
        r = np.random.default_rng(seed)
        h = np.full(N, float(base))
        for f, a in freqs:
            h += amp * a * np.sin(XS[0] * f * 2 * math.pi + r.uniform(0, 6.3))
        self.layer(YS >= h[None, :], C(color), sigma)
        return h

    def treeline(self, base, amp, color, sigma, seed, spikes=0.18):
        r = np.random.default_rng(seed)
        h = np.full(N, float(base)) + amp * np.sin(XS[0] * 0.04 + r.uniform(0, 6))
        h -= np.abs(np.sin(XS[0] * spikes + r.uniform(0, 6))) * amp * 1.4     # conifer tips
        self.layer(YS >= h[None, :], C(color), sigma)

    def clouds(self, n, y0, y1, color, shade, sigma, seed, scale=1.0):
        r = np.random.default_rng(seed)
        for _ in range(n):
            cx, cy = r.uniform(-20, N + 20), r.uniform(y0, y1)
            m = np.zeros((N, N))
            for _ in range(7):
                bx, by, br = cx + r.uniform(-24, 24) * scale, cy + r.uniform(-4, 3), r.uniform(7, 14) * scale
                m = np.maximum(m, ((XS - bx) ** 2 + ((YS - by) * 1.6) ** 2 < br * br).astype(float))
            col = mix(C(color), C(shade), np.clip((YS - cy + 6) / 14, 0, 1))
            self.layer(m > 0, col, sigma)

    def canopy(self, cx, cy, r, dark, light, sigma, sun=(1, -1)):
        m = (XS - cx) ** 2 + (YS - cy) ** 2 < r * r
        lit = np.clip(0.5 + ((XS - cx) * sun[0] * 0.5 + (YS - cy) * sun[1] * 0.6) / r, 0, 1)
        # leaf texture: quantised noise keeps a painterly pixel look
        tex = (self.rng.random((N, N)) - 0.5) * 0.18
        self.layer(m, mix(C(dark), C(light), np.clip(lit + tex, 0, 1)), sigma)

    def tree(self, x, top, ground, w, dark, light, trunk, sigma):
        self.layer((np.abs(XS - x) < w / 2) & (YS > top + 10) & (YS < ground), C(trunk), sigma)
        for _ in range(6):
            self.canopy(x + self.rng.uniform(-w * 3, w * 3), top + self.rng.uniform(-6, 24), self.rng.uniform(w * 2.2, w * 3.6),
                        dark, light, sigma)

    def ground(self, horizon, far, near, tufts=None, blades=('#9ad060', '#5e8a3a'), focus=26):
        depth = np.clip((YS - horizon) / (N - horizon), 0, 1)
        g = mix(C(far), C(near), depth)
        mask = YS >= horizon
        base = self.img.copy()
        self.img = np.where(mask[..., None], g, self.img)
        for _ in range(int(N * 0.25)):                  # soft patches
            cx, cy, r = self.rng.uniform(0, N), self.rng.uniform(horizon + 4, N), self.rng.uniform(2.5, 6)
            m = mask & (((XS - cx) / (r * 2.2)) ** 2 + ((YS - cy) / r) ** 2 < 1)
            self.img[m] *= self.rng.uniform(0.93, 1.05)
        if tufts:
            for _ in range(tufts):
                x, y = int(self.rng.integers(0, N)), int(self.rng.integers(horizon + 3, N))
                hgt = 1 + int(depth[y, x] * 4)
                for k in (-1, 0, 1):
                    m = (XS == x + k) & (YS >= y - hgt + abs(k)) & (YS <= y)
                    self.img[m] = C(blades[0] if k == 0 else blades[1])
        # distant ground slightly out of focus
        soft = blur(self.img, 1.3)
        w = np.clip(1 - (YS - horizon) / focus, 0, 1)[..., None] * mask[..., None]
        self.img = self.img * (1 - w) + soft * w
        return mask

    def path(self, horizon, color, edge, width=0.42, wobble=10, x0=116):
        curve = x0 + np.sin((YS - horizon) / 22) * wobble
        half = 3 + (YS - horizon) * width
        m = (YS >= horizon) & (np.abs(XS - curve) < half)
        depth = np.clip((YS - horizon) / (N - horizon), 0, 1)
        col = mix(C(color), C(color) * 0.78, depth)
        self.img[m] = col[m]
        e = (YS >= horizon) & (np.abs(np.abs(XS - curve) - half) < 1.0)
        self.img[e] = C(edge)
        for _ in range(26):
            x, y = int(self.rng.integers(0, N)), int(self.rng.integers(horizon + 3, N))
            if m[y, x]:
                self.img[y, max(0, x - 1):x + 2] = C(color) * 1.15
                if y + 1 < N:
                    self.img[y + 1, max(0, x - 1):x + 2] = C(color) * 0.7
        return m

    def flowers(self, horizon, n, colors, avoid=None):
        for _ in range(n):
            x, y = int(self.rng.integers(0, N)), int(self.rng.integers(horizon + 4, N))
            if avoid is not None and avoid[y, x]:
                continue
            self.img[y, x] = C(colors[self.rng.integers(0, len(colors))])

    def water(self, horizon, y1, deep, shallow, sky_tint, shimmer):
        depth = np.clip((YS - horizon) / max(1, y1 - horizon), 0, 1)
        m = (YS >= horizon) & (YS < y1)
        col = mix(C(sky_tint), mix(C(shallow), C(deep), depth), 0.65)
        self.img[m] = col[m]
        for _ in range(140):
            x, y = self.rng.uniform(0, N), self.rng.uniform(horizon + 1, y1)
            ln = 2 + int((y - horizon) / 8)
            self.img[(np.abs(YS - int(y)) < 0.5) & (XS >= x) & (XS < x + ln) & m] = C(shimmer)
        return m

    def foreground(self, color, sigma=2.6, sides=(1, 1)):
        m = np.zeros((N, N), bool)
        for side, base in ((0, 6), (1, N - 22)):
            if not sides[side]:
                continue
            for k in range(5):
                xb = base + k * 4
                lean = (0.5 - side) * 0.9
                m |= (np.abs(XS - xb - (N - YS) * lean * 0.45) < 2.3) & (YS > 176 + (k % 3) * 8)
        self.layer(m, C(color), sigma)

    def shafts(self, x0s, color, strength=0.32, slope=0.45, y0=10, y1=190):
        s = np.zeros((N, N))
        for x0, w in x0s:
            d = np.abs((XS - x0) - (YS - y0) * slope)
            s += np.clip(1 - d / w, 0, 1) * np.clip((YS - y0) / 100, 0, 1) * np.clip((y1 - YS) / 80, 0, 1)
        self.img += blur(s, 2.4)[..., None] * C(color) * strength

    def finish(self, grade=(1.0, 1.0, 1.0), vignette=0.62, floor=0.22, lift=0.0):
        global LAST
        LAST = self
        v = np.clip(1 - (((XS - N / 2) ** 2 + (YS - N / 2) ** 2) / (N * vignette) ** 2) ** 1.5, floor, 1)
        self.img = self.img * v[..., None] * np.array(grade) + lift
        return np.clip(self.img, 0, 255).astype(np.uint8)


# ---- scenes -----------------------------------------------------------------
def meadow():
    p = Paint(11)
    p.sky(['#2c5a9a', '#6aa0d8', '#f4c88a', '#ffe0a8'], 0, 140)
    p.glow(165, 96, 30, '#fff0c0', 0.8, screen=False)
    p.clouds(5, 20, 70, '#fff4e8', '#e8b8a0', 1.6, 3)
    p.ridge(100, 12, '#9aa8c8', 2.4, 4)
    p.treeline(122, 5, '#5a7a6a', 1.6, 5)
    p.tree(14, 40, 150, 6, '#1e3a2c', '#4e7a42', '#3a2a22', 1.0)
    p.tree(214, 30, 150, 7, '#1e3a2c', '#4e7a42', '#3a2a22', 1.0)
    p.ground(140, '#9ab860', '#3e6a2a', tufts=150)
    road = p.path(140, '#c8a070', '#5a7a38', x0=120)
    p.flowers(140, 40, ['#ffe070', '#ff8aa8', '#ffffff', '#a8c8ff'], road)
    p.shafts([(140, 10), (175, 13)], '#ffe0a0', 0.22)
    p.foreground('#20401e')
    p.particles, p.particle_color = 1, (255, 236, 150)       # drifting pollen
    return p.finish((1.03, 1.0, 0.94))


def shore():
    p = Paint(21)
    p.sky(['#3a2a6a', '#c86a8a', '#ffb07a', '#ffe0a0'], 0, 112)
    p.glow(120, 104, 18, '#fff0c0', 1.2, screen=False)
    p.clouds(6, 30, 90, '#ffc8b0', '#a8607a', 1.4, 6, 1.2)
    p.ridge(104, 6, '#7a5a8a', 2.0, 7, freqs=((0.01, 1), (0.05, 0.3)))
    sea = p.water(110, 158, '#1a4a7a', '#3a8ab8', '#ffb07a', '#ffe8c0')
    col = np.exp(-((XS - 120) ** 2) / (2 * 8 ** 2)) * sea * (np.sin(YS * 1.7) > 0.2)     # sun column on the water
    p.add(col[..., None] * C('#ffd890') * 0.6)
    p.ground(156, '#f0d8a0', '#c8a068', tufts=0)
    for _ in range(30):
        x, y = int(p.rng.integers(0, N)), int(p.rng.integers(160, N))
        p.img[y, x] = C('#fff0d0')
    p.ridge(150, 24, '#3a2a3a', 2.2, 23, freqs=((0.008, 1), (0.05, 0.2)))                          # far headland
    p.ground(156, '#f0d8a0', '#c8a068', tufts=0)
    p.foreground('#3a2a24', sides=(0, 1))
    p.particles, p.particle_color = 2, (220, 240, 255)       # sea spray sparkles
    return p.finish((1.04, 0.98, 0.98))


def forest():
    p = Paint(31)
    p.sky(['#1a3a4a', '#5a8a7a', '#e8c890'], 0, 130)
    p.glow(150, 80, 36, '#ffe8b0', 0.6, screen=False)
    p.treeline(96, 8, '#4a6a62', 2.2, 8)
    p.treeline(112, 6, '#2e4a3e', 1.4, 9)
    for x in (10, 52, 180, 222, 150):
        p.tree(x, 10, 160, 7, '#162e22', '#4a7a42', '#2e2220', 0.8)
    p.ground(150, '#6a8a48', '#2e5226', tufts=130)
    road = p.path(150, '#a8865a', '#4a6a30', x0=112, width=0.34)
    p.flowers(150, 16, ['#ffe070', '#c8a8ff'], road)
    p.shafts([(110, 9), (140, 14), (176, 8)], '#ffd890', 0.34)
    p.foreground('#102414')
    p.particles, p.particle_color = 3, (200, 255, 140)       # fireflies
    return p.finish((1.0, 1.02, 0.95))


def caves():
    p = Paint(41)
    p.sky(['#120a14', '#2a1418', '#4a1e1a'], 0, N)
    for k in range(6):                                         # cave walls receding
        p.ridge(30 + k * 18, 14, ['#1e1218', '#2a161a', '#3a1c1c', '#4a2420', '#5a2a22', '#6a3024'][k], 2.4 - k * 0.35, 40 + k,
                freqs=((0.02, 1), (0.07, 0.5), (0.2, 0.25)))
    for x in range(-10, N, 26):                                 # stalactites
        w = p.rng.uniform(4, 9)
        hgt = p.rng.uniform(16, 44)
        p.layer((YS < hgt * (1 - np.abs(XS - x) / w)) & (np.abs(XS - x) < w), C('#1a0e12'), 0.8)
    lava = (YS > 180) & (YS < 196 + 4 * np.sin(XS / 13))
    p.layer(lava, mix(C('#ff6a1a'), C('#ffd060'), np.clip((YS - 180) / 16, 0, 1)), 0.6)
    p.glow(116, 188, 60, '#ff7a2a', 0.55, flicker=1)
    p.ground(150, '#5a3028', '#2a1416')
    p.img[lava] = mix(C('#ff7a1a'), C('#ffe070'), np.clip((YS - 180) / 16, 0, 1))[lava]
    for x, y in ((40, 70), (190, 60)):
        p.glow(x, y, 14, '#ffa040', 0.5, flicker=1)
    p.particles, p.particle_color = 4, (255, 150, 60)        # embers
    return p.finish((1.06, 0.98, 0.92), floor=0.15)


def spires():
    p = Paint(51)
    p.sky(['#2a2a7a', '#7a6ad8', '#e8a8e8', '#ffe0f0'], 0, 160)
    p.stars(40, 60, 0.8)
    p.glow(60, 70, 40, '#fff0ff', 0.4, screen=False)
    for x, h, w, sig in ((30, 70, 7, 2.2), (190, 60, 9, 2.0), (150, 96, 6, 1.4), (70, 110, 5, 1.0)):
        m = (np.abs(XS - x) < (YS - (N - h - 90)) * w / 60 + 1) & (YS > N - h - 90)
        p.layer(m, mix(C('#e8e0ff'), C('#9a88e0'), np.clip((XS - x) / w + 0.5, 0, 1)), sig)
    p.clouds(10, 140, 200, '#ffffff', '#c8b0f0', 1.6, 52, 1.6)
    p.clouds(8, 178, 240, '#fff8ff', '#d8c0f8', 0.0, 53, 1.8)
    p.particles, p.particle_color = 5, (255, 250, 255)       # stardust
    return p.finish((1.0, 0.98, 1.04), floor=0.3)


def dungeon(kind):
    p = Paint(60 + kind)
    wall, mortar, light, fog = [('#3a3a52', '#1e1e2a', '#8affb0', '#2a4a3a'), ('#2a5a6a', '#123038', '#8af0ff', '#2a6a7a'),
                                ('#4a3626', '#1e140e', '#c8ff6a', '#3a4a22'), ('#4a2226', '#1a0a0c', '#ffa040', '#6a2a1a'),
                                ('#d8d4f0', '#9a96c0', '#fff4b0', '#e8e0ff')][kind]
    p.img[:] = C(mortar)
    # back wall of irregular stones, out of focus
    stones = np.zeros((N, N, 3))
    stones[:] = C(mortar)
    for row in range(12):
        y0 = row * 14
        off = p.rng.uniform(0, 20)
        x = -off
        while x < N:
            w = p.rng.uniform(16, 30)
            sh = p.rng.uniform(0.62, 1.05)
            m = (XS >= x + 1) & (XS < x + w - 1) & (YS >= y0 + 1) & (YS < y0 + 13)
            lit = np.clip(1 - (YS - y0) / 13 * 0.35, 0, 1)
            stones[m] = (C(wall) * sh)[None, :] * lit[m][:, None]
            x += w
    p.layer(YS < 150, stones, 1.1)
    for x in (34, 198):                                        # pillars in the mid-ground
        m = (np.abs(XS - x) < 13) & (YS < 156)
        shade = mix(C(wall) * 1.25, C(wall) * 0.55, np.clip((XS - x + 13) / 26, 0, 1))
        p.layer(m, shade, 0.5)
    arch = ((np.abs(XS - 116) < 22) & (YS > 92) & (YS < 150)) | (((XS - 116) / 22) ** 2 + ((YS - 92) / 22) ** 2 < 1)
    frame = ((np.abs(XS - 116) < 27) & (YS > 92) & (YS < 150)) | (((XS - 116) / 27) ** 2 + ((YS - 92) / 27) ** 2 < 1)
    p.layer(frame & (YS < 150), C(wall) * 1.2, 0.6)
    p.layer(arch & (YS < 150), C(mortar) * 0.35, 0.6)
    p.glow(116, 120, 30, light, 0.25, screen=False)
    for x in (64, 168):                                        # torches / braziers
        p.layer((np.abs(XS - x) < 1.3) & (YS > 64) & (YS < 78), C('#2a1e18'))
        p.glow(x, 60, 24, light if kind != 4 else '#ffe8a0', 0.45, flicker=1)
        p.glow(x, 62, 3.2, '#fff8e0', 1.4, screen=False)
    # floor in perspective
    p.ground(150, wall, mortar)
    for k in range(-12, 13):
        line = (YS >= 150) & (np.abs(XS - (116 + k * (10 + (YS - 150) * 0.6))) < 0.55)
        p.img[line] = C(mortar)
    for y in (154, 162, 174, 190, 212):
        p.img[(YS >= y) & (YS < y + 1)] = C(mortar)
    if kind == 1:
        p.water(196, N, '#0a3a4a', '#2a8a9a', '#5ad8e8', '#bff8ff')
        p.shafts([(90, 8), (130, 10)], '#bff8ff', 0.25, slope=0.1, y0=0)
    elif kind == 2:
        for _ in range(9):
            x = p.rng.uniform(0, N)
            pts = [(x + math.sin(t / 7) * 7, t) for t in range(0, 160, 2)]
            for px, py in pts:
                p.img[((XS - px) ** 2 + (YS - py) ** 2) < 5] = C('#3a2414')
        for x, y in ((40, 160), (180, 170), (100, 200)):
            p.glow(x, y, 9, '#c8ff6a', 0.5, flicker=2)
    elif kind == 3:
        lava = (YS > 200) & (YS < 210 + 3 * np.sin(XS / 9))
        p.img[lava] = C('#ff8a2a')
        p.glow(116, 205, 50, '#ff6a1a', 0.45, flicker=1)
    elif kind == 4:
        p.stars(30, 140)
        p.shafts([(116, 16)], '#fff4c0', 0.3, slope=0.0, y0=0)
    p.add(np.exp(-((XS - 116) ** 2 / (2 * 40 ** 2) + (YS - 140) ** 2 / (2 * 50 ** 2)))[..., None] * C(fog) * 0.35)
    p.particles, p.particle_color = 6, tuple(int(v) for v in C(light))
    return p.finish(floor=0.12 if kind != 4 else 0.35)


def town(night=False):
    p = Paint(71)
    if night:
        p.sky(['#0a0e2a', '#1a2450', '#3a3a6a'], 0, 120)
        p.stars(70, 90)
        p.glow(176, 34, 10, '#fff4d0', 1.0, screen=False)
    else:
        p.sky(['#4a7ad0', '#8ab8f0', '#ffd8a8'], 0, 120)
        p.clouds(4, 18, 50, '#ffffff', '#e8c8c0', 1.4, 72)
    p.ridge(96, 8, '#6a7aa8' if not night else '#22284a', 2.2, 73)
    # houses: far row blurred, near row crisp
    def house(x, top, w, wall, roof, sigma, lit):
        wl = C(wall) * (0.55 if night else 1.0)
        rf = C(roof) * (0.55 if night else 1.0)
        m = (XS >= x) & (XS < x + w) & (YS >= top) & (YS < 150)
        shade = mix(wl * 1.08, wl * 0.72, np.clip((XS - x) / w, 0, 1))
        p.layer(m, shade, sigma)
        rm = (YS >= top - w * 0.45) & (YS < top + 2) & (np.abs(XS - (x + w / 2)) <= (YS - (top - w * 0.45)) * 1.1 + 1)
        p.layer(rm, mix(rf * 1.15, rf * 0.6, np.clip((XS - x) / w, 0, 1)), sigma)
        for wx in np.arange(x + 4, x + w - 6, 9):
            win = (XS >= wx) & (XS < wx + 4) & (YS >= top + 8) & (YS < top + 14)
            p.layer(win, C('#ffd870') if lit else C('#4a6a98'), sigma)
            if lit:
                p.glow(wx + 2, top + 11, 7, '#ffb050', 0.35, screen=sigma == 0)
    for x, t, w, wa, ro in ((-6, 70, 36, '#d8c8b0', '#7a4a3a'), (60, 64, 30, '#c8b8a8', '#4a5a8a'),
                            (130, 66, 34, '#d8c8b0', '#5a7a4a'), (196, 72, 40, '#c8b8a8', '#8a4a6a')):
        house(x, t, w, wa, ro, 1.6, night)
    for x, t, w, wa, ro in ((4, 96, 44, '#f0e0c8', '#c84a3a'), (178, 92, 52, '#ead8c0', '#3a6ab8')):
        house(x, t, w, wa, ro, 0.0, night)
    p.ground(148, '#c8b08a' if not night else '#5a5470', '#8a7458' if not night else '#2e2a40')
    for _ in range(260):                                        # cobbles
        x, y = int(p.rng.integers(0, N)), int(p.rng.integers(150, N))
        p.img[y, x:x + 2] *= 0.82
    # fountain behind the hero
    rim = ((XS - 116) / 34) ** 2 + ((YS - 172) / 9) ** 2 < 1
    p.img[rim] = C('#a8a8c0') * (0.6 if night else 1)
    pool = ((XS - 116) / 30) ** 2 + ((YS - 171) / 7) ** 2 < 1
    p.img[pool] = C('#5ac8f0') * (0.6 if night else 1)
    p.img[(np.abs(XS - 116) < 3) & (YS > 140) & (YS < 170)] = C('#a8a8c0') * (0.6 if night else 1)
    for x in (52, 180):                                          # lanterns
        p.img[(np.abs(XS - x) < 1) & (YS > 118) & (YS < 160)] = C('#2a2a38')
        p.glow(x, 116, 16 if night else 6, '#ffc060', 0.9 if night else 0.3, flicker=1 if night else 0)
        p.glow(x, 116, 2.5, '#fff4c0', 1.4, screen=False)
    p.particles, p.particle_color = (3, (255, 220, 120)) if night else (1, (255, 250, 220))
    return p.finish((0.95, 0.98, 1.1) if night else (1.03, 1.0, 0.95), floor=0.3)


def title():
    p = Paint(81)
    p.sky(['#06061a', '#141444', '#3a2a6a', '#a85a7a'], 0, 200)
    p.stars(160, 150)
    band = np.exp(-((YS - 60 - np.sin(XS / 30) * 14) ** 2) / (2 * 10 ** 2))
    p.add(blur(band, 3)[..., None] * C('#5affc8') * 0.35)
    p.glow(116, 150, 46, '#ffb070', 0.7, screen=False)
    p.ridge(150, 10, '#2a1a3a', 2.0, 82)
    c = '#120a1e'
    castle = ((XS > 92) & (XS < 140) & (YS > 112)) | ((np.abs(XS - 98) < 5) & (YS > 92)) | ((np.abs(XS - 134) < 5) & (YS > 96)) \
        | ((np.abs(XS - 116) < 7) & (YS > 84))
    for x, y0 in ((98, 92), (134, 96), (116, 84)):
        castle |= (YS >= y0 - 12) & (YS < y0) & (np.abs(XS - x) <= (YS - (y0 - 12)) / 2 + 1)
    p.layer(castle, C(c), 0.4)
    for x, y in ((110, 100), (122, 104), (98, 110), (134, 114)):
        p.glow(x, y, 4, '#ffc860', 0.8, screen=False)
        p.img[y, x] = C('#ffe090')
    p.ridge(176, 8, '#0a0614', 0.0, 83)
    p.add(np.clip((YS - 150) / 80, 0, 1)[..., None] * C('#6a4a8a') * 0.25)    # ground mist
    p.particles, p.particle_color = 5, (255, 240, 255)
    return p.finish(floor=0.2)


ISLANDS = [(116, 186), (54, 142), (66, 64), (160, 44), (188, 124)]   # 233-scale map coordinates
TOWN_SPOT = (116, 110)


def world():
    p = Paint(91)
    depth = np.clip(np.hypot(XS - 116, YS - 116) / 160, 0, 1)
    p.img = mix(C('#2a7ab8'), C('#0e2a5a'), depth)
    for _ in range(220):
        x, y = p.rng.uniform(0, N), p.rng.uniform(0, N)
        p.img[(np.abs(YS - int(y)) < 0.5) & (XS >= x) & (XS < x + 3)] = C('#6ab8e8')
    looks = [('#7ec85a', '#3e7a2a'), ('#f0d898', '#b89a5a'), ('#2e7a3e', '#163e22'), ('#7a3a2a', '#3a1a16'), ('#e8e0ff', '#9a88d8')]
    spots = ISLANDS + [TOWN_SPOT]
    looks = looks + [('#9ad86a', '#4a8a3a')]
    for (x, y), (top, side) in zip(spots, looks):
        shadow = ((XS - x - 3) / 22) ** 2 + ((YS - y - 6) / 12) ** 2 < 1
        p.img[shadow] *= 0.6
        isl = ((XS - x) / 20) ** 2 + ((YS - y) / 11) ** 2 < 1
        lit = np.clip(0.5 - (XS - x) / 30 - (YS - y) / 20, 0, 1)
        p.img[isl] = mix(C(side), C(top), lit)[isl]
        shore = isl & ~(((XS - x) / 18) ** 2 + ((YS - y) / 9.5) ** 2 < 1)
        p.img[shore] = C('#f0e8c8')
    for (x1, y1), (x2, y2) in zip(ISLANDS, ISLANDS[1:]):
        n = int(math.hypot(x2 - x1, y2 - y1))
        for i in range(0, n, 5):
            xx, yy = int(x1 + (x2 - x1) * i / n), int(y1 + (y2 - y1) * i / n)
            p.img[yy:yy + 2, xx:xx + 2] = C('#f8f0d0')
    p.clouds(6, 0, N, '#ffffff', '#c8d8f0', 2.0, 92, 1.3)
    p.particles, p.particle_color = 0, (255, 255, 255)
    return p.finish(vignette=0.58, floor=0.25)


def all_scenes():
    """name -> (rgb 233x233, metadata)."""
    out = []
    for name, fn in (('title', title), ('town', town), ('town_night', lambda: town(True)), ('world', world),
                     ('field_meadow', meadow), ('field_shore', shore), ('field_forest', forest),
                     ('field_caves', caves), ('field_spires', spires),
                     ('dungeon_crypt', lambda: dungeon(0)), ('dungeon_temple', lambda: dungeon(1)),
                     ('dungeon_roots', lambda: dungeon(2)), ('dungeon_forge', lambda: dungeon(3)),
                     ('dungeon_citadel', lambda: dungeon(4))):
        # each painter returns the image; metadata is read from the last Paint used
        img = fn()
        out.append((name, img, {'glows': LAST.glows, 'particles': LAST.particles, 'color': LAST.particle_color}))
    return out
