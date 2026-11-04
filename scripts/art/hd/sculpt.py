"""Higher-fidelity procedural pixel art for detailed (non-chibi) sprites.

Every primitive carries a surface normal, a material and a depth. Colours come
from 6-tone hue-shifted ramps with clean bands (no dithering), then three
hand-pixelling tricks are applied automatically:

  * contact shadows: a part touching something in front of it darkens a step
  * selective outline: the silhouette takes a darker tone of its own material
  * rim light: a warm back light catches the right-hand edge (HD-2D look)
"""
import colorsys
import math

import numpy as np

L_KEY = np.array([-0.55, -0.55, 0.63])
L_KEY = L_KEY / np.linalg.norm(L_KEY)


def hexrgb(h):
    if not isinstance(h, str):
        return tuple(int(v) for v in h)
    h = h.lstrip('#')
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def _hsv(rgb):
    return colorsys.rgb_to_hsv(*[c / 255.0 for c in rgb])


def _rgb(h, s, v):
    r, g, b = colorsys.hsv_to_rgb(h % 1.0, min(1, max(0, s)), min(1, max(0, v)))
    return (round(r * 255), round(g * 255), round(b * 255))


def ramp6(base_hex, metal=False):
    """deep, dark, shade, base, light, shine — shadows drift to violet, lights to gold."""
    h, s, v = _hsv(hexrgb(base_hex))

    def hue_to(target, amt):
        d = ((target - h * 360 + 540) % 360) - 180
        return h + max(-amt, min(amt, d)) / 360.0

    cool = lambda a: hue_to(255, a)
    warm = lambda a: hue_to(48, a)
    if metal:
        return [_rgb(cool(30), s + 0.15, v * 0.28), _rgb(cool(22), s + 0.1, v * 0.48), _rgb(cool(10), s + 0.04, v * 0.72),
                _rgb(h, s, v), _rgb(warm(8), s * 0.6, min(1, v * 1.2 + 0.05)), _rgb(warm(12), s * 0.15, 1.0)]
    return [_rgb(cool(32), s + 0.18, v * 0.32), _rgb(cool(22), s + 0.12, v * 0.52), _rgb(cool(10), s + 0.06, v * 0.76),
            _rgb(h, s, v), _rgb(warm(8), s * 0.85, min(1, v * 1.12 + 0.03)), _rgb(warm(14), s * 0.45, min(1, v * 1.3 + 0.08))]


