"""Companion forms, drawn procedurally on a 48x48 grid (shown at 3x on the device)."""
from canvas import Canvas, eye, brow

SIZE = 48
INK = '#1d1430'
EXPRESSIONS = ['idle', 'blink', 'happy', 'sad', 'eat', 'sleep']

MOUTHS = {
    'smile': ['K...K', '.KKK.'],
    'cat': ['K.K.K', '.K.K.'],
    'open': ['.KKK.', 'KRRRK', 'KRRRK', '.KKK.'],
    'flat': ['.KKK.'],
    'o': ['.K.', 'K.K', '.K.'],
    'fang': ['K...K', '.KKK.', '.W.W.'],
    'grin': ['KKKKK', 'KWWWK', '.KKK.'],
}


def face(c, cx, ey, gap, ew, eh, expr, my, blush='#ff8fa3', mouth='smile', iris=None, wild=False,
         ink=INK):
    lx = int(cx - gap / 2 - ew + 0.5)
    rx = int(cx + gap / 2 + 0.5)
    kind = {'idle': 'open', 'blink': 'blink', 'happy': 'happy', 'sad': 'sad', 'eat': 'happy',
            'sleep': 'shut'}[expr]
    eye(c, lx, ey, ew, eh, kind, iris=iris, ink=ink)
    eye(c, rx, ey, ew, eh, kind, iris=iris, ink=ink, mirror=True)
    if wild and expr in ('idle', 'blink'):
        brow(c, lx - 1, ey - 2, ew + 1, True, ink)
        brow(c, rx, ey - 2, ew + 1, False, ink)
    if expr == 'sad':
        brow(c, lx, ey - 2, ew, False, ink)
        brow(c, rx, ey - 2, ew, True, ink)
    if blush and expr != 'sleep':
        for dx in (0, 1, 2):
            c.px(lx - 2 + dx, ey + eh + (0 if dx != 1 else 0), blush)
            c.px(rx + ew - 1 + dx, ey + eh, blush)
    m = {'idle': mouth, 'blink': mouth, 'happy': 'open', 'sad': 'flat', 'eat': 'open', 'sleep': 'o'}[expr]
    if expr == 'sleep':
        m = 'flat'
    rows = MOUTHS[m]
    w = len(rows[0])
    c.pxs(int(cx - w / 2 + 0.5), my, rows, {'K': ink, 'R': '#e8506a', 'W': '#ffffff'})


def feet(c, cx, y, spread, mat, rx=4.5, ry=3):
    c.ellipse(cx - spread, y, rx, ry, mat)
    c.ellipse(cx + spread, y, rx, ry, mat)


# --------------------------------------------------------------------------
def seed(frame=0):
    c = Canvas(SIZE, SIZE, {'shell': '#fff0d2', 'spot': '#73e0d0', 'glow': '#c8fff4', 'crack': '#5a4a6a'})
    c.ellipse(24, 27, 14, 18, 'shell')
    for (x, y, r) in ((17, 20, 3.5), (30, 28, 4.5), (20, 36, 3), (31, 15, 2.5), (14, 30, 2)):
        c.ellipse(x, y, r, r * 0.85, 'spot', patch=True)
    c.ellipse(24, 27, 6, 7, 'glow', patch=True, light=0.95) if frame == 2 else None
    img_cracks = {
        1: [(18, 12), (21, 15), (19, 18), (23, 20)],
        2: [(18, 12), (21, 15), (19, 18), (23, 20), (27, 18), (30, 21), (33, 19)],
    }
    if frame:
        pts = img_cracks[frame]
        for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
            n = max(abs(x2 - x1), abs(y2 - y1))
            for i in range(n + 1):
                c.px(round(x1 + (x2 - x1) * i / n), round(y1 + (y2 - y1) * i / n), '#4a3a5a')
    # sparkle
    c.pxs(31, 9, ['.W.', 'WWW', '.W.'], {'W': '#ffffff'})
    return c.render()


