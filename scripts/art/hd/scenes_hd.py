"""HD-2D style backgrounds: painterly blurred depth layers behind crisp pixel ground,
light shafts, haze, floating motes and a vignette. Rendered at 233x233 (shown 2x)."""
import math

import numpy as np

N = 233


def hexf(h):
    h = h.lstrip('#')
    return np.array([int(h[i:i + 2], 16) for i in (0, 2, 4)], float)


def blur(img, sigma):
    if sigma <= 0:
        return img
    r = int(sigma * 3)
    k = np.exp(-(np.arange(-r, r + 1) ** 2) / (2 * sigma * sigma))
    k /= k.sum()
    out = img.copy()
    for axis in (0, 1):
        out = np.apply_along_axis(lambda m: np.convolve(np.pad(m, r, mode='edge'), k, mode='valid'), axis, out)
    return out


class Layer:
    def __init__(self):
        self.rgb = np.zeros((N, N, 3))
        self.a = np.zeros((N, N))

    def fill(self, mask, color, alpha=1.0):
        c = hexf(color) if isinstance(color, str) else np.array(color, float)
        self.rgb[mask] = c
        self.a[mask] = alpha


def noise1(x, seed, octaves=((0.02, 1.0), (0.05, 0.45), (0.13, 0.2))):
    rng = np.random.default_rng(seed)
    v = np.zeros_like(x, float)
    for f, a in octaves:
        v += a * np.sin(x * f * 2 * math.pi + rng.uniform(0, 6.28))
    return v


def over(dst, layer, sigma=0.0):
    rgb, a = layer.rgb, layer.a
    if sigma:
        pre = blur(rgb * a[..., None], sigma)
        a = np.clip(blur(a, sigma), 0, 1)
        rgb = np.where(a[..., None] > 1e-4, pre / np.maximum(a[..., None], 1e-4), 0)
    return dst * (1 - a[..., None]) + rgb * a[..., None]


