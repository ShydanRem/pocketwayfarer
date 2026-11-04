"""Tiny procedural pixel-art renderer.

Shapes are drawn on a low-resolution grid with a material and a light
intensity per pixel. Colours are resolved at the end through hue-shifted
ramps (cool shadows, warm highlights), ordered dithering on the ramp
boundaries and a selective dark outline — the usual hand-made pixel art
tricks, applied consistently to every sprite.
"""
import colorsys
import math

import numpy as np

LIGHT = np.array([-0.5, -0.62, 0.6])
LIGHT = LIGHT / np.linalg.norm(LIGHT)
BAYER2 = np.array([[0.0, 0.5], [0.75, 0.25]])
BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0


def hexrgb(h):
    if not isinstance(h, str):
        return tuple(int(v) for v in h)
    h = h.lstrip('#')
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def shift(rgb, dh=0.0, ds=0.0, dv=1.0, sat_mul=1.0):
    r, g, b = [c / 255.0 for c in rgb]
    h, s, v = colorsys.rgb_to_hsv(r, g, b)
    h = (h + dh / 360.0) % 1.0
    s = min(1.0, max(0.0, s * sat_mul + ds))
    v = min(1.0, max(0.0, v * dv))
    r, g, b = colorsys.hsv_to_rgb(h, s, v)
    return (round(r * 255), round(g * 255), round(b * 255))


def toward(h_from, target, amount):
    """Hue shift in degrees from h_from toward target by at most amount."""
    d = (target - h_from + 540) % 360 - 180
    return max(-amount, min(amount, d))


def ramp(base_hex):
    """Five shades: deep, shadow, base, light, highlight."""
    base = hexrgb(base_hex)
    h = colorsys.rgb_to_hsv(*[c / 255 for c in base])[0] * 360
    cool = lambda a: toward(h, 250, a)
    warm = lambda a: toward(h, 55, a)
    return [
        shift(base, cool(26), 0.12, 0.42),
        shift(base, cool(14), 0.08, 0.68),
        base,
        shift(base, warm(8), -0.06, 1.14),
        shift(base, warm(14), 0, 1.32, sat_mul=0.45),
    ]