def lumino(expr):
    c = Canvas(SIZE, SIZE, {'body': '#ffe48a', 'belly': '#fff6d0', 'bulb': '#8ff3ff', 'stalk': '#e8c060',
                            'arm': '#f5d26e'})
    feet(c, 24, 42, 7, 'arm')
    c.ellipse(9, 31, 4, 3.5, 'arm', angle=-30)
    c.ellipse(39, 31, 4, 3.5, 'arm', angle=30)
    c.ellipse(24, 29, 16, 14.5, 'body')
    c.ellipse(24, 34, 9, 7, 'belly', patch=True)
    c.line([(24, 15), (25, 11), (27, 8)], 'stalk', width=1.6)
    c.circle(27.5, 6.5, 3.6, 'bulb')
    face(c, 24, 24, 9, 4, 5, expr, 32)
    return c.render()


def favillo(expr):
    c = Canvas(SIZE, SIZE, {'body': '#ff7b3a', 'belly': '#ffd9a8', 'flame': '#ff4a2a', 'core': '#ffd23f',
                            'ear': '#e85a2a', 'inner': '#ffb08a'})
    c.flame(40, 38, 8, 15, 'flame', angle=35)
    c.flame(40, 37, 4, 9, 'core', angle=35)
    feet(c, 24, 43, 7, 'ear')
    c.poly([(9, 22), (12, 8), (19, 17)], 'ear')
    c.poly([(39, 22), (36, 8), (29, 17)], 'ear')
    c.poly([(11.5, 19), (12.5, 12), (16, 17)], 'inner', light=0.8)
    c.poly([(36.5, 19), (35.5, 12), (32, 17)], 'inner', light=0.8)
    c.ellipse(24, 29, 15, 13.5, 'body')
    c.ellipse(24, 35, 8.5, 6.5, 'belly', patch=True)
    c.flame(24, 18, 11, 17, 'flame')
    c.flame(24, 18, 6, 11, 'core')
    face(c, 24, 25, 9, 4, 5, expr, 33, mouth='cat')
    return c.render()


def spruzzo(expr):
    c = Canvas(SIZE, SIZE, {'body': '#4aaeff', 'belly': '#c8ecff', 'fin': '#7fd6ff', 'bubble': '#e8fbff'})
    c.poly([(8, 28), (1, 21), (4, 34)], 'fin', light=0.7)
    c.poly([(40, 28), (47, 21), (44, 34)], 'fin', light=0.7)
    feet(c, 24, 43, 7, 'fin')
    c.ellipse(24, 30, 15, 13.5, 'body')
    c.poly([(24, 3), (12, 23), (36, 23)], 'body', light=0.66, grad=0.3)
    c.ellipse(24, 35, 9, 6.5, 'belly', patch=True)
    c.circle(39, 9, 2.6, 'bubble')
    c.circle(43, 4, 1.6, 'bubble')
    face(c, 24, 25, 9, 4, 5, expr, 33)
    return c.render()


