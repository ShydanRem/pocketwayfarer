"""Static style mock-ups composed in Python (not the firmware UI)."""
import sys
import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, '..')
import font5x7 as F
from figure import warrior, mage
from beasts import dire_wolf, skeleton_knight
from scenes_hd import forest, crypt


def text(img, s, x, y, color=(255, 246, 224), scale=2, shadow=True, center=True):
    d = ImageDraw.Draw(img)
    glyphs = [F.glyph_rows(ch) for ch in s]
    w = sum((max(len(r) for r in g[0]) + 1) * scale for g in glyphs)
    if center:
        x -= w // 2
    for pass_, col in ((0, (20, 14, 30)), (1, color)) if shadow else ((1, color),):
        cx = x + (scale if pass_ == 0 else 0)
        oy = scale if pass_ == 0 else 0
        for rows, top in glyphs:
            for j, row in enumerate(rows):
                for i, ch in enumerate(row):
                    if ch == '#':
                        d.rectangle([cx + i * scale, y + oy + (j + top) * scale, cx + i * scale + scale - 1,
                                     y + oy + (j + top) * scale + scale - 1], fill=col)
            cx += (max(len(r) for r in rows) + 1) * scale


def sprite(img, arr, cx, by, scale=2):
    s = Image.fromarray(arr, 'RGBA')
    s = s.resize((s.width * scale, s.height * scale), Image.NEAREST)
    img.alpha_composite(s, (int(cx - s.width / 2), int(by - s.height)))


def shadow(img, cx, by, rx, ry):
    sh = Image.new('RGBA', img.size, (0, 0, 0, 0))
    ImageDraw.Draw(sh).ellipse([cx - rx, by - ry, cx + rx, by + ry], fill=(10, 8, 20, 110))
    img.alpha_composite(sh)


def arc(img, start, end, color, value=1.0, r=226, w=14):
    d = ImageDraw.Draw(img)
    box = [233 - r, 233 - r, 233 + r, 233 + r]
    d.arc(box, start, end, fill=(20, 16, 34), width=w)
    d.arc([box[0] + 3, box[1] + 3, box[2] - 3, box[3] - 3], start, start + (end - start) * value, fill=color, width=w - 6)


def button(img, cx, cy, r, color, label=None):
    d = ImageDraw.Draw(img)
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(20, 16, 34))
    d.ellipse([cx - r + 3, cy - r + 3, cx + r - 3, cy + r - 3], fill=color)
    d.ellipse([cx - r + 6, cy - r + 5, cx + r - 6, cy - 2], fill=tuple(min(255, c + 40) for c in color))
    if label:
        text(img, label, cx, cy - 7, scale=2)


def battle(bg, foes, hero, title):
    img = Image.fromarray(bg).resize((466, 466), Image.NEAREST).convert('RGBA')
    for arr, x, by in foes:
        shadow(img, x, by - 4, 34, 7)
        sprite(img, arr, x, by)
    shadow(img, 350, 372, 28, 6)
    sprite(img, hero, 350, 376)
    # target cursor and damage
    text(img, '-1288', foes[0][1], foes[0][2] - 110, (255, 220, 90), scale=3)
    text(img, 'CRITICO!', foes[0][1], foes[0][2] - 78, (255, 220, 90), scale=2)
    # vitals on the rim, name plate, commands
    arc(img, 98, 172, (108, 224, 106), 0.8)
    arc(img, 8, 82, (90, 160, 255), 0.55)
    arc(img, 200, 340, (255, 90, 90), 0.4, w=10)
    text(img, title, 233, 38)
    plate = Image.new('RGBA', img.size, (0, 0, 0, 0))
    ImageDraw.Draw(plate).rounded_rectangle([296, 134, 420, 160], 6, fill=(18, 20, 40, 200))
    img.alpha_composite(plate)
    text(img, 'Aldric Lv24', 358, 140, scale=2)
    for i, (cx, cy, col) in enumerate(((150, 410, (200, 70, 50)), (205, 432, (130, 80, 220)),
                                        (262, 432, (170, 120, 60)), (317, 410, (60, 120, 220)))):
        button(img, cx, cy, 24, col)
    mask = Image.new('L', (466, 466), 0)
    ImageDraw.Draw(mask).ellipse((0, 0, 465, 465), fill=255)
    out = Image.new('RGBA', (466, 466), (0, 0, 0, 0))
    out.paste(img, (0, 0), mask)
    return out


if __name__ == '__main__':
    out_dir = sys.argv[1]
    a = battle(forest(), [(dire_wolf(1), 130, 352), (dire_wolf(0), 175, 262)], warrior(0), 'Bosco Sussurrante')
    b = battle(crypt(), [(skeleton_knight(1), 120, 360), (skeleton_knight(0), 180, 268)], mage(0), 'Cripta dei Lumi')
    sheet = Image.new('RGBA', (466 * 2 + 20, 466), (18, 18, 28, 255))
    sheet.alpha_composite(a, (0, 0))
    sheet.alpha_composite(b, (486, 0))
    sheet.save(out_dir + '/mockup.png')