def forest(seed=3):
    ys, xs = np.mgrid[0:N, 0:N].astype(float)
    rng = np.random.default_rng(seed)
    # sky: deep teal to warm gold at the horizon, sun glow
    t = np.clip(ys / 130, 0, 1)[..., None]
    img = hexf('#1e3a5a') * (1 - t) + hexf('#f2b870') * t
    sun = np.exp(-((xs - 160) ** 2 + (ys - 92) ** 2) / (2 * 34 ** 2))[..., None]
    img = img + sun * hexf('#ffe0a0') * 0.7
    # far mountains (hazy)
    L = Layer()
    h = 92 + 12 * noise1(xs[0], seed)
    L.fill(ys >= h[None, :], '#8a90b0')
    img = over(img, L, 2.2)
    # far tree line
    L = Layer()
    h = 112 + 6 * noise1(xs[0], seed + 1, ((0.08, 1), (0.21, 0.5), (0.4, 0.3)))
    L.fill(ys >= h[None, :], '#4a6a72')
    img = over(img, L, 1.6)
    # mid trees: trunks and canopies
    L = Layer()
    for x in (12, 48, 190, 222, 160):
        w = rng.uniform(5, 8)
        L.fill((np.abs(xs - x) < w / 2) & (ys > 40) & (ys < 160), '#3a2a26')
        for _ in range(7):
            cx, cy, r = x + rng.uniform(-22, 22), rng.uniform(10, 70), rng.uniform(14, 24)
            m = (xs - cx) ** 2 + (ys - cy) ** 2 < r * r
            lit = np.clip(0.5 + ((xs - cx) * 0.5 - (ys - cy) * 0.6) / r, 0, 1)
            col = hexf('#1e3a2c')[None, None, :] * (1 - lit[..., None]) + hexf('#5a8a4a')[None, None, :] * lit[..., None]
            L.rgb[m] = col[m]
            L.a[m] = 1
    img = over(img, L, 0.9)
    # light shafts through the canopy
    shafts = np.zeros((N, N))
    for x0, w in ((120, 9), (150, 14), (182, 7)):
        d = np.abs((xs - x0) - (ys - 20) * 0.45)
        shafts += np.clip(1 - d / w, 0, 1) * np.clip((ys - 20) / 120, 0, 1) * np.clip((200 - ys) / 80, 0, 1)
    img = img + blur(shafts, 2.5)[..., None] * hexf('#ffd890') * 0.35
    # ground: pixel grass, path in perspective, blur fading out toward the camera
    g = Layer()
    horizon = 146
    gm = ys >= horizon
    depth = np.clip((ys - horizon) / (N - horizon), 0, 1)
    base = hexf('#7a9a50')[None, None, :] * (1 - depth[..., None]) + hexf('#3e6a2a')[None, None, :] * depth[..., None]
    g.rgb[gm] = base[gm]
    g.a[gm] = 1
    curve = 116 + np.sin((ys - horizon) / 22) * 10
    half = 3 + (ys - horizon) * 0.42
    path = gm & (np.abs(xs - curve) < half)
    pcol = hexf('#c8a070')[None, None, :] * (1 - depth[..., None]) + hexf('#9a744a')[None, None, :] * depth[..., None]
    g.rgb[path] = pcol[path]
    edge = gm & (np.abs(np.abs(xs - curve) - half) < 1.2)
    g.fill(edge, '#5a7a38')
    for _ in range(40):                                   # darker grass patches
        cx, cy, r = rng.uniform(0, N), rng.uniform(horizon + 6, N), rng.uniform(4, 10)
        m = gm & ~path & (((xs - cx) / (r * 1.8)) ** 2 + ((ys - cy) / r) ** 2 < 1)
        g.rgb[m] *= 0.86
    for _ in range(140):                                  # grass tufts
        x, y = rng.integers(0, N), rng.integers(horizon + 3, N)
        if path[y, x]:
            continue
        hgt = 1 + int(depth[y, x] * 4)
        for k in (-1, 0, 1):
            m = (xs == x + k) & (ys >= y - hgt + abs(k)) & (ys <= y)
            g.fill(m, '#9ad060' if k == 0 else '#5e8a3a')
    for _ in range(22):                                   # pebbles on the path
        x, y = rng.integers(0, N), rng.integers(horizon + 4, N)
        if path[y, x]:
            g.fill((np.abs(xs - x) <= 1) & (ys == y), '#e0c090')
            g.fill((np.abs(xs - x) <= 1) & (ys == y + 1), '#7a5a3a')
    for _ in range(18):
        x, y = rng.integers(0, N), rng.integers(horizon + 4, N)
        if not path[y, x]:
            g.fill((xs == x) & (ys == y), str(rng.choice(['#ffe070', '#ff8aa8', '#ffffff', '#a8c8ff'])))
    near = g.rgb.copy()
    soft = blur(g.rgb * g.a[..., None], 1.4)
    w = np.clip(1 - (ys - horizon) / 28, 0, 1)[..., None]      # far ground out of focus
    g.rgb = near * (1 - w) + soft * w
    img = over(img, g)
    # soft light pool on the ground in front of the sun
    pool = np.exp(-((xs - 140) ** 2 / (2 * 50 ** 2) + (ys - 170) ** 2 / (2 * 18 ** 2)))
    img = img + pool[..., None] * hexf('#ffd080') * 0.18
    # foreground blades, strongly out of focus
    f = Layer()
    for x0, lean in ((6, 0.6), (20, 0.3), (210, -0.4), (226, -0.7), (196, -0.2)):
        for k in range(4):
            xb = x0 + k * 3
            blade = (np.abs(xs - xb - (N - ys) * lean * 0.5) < 2.2) & (ys > 180 + k * 6)
            f.fill(blade, '#1e3a22')
    img = over(img, f, 2.6)
    # floating motes
    for _ in range(26):
        x, y = rng.uniform(0, N), rng.uniform(40, 200)
        r = rng.uniform(0.6, 1.6)
        img += (np.exp(-((xs - x) ** 2 + (ys - y) ** 2) / (2 * r * r)))[..., None] * hexf('#fff0b0') * 0.6
    # vignette and warm grade
    v = np.clip(1 - (((xs - N / 2) ** 2 + (ys - N / 2) ** 2) / (N * 0.62) ** 2) ** 1.6, 0.25, 1)
    img = img * v[..., None]
    img = img * np.array([1.04, 1.0, 0.93])
    return np.clip(img, 0, 255).astype(np.uint8)