class Sculpt:
    def __init__(self, w, h, materials, metal=()):
        self.w, self.h = w, h
        self.names = list(materials)
        self.ramps = [ramp6(materials[k], k in metal) for k in self.names]
        self.mat = np.full((h, w), -1, int)
        self.lum = np.zeros((h, w))
        self.part = np.full((h, w), -1, int)
        self.depth = np.full((h, w), -1e9)
        self.flat = {}                          # exact colours drawn last
        ys, xs = np.mgrid[0:h, 0:w]
        self.xs, self.ys = xs + 0.5, ys + 0.5
        self.z = 0.0
        self.parts = 0

    def m(self, name):
        return self.names.index(name)

    # -- core -------------------------------------------------------------
    def _put(self, mask, mat, nx, ny, nz, z=None, light_bias=0.0, part=None):
        if z is None:
            self.z += 1
            z = self.z
        if part is None:
            self.parts += 1
            part = self.parts
        n = np.stack([nx, ny, nz], -1)
        n /= np.linalg.norm(n, axis=-1, keepdims=True) + 1e-9
        lum = np.clip(n @ L_KEY, 0, 1) * 0.85 + 0.15 + light_bias
        m = mask & (z >= self.depth)
        k = self.m(mat)
        self.mat[m] = k
        self.lum[m] = lum[m]
        self.depth[m] = z
        self.part[m] = part
        return part

    def ellipsoid(self, cx, cy, rx, ry, mat, angle=0.0, z=None, flat=0.0, bias=0.0, part=None, clip=None):
        a = math.radians(angle)
        dx, dy = self.xs - cx, self.ys - cy
        u = (dx * math.cos(a) + dy * math.sin(a)) / rx
        v = (-dx * math.sin(a) + dy * math.cos(a)) / ry
        d2 = u * u + v * v
        mask = d2 <= 1
        if clip is not None:
            mask &= clip(self.xs, self.ys)
        nz = np.sqrt(np.clip(1 - d2, 0, 1)) + flat
        nx = u * math.cos(a) - v * math.sin(a)
        ny = u * math.sin(a) + v * math.cos(a)
        return self._put(mask, mat, nx, ny, nz, z, bias, part)

    def capsule(self, p1, p2, r1, r2, mat, z=None, bias=0.0, part=None):
        """Tapered limb with cylindrical shading and round ends."""
        (x1, y1), (x2, y2) = p1, p2
        vx, vy = x2 - x1, y2 - y1
        L2 = vx * vx + vy * vy or 1e-6
        px, py = self.xs - x1, self.ys - y1
        t = np.clip((px * vx + py * vy) / L2, 0, 1)
        cx, cy = x1 + t * vx, y1 + t * vy
        dx, dy = self.xs - cx, self.ys - cy
        r = r1 + (r2 - r1) * t
        d = np.hypot(dx, dy)
        mask = d <= r
        q = np.clip(d / np.maximum(r, 1e-6), 0, 1)
        nz = np.sqrt(1 - q * q)
        nx = np.where(d > 0, dx / (d + 1e-9) * q, 0)
        ny = np.where(d > 0, dy / (d + 1e-9) * q, 0)
        return self._put(mask, mat, nx, ny, nz, z, bias, part)

    def poly(self, pts, mat, normal=(-0.12, -0.18, 1.0), z=None, folds=0.0, bias=0.0, part=None, curve=0.0):
        """Flat panel; `folds` adds vertical cloth folds, `curve` bends the normal across x."""
        pts = np.array(pts, float)
        inside = np.zeros((self.h, self.w), bool)
        n = len(pts)
        for i in range(n):
            x1, y1 = pts[i]
            x2, y2 = pts[(i + 1) % n]
            cond = (y1 > self.ys) != (y2 > self.ys)
            with np.errstate(divide='ignore', invalid='ignore'):
                xi = (x2 - x1) * (self.ys - y1) / (y2 - y1 + 1e-9) + x1
            inside ^= cond & (self.xs < xi)
        x0, x1_ = pts[:, 0].min(), pts[:, 0].max()
        u = (self.xs - (x0 + x1_) / 2) / max(1, (x1_ - x0) / 2)
        nx = np.full((self.h, self.w), normal[0]) + u * curve
        if folds:
            nx = nx + np.sin(self.xs * folds) * 0.35
        ny = np.full((self.h, self.w), normal[1])
        nz = np.full((self.h, self.w), normal[2])
        return self._put(inside, mat, nx, ny, nz, z, bias, part)

    def px(self, x, y, color):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.flat[(int(x), int(y))] = hexrgb(color)

    def pxs(self, x0, y0, rows, colors):
        for j, row in enumerate(rows):
            for i, ch in enumerate(row):
                if ch in colors:
                    self.px(x0 + i, y0 + j, colors[ch])

    def erase(self, fn):
        m = fn(self.xs, self.ys)
        self.mat[m] = -1
        self.part[m] = -1
        self.depth[m] = -1e9

    # -- resolve ---------------------------------------------------------
    def render(self, rim=True, outline=True, rim_color=(255, 208, 140), bands=(0.34, 0.5, 0.66, 0.83, 0.95)):
        h, w = self.h, self.w
        idx = np.zeros((h, w), int)
        for b in bands:
            idx += self.lum > b
        solid = self.mat >= 0
        # contact shadows: something nearer is right next to me
        shade = np.zeros((h, w), int)
        for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
            nb_d = np.full((h, w), -1e9)
            nb_p = np.full((h, w), -1)
            ys = slice(max(0, dy), h + min(0, dy))
            yd = slice(max(0, -dy), h + min(0, -dy))
            xs = slice(max(0, dx), w + min(0, dx))
            xd = slice(max(0, -dx), w + min(0, -dx))
            nb_d[yd, xd] = self.depth[ys, xs]
            nb_p[yd, xd] = self.part[ys, xs]
            hit = solid & (nb_p >= 0) & (nb_p != self.part) & (nb_d > self.depth + 0.5)
            shade = np.maximum(shade, hit.astype(int))
        idx = np.clip(idx - shade * 2, 0, 5)
        img = np.zeros((h, w, 4), np.uint8)
        for y in range(h):
            for x in range(w):
                k = self.mat[y, x]
                if k < 0:
                    continue
                img[y, x, :3] = self.ramps[k][idx[y, x]]
                img[y, x, 3] = 255
        if rim:
            # pixels whose right neighbour is empty catch the back light
            edge = solid.copy()
            edge[:, :-1] &= ~solid[:, 1:]
            edge[:, -1] = solid[:, -1]
            up = np.zeros_like(solid)
            up[1:, :] = solid[:-1, :]
            edge &= up       # not on the very top pixel row of a shape
            for y, x in zip(*np.where(edge)):
                if idx[y, x] >= 4:
                    continue
                c = np.array(img[y, x, :3], float)
                img[y, x, :3] = np.clip(c * 0.62 + np.array(rim_color) * 0.38, 0, 255).astype(np.uint8)
        for (x, y), c in self.flat.items():
            img[y, x, :3] = c
            img[y, x, 3] = 255
        if outline:
            a = img[:, :, 3] > 0
            out = img.copy()
            for y in range(h):
                for x in range(w):
                    if a[y, x]:
                        continue
                    best = None
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        nx, ny = x + dx, y + dy
                        if 0 <= nx < w and 0 <= ny < h and a[ny, nx]:
                            k = self.mat[ny, nx]
                            col = self.ramps[k][0] if k >= 0 else tuple(img[ny, nx, :3])
                            best = tuple(int(c * 0.55) for c in col)
                            break
                    if best:
                        out[y, x, :3] = best
                        out[y, x, 3] = 255
            img = out
        return img