def fogliolo(expr):
    c = Canvas(SIZE, SIZE, {'body': '#6fd15c', 'belly': '#dcf7c8', 'leaf': '#3fae4f', 'vein': '#2c8a3c',
                            'stem': '#8a5a2b'})
    feet(c, 24, 43, 7, 'leaf')
    c.ellipse(24, 30, 15.5, 13.5, 'body')
    c.ellipse(24, 35, 9, 6.5, 'belly', patch=True)
    c.line([(24, 17), (24, 12)], 'stem', width=1.5)
    c.ellipse(17, 9, 7.5, 3.6, 'leaf', angle=-25)
    c.ellipse(31, 9, 7.5, 3.6, 'leaf', angle=25)
    c.ellipse(8, 22, 5, 2.5, 'leaf', angle=40)
    c.ellipse(40, 22, 5, 2.5, 'leaf', angle=-40)
    for i in range(5):
        c.px(13 + i, 10 - i // 2, '#2c8a3c')
        c.px(34 - i, 10 - i // 2, '#2c8a3c')
    face(c, 24, 25, 9, 4, 5, expr, 33)
    return c.render()


def pirolume(expr):
    c = Canvas(SIZE, SIZE, {'body': '#ff8c42', 'belly': '#fff0c8', 'ear': '#e8662e', 'inner': '#ffd0a0',
                            'flame': '#ff5a2a', 'core': '#ffe066', 'tip': '#fff6e0'})
    c.flame(41, 40, 12, 24, 'flame', angle=28)
    c.flame(41, 39, 6, 15, 'core', angle=28)
    feet(c, 24, 44, 8, 'ear', rx=5)
    c.ellipse(24, 36, 11, 9, 'body')
    c.ellipse(24, 38, 6, 6, 'belly', patch=True)
    c.poly([(8, 20), (6, 3), (19, 12)], 'ear')
    c.poly([(40, 20), (42, 3), (29, 12)], 'ear')
    c.poly([(9.5, 16), (8.5, 7), (15, 12)], 'inner', light=0.8)
    c.poly([(38.5, 16), (39.5, 7), (33, 12)], 'inner', light=0.8)
    for x, a in ((9, -50), (12, -25), (36, 25), (39, 50)):
        c.flame(x, 31, 6, 10, 'flame', angle=a)
    c.ellipse(24, 21, 15, 12, 'body')
    c.ellipse(24, 26, 9, 5.5, 'belly', patch=True)
    c.flame(24, 11, 8, 12, 'flame')
    c.flame(24, 11, 4, 7, 'core')
    face(c, 24, 17, 10, 4, 5, expr, 25, mouth='cat')
    return c.render()


def cenerotto(expr):
    c = Canvas(SIZE, SIZE, {'body': '#4c4560', 'belly': '#6b6080', 'horn': '#2e2a3c', 'crack': '#ff7a1a',
                            'flame': '#ff3d1f', 'core': '#ffb02e'})
    c.flame(41, 41, 10, 20, 'flame', angle=30)
    feet(c, 24, 44, 8, 'horn', rx=5)
    c.ellipse(24, 35, 13, 10, 'body')
    c.poly([(9, 16), (3, 3), (16, 11)], 'horn', light=0.5)
    c.poly([(39, 16), (45, 3), (32, 11)], 'horn', light=0.5)
    c.ellipse(24, 21, 16, 12.5, 'body')
    c.ellipse(24, 37, 7, 6, 'belly', patch=True)
    for pts in ([(12, 26), (15, 29), (14, 33)], [(35, 25), (33, 29), (36, 32)], [(21, 38), (24, 41), (27, 38)]):
        for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
            n = max(abs(x2 - x1), abs(y2 - y1))
            for i in range(n + 1):
                c.px(round(x1 + (x2 - x1) * i / n), round(y1 + (y2 - y1) * i / n), '#ff8a2a')
    c.flame(24, 10, 9, 11, 'flame')
    c.flame(24, 10, 4, 6, 'core')
    face(c, 24, 18, 10, 4, 5, expr, 26, mouth='fang', iris='#ff7a1a', wild=True, blush=None)
    return c.render()


def coralba(expr):
    c = Canvas(SIZE, SIZE, {'body': '#5cc6e6', 'belly': '#e0f8ff', 'coral': '#ff8fb5', 'fin': '#9ae6ff',
                            'pearl': '#fff4fa'})
    c.poly([(8, 32), (0, 26), (3, 40)], 'fin', light=0.75)
    c.poly([(40, 32), (48, 26), (45, 40)], 'fin', light=0.75)
    feet(c, 24, 44, 8, 'fin', rx=5)
    c.ellipse(24, 36, 11.5, 9, 'body')
    c.ellipse(24, 38, 6.5, 6, 'belly', patch=True)
    for side in (-1, 1):
        base = 24 + side * 8
        c.line([(base, 12), (base + side * 5, 4), (base + side * 9, 2)], 'coral', width=2)
        c.line([(base + side * 5, 5), (base + side * 2, 0)], 'coral', width=1.6)
        c.line([(base + side * 7, 3), (base + side * 11, 7)], 'coral', width=1.6)
    c.ellipse(24, 21, 15, 12.5, 'body')
    c.ellipse(24, 26, 9, 5.5, 'belly', patch=True)
    c.circle(24, 11, 2.6, 'pearl')
    face(c, 24, 18, 10, 4, 5, expr, 26)
    return c.render()


def abissino(expr):
    c = Canvas(SIZE, SIZE, {'body': '#2f3f96', 'belly': '#6276d8', 'fin': '#1f2a6a', 'lure': '#c8ff74',
                            'stalk': '#4a5ab8'})
    c.poly([(8, 30), (0, 20), (4, 40)], 'fin', light=0.5)
    c.poly([(40, 30), (48, 20), (44, 40)], 'fin', light=0.5)
    feet(c, 24, 44, 8, 'fin', rx=5)
    c.ellipse(24, 36, 12, 9.5, 'body')
    c.ellipse(24, 38, 7, 6, 'belly', patch=True)
    c.poly([(24, 4), (19, 12), (29, 12)], 'fin', light=0.5)
    c.ellipse(24, 22, 16, 12.5, 'body')
    c.line([(28, 11), (33, 4), (38, 3)], 'stalk', width=1.5)
    c.circle(39, 4, 3, 'lure')
    face(c, 24, 18, 10, 4, 5, expr, 26, mouth='fang', iris='#ffffff', wild=False, blush=None,
         ink='#d8ff8a')
    if expr in ('idle', 'blink'):
        brow(c, 13, 15, 5, True, '#0c0a20')
        brow(c, 30, 15, 5, False, '#0c0a20')
    return c.render()


def silvante(expr):
    c = Canvas(SIZE, SIZE, {'body': '#82d672', 'belly': '#f0fbe0', 'antler': '#9a6838', 'leaf': '#3fb05a',
                            'flower': '#ff9ad2', 'ear': '#68b85a'})
    feet(c, 24, 44, 8, 'ear', rx=5)
    c.ellipse(24, 36, 11.5, 9, 'body')
    c.ellipse(24, 38, 6.5, 6, 'belly', patch=True)
    for side in (-1, 1):
        b = 24 + side * 7
        c.line([(b, 12), (b + side * 4, 5), (b + side * 9, 1)], 'antler', width=2)
        c.line([(b + side * 4, 6), (b + side * 1, 1)], 'antler', width=1.6)
        c.ellipse(b + side * 10, 3, 3, 1.8, 'leaf', angle=side * 20)
        c.circle(b + side * 1, 1.5, 1.8, 'flower')
    c.ellipse(5, 20, 5, 2.4, 'ear', angle=25)
    c.ellipse(43, 20, 5, 2.4, 'ear', angle=-25)
    c.ellipse(24, 21, 15, 12.5, 'body')
    c.ellipse(24, 26, 9, 5.5, 'belly', patch=True)
    for x, y in ((13, 12), (35, 12)):
        c.circle(x, y, 2.4, 'flower')
        c.px(x, y, '#fff3a0')
    face(c, 24, 18, 10, 4, 5, expr, 26)
    return c.render()


def rovaccio(expr):
    c = Canvas(SIZE, SIZE, {'body': '#5e7f3a', 'belly': '#9cbf62', 'thorn': '#3a4f22', 'moss': '#8fd14a',
                            'berry': '#e8364e'})
    feet(c, 24, 44, 8, 'thorn', rx=5)
    for i, (x, y, a) in enumerate(((7, 14, -40), (14, 7, -20), (24, 4, 0), (34, 7, 20), (41, 14, 40)
                                   )):
        c.flame(x, y + 6, 6, 9, 'thorn', angle=a)
    c.ellipse(24, 35, 12.5, 10, 'body')
    c.ellipse(24, 21, 16, 13, 'body')
    c.ellipse(24, 37, 7, 6, 'belly', patch=True)
    c.ellipse(15, 13, 5, 2.5, 'moss', patch=True)
    c.ellipse(33, 12, 4, 2, 'moss', patch=True)
    for x, y in ((10, 30), (38, 31), (31, 41)):
        c.circle(x, y, 1.8, 'berry')
    face(c, 24, 18, 10, 4, 5, expr, 26, mouth='grin', wild=True, blush=None, iris='#c6ff5a')
    return c.render()


FORMS = [None, lumino, favillo, spruzzo, fogliolo, pirolume, cenerotto, coralba, abissino, silvante, rovaccio]