def crypt(seed=5):
    ys, xs = np.mgrid[0:N, 0:N].astype(float)
    rng = np.random.default_rng(seed)
    img = np.zeros((N, N, 3)) + hexf('#141420')
    # back wall: large stones, blurred
    L = Layer()
    for row in range(10):
        y0 = row * 16
        off = 12 if row % 2 else 0
        for col in range(-1, 10):
            x0 = col * 26 + off
            shade = rng.uniform(0.6, 1.0)
            c = hexf('#3a3a52') * shade
            if rng.random() < 0.15:
                c = hexf('#3a5240') * shade
            L.fill((xs >= x0 + 1) & (xs < x0 + 25) & (ys >= y0 + 1) & (ys < y0 + 15) & (ys < 150), c)
    img = over(img, L, 1.2)
    # arches (pillars) in mid ground
    P = Layer()
    for x in (36, 196):
        P.fill((np.abs(xs - x) < 13) & (ys < 160), '#5a5a74')
        P.fill((np.abs(xs - x) < 13) & (np.abs(xs - x) > 10) & (ys < 160), '#3a3a50')
    img = over(img, P, 0.6)
    # torches with bloom
    for x, y in ((62, 70), (170, 70)):
        glow = np.exp(-((xs - x) ** 2 + (ys - y) ** 2) / (2 * 26 ** 2))
        img += glow[..., None] * hexf('#ff9a3a') * 0.55
        img += np.exp(-((xs - x) ** 2 + (ys - y + 3) ** 2) / (2 * 3.5 ** 2))[..., None] * hexf('#fff0a0') * 1.2
        T = Layer()
        T.fill((np.abs(xs - x) < 1.5) & (ys > y) & (ys < y + 12), '#3a2a20')
        img = over(img, T)
    # floor: crisp flagstones in perspective
    g = Layer()
    horizon = 150
    gm = ys >= horizon
    g.fill(gm, '#3a3a4c')
    for k in range(-10, 11):
        line = gm & (np.abs(xs - (N / 2 + k * (12 + (ys - horizon) * 0.55))) < 0.6)
        g.fill(line, '#24242e')
    for y in (156, 166, 180, 198, 222):
        g.fill((ys >= y) & (ys < y + 1), '#24242e')
    tex = rng.random((N, N))
    g.fill(gm & (tex < 0.05), '#4a4a5e')
    img = over(img, g)
    # green ghost light from the doorway and dust
    img += np.exp(-((xs - 116) ** 2 / (2 * 30 ** 2) + (ys - 120) ** 2 / (2 * 40 ** 2)))[..., None] * hexf('#5aff9a') * 0.12
    for _ in range(30):
        x, y = rng.uniform(0, N), rng.uniform(30, 210)
        img += np.exp(-((xs - x) ** 2 + (ys - y) ** 2) / 1.2)[..., None] * hexf('#c8ffe0') * 0.4
    v = np.clip(1 - (((xs - N / 2) ** 2 + (ys - N / 2) ** 2) / (N * 0.6) ** 2) ** 1.5, 0.15, 1)
    img = img * v[..., None]
    return np.clip(img, 0, 255).astype(np.uint8)
