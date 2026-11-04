"""Hand-drawn pixel art helpers: sprites are ASCII grids + a palette."""
import numpy as np


def hexrgb(h):
    h = h.lstrip('#')
    return tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))


def grid(rows, palette, w=None):
    w = w or max(len(r) for r in rows)
    img = np.zeros((len(rows), w, 4), np.uint8)
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch in ('.', ' ') or ch not in palette:
                continue
            img[y, x, :3] = hexrgb(palette[ch])
            img[y, x, 3] = 255
    return img


def check(rows, w):
    bad = [(i, len(r)) for i, r in enumerate(rows) if len(r) != w]
    if bad:
        raise SystemExit(f'rows with wrong width: {bad}')


def show(imgs, path, scale=6, bg=(48, 44, 64)):
    from PIL import Image
    W = sum(i.shape[1] for i in imgs) + 4 * (len(imgs) + 1)
    H = max(i.shape[0] for i in imgs) + 8
    s = Image.new('RGBA', (W, H), bg + (255,))
    x = 4
    for i in imgs:
        s.alpha_composite(Image.fromarray(i, 'RGBA'), (x, 4))
        x += i.shape[1] + 4
    s.resize((W * scale, H * scale), Image.NEAREST).save(path)