class Canvas:
    def __init__(self, w, h, materials):
        self.w, self.h = w, h
        self.mat = np.full((h, w), -1, dtype=int)
        self.lum = np.zeros((h, w))
        self.over = {}                      # (x, y) -> rgb, drawn after shading
        self.names = list(materials.keys())
        self.ramps = [ramp(materials[k]) for k in self.names]
        self.flat = set()                   # materials that ignore lighting
        self.ys, self.xs = np.mgrid[0:h, 0:w]
        self.xs = self.xs + 0.5
        self.ys = self.ys + 0.5

    def m(self, name):
        return self.names.index(name)

    # ---- primitives --------------------------------------------------
    def ellipse(self, cx, cy, rx, ry, mat, angle=0.0, patch=False, clip=None, bulge=1.0, light=None):
        a = math.radians(angle)
        dx, dy = self.xs - cx, self.ys - cy
        u = (dx * math.cos(a) + dy * math.sin(a)) / rx
        v = (-dx * math.sin(a) + dy * math.cos(a)) / ry
        d2 = u * u + v * v
        mask = d2 <= 1.0
        if clip is not None:
            mask &= clip(self.xs, self.ys)
        nz = np.sqrt(np.clip(1 - d2, 0, 1)) * bulge
        nx = u * math.cos(a) - v * math.sin(a)
        ny = u * math.sin(a) + v * math.cos(a)
        n = np.stack([nx, ny, nz + 0.2], axis=-1)
        n = n / np.linalg.norm(n, axis=-1, keepdims=True)
        lum = np.clip(n @ LIGHT, 0, 1) * 0.8 + 0.2 if light is None else np.full(d2.shape, light)
        self._apply(mask, mat, lum, patch)

    def circle(self, cx, cy, r, mat, **kw):
        self.ellipse(cx, cy, r, r, mat, **kw)

    def poly(self, pts, mat, light=0.62, grad=0.25, patch=False):
        """Filled polygon with a soft top-left gradient."""
        pts = np.array(pts, dtype=float)
        mask = np.zeros((self.h, self.w), dtype=bool)
        n = len(pts)
        inside = np.zeros((self.h, self.w), dtype=bool)
        for i in range(n):
            x1, y1 = pts[i]
            x2, y2 = pts[(i + 1) % n]
            cond = ((y1 > self.ys) != (y2 > self.ys))
            with np.errstate(divide='ignore', invalid='ignore'):
                xint = (x2 - x1) * (self.ys - y1) / (y2 - y1 + 1e-9) + x1
            inside ^= cond & (self.xs < xint)
        mask |= inside
        minx, miny = pts.min(axis=0)
        maxx, maxy = pts.max(axis=0)
        t = ((self.xs - minx) / max(1, maxx - minx) + (self.ys - miny) / max(1, maxy - miny)) / 2
        lum = np.clip(light + grad * (0.5 - t) * 2, 0, 1)
        self._apply(mask, mat, lum, patch)

    def rect(self, x, y, w, h, mat, light=0.62, round_=0, patch=False):
        mask = (self.xs >= x) & (self.xs < x + w) & (self.ys >= y) & (self.ys < y + h)
        if round_:
            for cx, cy in ((x + round_, y + round_), (x + w - round_, y + round_),
                           (x + round_, y + h - round_), (x + w - round_, y + h - round_)):
                corner = ((self.xs < x + round_) | (self.xs > x + w - round_)) & \
                         ((self.ys < y + round_) | (self.ys > y + h - round_))
                far = (self.xs - cx) ** 2 + (self.ys - cy) ** 2 > round_ * round_
                near_x = np.abs(self.xs - cx) <= round_
                near_y = np.abs(self.ys - cy) <= round_
                mask &= ~(corner & far & near_x & near_y)
        t = ((self.xs - x) / max(1, w) + (self.ys - y) / max(1, h)) / 2
        lum = np.clip(light + 0.3 * (0.5 - t), 0, 1)
        self._apply(mask, mat, lum, patch)

    def flame(self, cx, by, w, h, mat, angle=0.0, patch=False):
        """Teardrop pointing up (rotated by angle), base centred at (cx, by)."""
        a = math.radians(angle)
        dx, dy = self.xs - cx, self.ys - by
        u = dx * math.cos(a) + dy * math.sin(a)
        v = -dx * math.sin(a) + dy * math.cos(a)
        r = w / 2
        t = np.clip(-(v + r) / max(1, h - r), 0, 1)     # 0 at bulb centre, 1 at tip
        halfw = np.where(v > -r, np.sqrt(np.clip(r * r - (v + r) ** 2, 0, None)), r * (1 - t) ** 1.4)
        mask = (np.abs(u) <= halfw) & (v <= 0.5 + 0) & (v >= -h)
        lum = np.clip(0.95 - t * 0.35 - np.abs(u) / (r + 1e-6) * 0.35, 0, 1)
        self._apply(mask, mat, lum, patch)

    def line(self, pts, mat, width=1.0, light=0.55):
        mask = np.zeros((self.h, self.w), dtype=bool)
        for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
            px, py = self.xs - x1, self.ys - y1
            vx, vy = x2 - x1, y2 - y1
            L = vx * vx + vy * vy or 1
            t = np.clip((px * vx + py * vy) / L, 0, 1)
            d = np.hypot(px - t * vx, py - t * vy)
            mask |= d <= width / 2
        self._apply(mask, mat, np.full((self.h, self.w), light), False)

    def clear(self, mask_fn):
        mask = mask_fn(self.xs, self.ys)
        self.mat[mask] = -1

    def _apply(self, mask, mat, lum, patch):
        k = self.m(mat)
        if patch:
            mask = mask & (self.mat >= 0)
            self.mat[mask] = k
            return
        self.mat[mask] = k
        self.lum[mask] = lum[mask] if hasattr(lum, 'shape') else lum

    # ---- details drawn after shading -------------------------------
    def px(self, x, y, color):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.over[(int(x), int(y))] = hexrgb(color) if isinstance(color, str) else color

    def pxs(self, x0, y0, rows, colors):
        """rows: list of strings; colors: dict char -> hex."""
        for j, row in enumerate(rows):
            for i, ch in enumerate(row):
                if ch in colors:
                    self.px(x0 + i, y0 + j, colors[ch])

    def filled(self, x, y):
        return 0 <= x < self.w and 0 <= y < self.h and (self.mat[y, x] >= 0 or (x, y) in self.over)

    # ---- resolve ---------------------------------------------------
    def render(self, outline=True, outline_color=None, dither=True):
        img = np.zeros((self.h, self.w, 4), dtype=np.uint8)
        for y in range(self.h):
            for x in range(self.w):
                k = self.mat[y, x]
                if k < 0:
                    continue
                L = self.lum[y, x]
                t = BAYER2[y % 2, x % 2] - 0.375 if dither else 0
                level = L * 4.0 + t * 0.55
                idx = int(max(0, min(4, round(level - 0.5 + 0.5))))
                if idx == 4 and L < 0.9:
                    idx = 3
                img[y, x, :3] = self.ramps[k][idx]
                img[y, x, 3] = 255
        for (x, y), c in self.over.items():
            img[y, x, :3] = c
            img[y, x, 3] = 255
        if outline:
            solid = img[:, :, 3] > 0
            out = img.copy()
            for y in range(self.h):
                for x in range(self.w):
                    if solid[y, x]:
                        continue
                    neigh = [(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
                    hits = [(nx, ny) for nx, ny in neigh if 0 <= nx < self.w and 0 <= ny < self.h and solid[ny, nx]]
                    if not hits:
                        continue
                    if outline_color:
                        c = hexrgb(outline_color)
                    else:
                        nx, ny = hits[0]
                        k = self.mat[ny, nx]
                        base = self.ramps[k][0] if k >= 0 else tuple(img[ny, nx, :3])
                        c = shift(base, 0, 0.1, 0.55)
                    out[y, x, :3] = c
                    out[y, x, 3] = 255
            img = out
        return img


EYES = {
    # K ink, W shine, I iris tint, L lower highlight
    (2, 3): ['WK', 'KK', 'KI'],
    (3, 4): ['KWK', 'WKK', 'KKK', 'KIK'],
    (3, 5): ['.K.', 'KWK', 'WKK', 'KKK', 'KIK'],
    (4, 5): ['.KK.', 'KWWK', 'KWKK', 'KKIK', '.KK.'],
    (4, 6): ['.KK.', 'KWWK', 'KWKK', 'KKKK', 'KIIK', '.KK.'],
    (5, 6): ['.KKK.', 'KWWKK', 'KWWKK', 'KKKKK', 'KKIIK', '.KKK.'],
}


def eye(c, x, y, w, h, kind='open', iris=None, ink='#1d1430', shine='#ffffff', mirror=False):
    """Draws an eye whose top-left corner is (x, y)."""
    x, y = int(x), int(y)
    iris = iris or ink
    if kind in ('open', 'angry'):
        rows = EYES.get((w, h)) or [''.join('K' for _ in range(w))] * h
        if mirror:
            rows = [r[::-1] for r in rows]
        if kind == 'angry':                       # flat top lid
            rows = ['.' * w] + rows[1:]
        c.pxs(x, y, rows, {'K': ink, 'W': shine, 'I': iris})
    elif kind == 'blink':
        for i in range(w):
            c.px(x + i, y + h - 2, ink)
    elif kind == 'happy':
        mid = y + h // 2 - 1
        for i in range(w):
            c.px(x + i, mid + (1 if i in (0, w - 1) else 0), ink)
    elif kind == 'sad':
        for i in range(w):
            c.px(x + i, y + h - 2, ink)
        for i in range(1, w - 1):
            c.px(x + i, y + h - 1, ink)
    elif kind == 'glow':
        for j in range(h):
            for i in range(w):
                c.px(x + i, y + j, iris)
        c.px(x, y, shine)
    elif kind == 'shut':
        for i in range(w):
            c.px(x + i, y + h - 2 + (1 if i in (0, w - 1) else 0), ink)


def brow(c, x, y, w, inward_right=True, ink='#1d1430'):
    for i in range(w):
        dy = i if inward_right else (w - 1 - i)
        c.px(x + i, y + dy // 2, ink)


def to_png(img, path, scale=1):
    from PIL import Image
    im = Image.fromarray(img, 'RGBA')
    if scale != 1:
        im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
    im.save(path)
