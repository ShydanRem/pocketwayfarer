"""Wild creatures. Each species = archetype + palette; bosses use a larger grid."""
from canvas import Canvas, eye, brow

INK = '#1d1430'
FRAMES = ['idle', 'attack']


def _eyes(c, cx, ey, gap, w, h, frame, iris=None, ink=INK, angry_always=False):
    kind = 'angry' if frame == 'attack' or angry_always else 'open'
    lx = int(cx - gap / 2 - w + 0.5)
    rx = int(cx + gap / 2 + 0.5)
    eye(c, lx, ey, w, h, kind, iris=iris, ink=ink)
    eye(c, rx, ey, w, h, kind, iris=iris, ink=ink, mirror=True)
    if kind == 'angry':
        brow(c, lx - 1, ey - 1, w + 1, True, ink)
        brow(c, rx, ey - 1, w + 1, False, ink)


def _mouth(c, cx, y, frame, wide=5, ink=INK):
    if frame == 'attack':
        rows = ['.' + 'K' * (wide - 2) + '.', 'K' + 'R' * (wide - 2) + 'K', '.' + 'K' * (wide - 2) + '.']
    else:
        rows = ['K' + '.' * (wide - 2) + 'K', '.' + 'K' * (wide - 2) + '.']
    c.pxs(int(cx - wide / 2 + 0.5), y, rows, {'K': ink, 'R': '#e8506a'})


def slime(frame, s, color, crown=False):
    c = Canvas(s, s, {'body': color, 'shine': '#ffffff', 'gold': '#ffcc33', 'gem': '#ff4f7a'})
    k = s / 44
    floor = lambda xs, ys: ys <= 40 * k
    c.ellipse(22 * k, 30 * k, 18 * k, 14 * k, 'body', clip=floor, bulge=0.8)
    c.ellipse(15 * k, 22 * k, 3.5 * k, 2.2 * k, 'shine', angle=-30, light=1.0)
    if crown:
        cx = 22 * k
        c.poly([(cx - 9 * k, 19 * k), (cx - 10 * k, 8 * k), (cx - 5 * k, 13 * k), (cx, 5 * k),
                (cx + 5 * k, 13 * k), (cx + 10 * k, 8 * k), (cx + 9 * k, 19 * k)], 'gold', light=0.75)
        c.circle(cx, 14 * k, 1.8 * k, 'gem')
    _eyes(c, 22 * k, int(26 * k), 8 * k, 3, 4, frame)
    _mouth(c, 22 * k, int(32 * k), frame)
    return c.render()


def shroom(frame, s, cap, stem='#f4e4c8'):
    c = Canvas(s, s, {'cap': cap, 'stem': stem, 'spot': '#fff8ee', 'foot': '#c9a87a'})
    c.ellipse(15, 40, 4, 2.5, 'foot')
    c.ellipse(29, 40, 4, 2.5, 'foot')
    c.ellipse(22, 31, 9, 9.5, 'stem')
    c.ellipse(22, 16, 19, 11, 'cap', clip=lambda xs, ys: ys <= 21)
    for x, y, r in ((13, 12, 2.8), (24, 9, 2.2), (31, 14, 2.5), (19, 17, 1.6)):
        c.ellipse(x, y, r, r * 0.8, 'spot', patch=True, light=0.95)
    _eyes(c, 22, 27, 6, 2, 3, frame)
    _mouth(c, 22, 33, frame, 4)
    return c.render()


def wisp(frame, s, outer, inner, boss=False):
    c = Canvas(s, s, {'outer': outer, 'inner': inner, 'spark': '#ffffff'})
    k = s / 44
    c.flame(22 * k, 40 * k, 30 * k, 40 * k, 'outer')
    c.flame(22 * k, 39 * k, 17 * k, 27 * k, 'inner')
    for x, y in ((8, 12), (36, 10), (34, 30)):
        c.px(int(x * k), int(y * k), '#ffffff')
    _eyes(c, 22 * k, int(27 * k), 8 * k, 3, 4, frame)
    _mouth(c, 22 * k, int(33 * k), frame, 4)
    return c.render()


def crab(frame, s, color, boss=False):
    k = s / 44
    c = Canvas(s, s, {'shell': color, 'claw': color, 'leg': color, 'belly': '#ffe2cc', 'stalk': color,
                      'plate': '#c8d0e8'})
    for i in range(3):
        c.line([(14 * k - i * 3 * k, 32 * k), (6 * k - i * 3 * k, 40 * k)], 'leg', width=2 * k)
        c.line([(30 * k + i * 3 * k, 32 * k), (38 * k + i * 3 * k, 40 * k)], 'leg', width=2 * k)
    rise = -3 if frame == 'attack' else 0
    for side in (-1, 1):
        cx = 22 * k + side * 15 * k
        c.line([(22 * k + side * 8 * k, 26 * k), (cx, 17 * k + rise)], 'leg', width=2.5 * k)
        c.circle(cx, 13 * k + rise, 6 * k, 'claw')
        c.clear(lambda xs, ys, cx=cx: (abs(xs - cx) < 1.6 * k) & (ys < 12 * k + rise) & (ys > 5 * k + rise))
    c.line([(18 * k, 22 * k), (17 * k, 14 * k)], 'stalk', width=1.6 * k)
    c.line([(26 * k, 22 * k), (27 * k, 14 * k)], 'stalk', width=1.6 * k)
    c.ellipse(22 * k, 29 * k, 15 * k, 9 * k, 'shell')
    if boss:
        c.ellipse(22 * k, 25 * k, 11 * k, 5 * k, 'plate', patch=True, light=0.8)
    c.ellipse(22 * k, 34 * k, 8 * k, 3 * k, 'belly', patch=True)
    eye(c, int(15 * k), int(10 * k), 3, 4, 'angry' if frame == 'attack' else 'open')
    eye(c, int(26 * k), int(10 * k), 3, 4, 'angry' if frame == 'attack' else 'open', mirror=True)
    _mouth(c, 22 * k, int(30 * k), frame, 4)
    return c.render()


def jelly(frame, s, color):
    c = Canvas(s, s, {'bell': color, 'tent': color, 'shine': '#ffffff', 'inner': '#ffffff'})
    for i, x in enumerate((12, 17, 22, 27, 32)):
        wob = 2 if frame == 'attack' else 1
        pts = [(x, 20), (x - wob, 26), (x + wob, 32), (x - wob, 38), (x, 42)]
        c.line(pts, 'tent', width=2)
    c.ellipse(22, 18, 15, 13, 'bell', clip=lambda xs, ys: ys <= 23, bulge=0.7)
    c.ellipse(15, 11, 3, 2, 'shine', angle=-30, light=1)
    _eyes(c, 22, 15, 7, 3, 4, frame)
    _mouth(c, 22, 20, frame, 4)
    return c.render()


def golem(frame, s, rock, glow, accent=None, leaves=False, boss=False):
    k = s / 44
    c = Canvas(s, s, {'rock': rock, 'dark': rock, 'glow': glow, 'leaf': '#4fbf5a', 'moss': '#7fcf4a'})
    c.rect(4 * k, 20 * k, 8 * k, 16 * k, 'rock', round_=3 * k, light=0.5)
    c.rect(32 * k, 20 * k, 8 * k, 16 * k, 'rock', round_=3 * k, light=0.5)
    c.rect(12 * k, 37 * k, 7 * k, 6 * k, 'rock', round_=2 * k, light=0.45)
    c.rect(25 * k, 37 * k, 7 * k, 6 * k, 'rock', round_=2 * k, light=0.45)
    c.rect(10 * k, 17 * k, 24 * k, 22 * k, 'rock', round_=5 * k)
    c.rect(13 * k, 5 * k, 18 * k, 14 * k, 'rock', round_=4 * k, light=0.7)
    if leaves:
        c.ellipse(17 * k, 4 * k, 5 * k, 2.4 * k, 'leaf', angle=-20)
        c.ellipse(27 * k, 4 * k, 5 * k, 2.4 * k, 'leaf', angle=20)
        c.ellipse(16 * k, 26 * k, 4 * k, 2 * k, 'moss', patch=True)
    if accent:
        for pts in ([(14, 22), (17, 27), (15, 32)], [(29, 21), (27, 26), (30, 31)], [(20, 33), (24, 35)]):
            for (x1, y1), (x2, y2) in zip(pts, pts[1:]):
                n = max(abs(x2 - x1), abs(y2 - y1))
                for i in range(n + 1):
                    c.px(int((x1 + (x2 - x1) * i / n) * k), int((y1 + (y2 - y1) * i / n) * k), accent)
    w = 4 if frame == 'attack' else 3
    for x in (17, 25):
        for j in range(2):
            for i in range(w):
                c.px(int(x * k) + i - (1 if frame == 'attack' and x == 17 else 0), int(11 * k) + j, glow)
    return c.render()


def bat(frame, s, body, wing, ear=None, flame=False):
    c = Canvas(s, s, {'body': body, 'wing': wing, 'belly': '#ffd8e8', 'flame': '#ff5a2a', 'core': '#ffd23f'})
    up = -4 if frame == 'attack' else 0
    for side in (-1, 1):
        pts = [(22 + side * 6, 18), (22 + side * 21, 8 + up), (22 + side * 20, 22 + up),
               (22 + side * 15, 19 + up), (22 + side * 12, 25), (22 + side * 8, 22)]
        c.poly(pts, 'wing', light=0.5)
    if flame:
        c.flame(22, 12, 8, 10, 'flame')
        c.flame(22, 12, 4, 6, 'core')
    c.poly([(15, 14), (14, 5), (20, 11)], 'body')
    c.poly([(29, 14), (30, 5), (24, 11)], 'body')
    c.ellipse(22, 20, 9, 9, 'body')
    c.ellipse(22, 24, 5, 4, 'belly', patch=True)
    c.line([(18, 29), (17, 33)], 'body', width=1.5)
    c.line([(26, 29), (27, 33)], 'body', width=1.5)
    _eyes(c, 22, 16, 5, 2, 3, frame)
    c.pxs(19, 22, ['W....W' if frame == 'idle' else 'KWKKWK'], {'W': '#ffffff', 'K': INK})
    return c.render()


def owl(frame, s, body, disc, moon=False):
    c = Canvas(s, s, {'body': body, 'wing': body, 'disc': disc, 'beak': '#ffb03a', 'feet': '#ffb03a',
                      'moon': '#fff4b0', 'belly': disc})
    c.ellipse(13, 41, 3.5, 2, 'feet')
    c.ellipse(31, 41, 3.5, 2, 'feet')
    c.poly([(9, 12), (11, 2), (16, 9)], 'body')
    c.poly([(35, 12), (33, 2), (28, 9)], 'body')
    c.ellipse(22, 25, 15, 16, 'body')
    c.ellipse(9, 28, 5, 10, 'wing', angle=10, light=0.45)
    c.ellipse(35, 28, 5, 10, 'wing', angle=-10, light=0.45)
    c.ellipse(22, 33, 9, 7, 'belly', patch=True)
    c.circle(16, 19, 6, 'disc', light=0.85)
    c.circle(28, 19, 6, 'disc', light=0.85)
    if moon:
        c.circle(22, 9, 2.6, 'moon', light=1)
    for cx in (16, 28):
        c.pxs(cx - 2, 17, ['.KKK.', 'KYYYK', 'KYKYK', 'KYYYK', '.KKK.'] if frame == 'idle'
              else ['KKKKK', 'KYYYK', 'KYKYK', '.KKK.', '.....'], {'K': INK, 'Y': '#ffd23f'})
        c.px(cx - 1, 18, '#ffffff')
    c.poly([(20, 24), (24, 24), (22, 28)], 'beak', light=0.8)
    return c.render()


def lizard(frame, s, body, spots):
    c = Canvas(s, s, {'body': body, 'belly': '#ffe0a0', 'spot': spots, 'tail': body})
    c.line([(30, 34), (37, 32), (40, 26), (37, 21)], 'tail', width=4)
    c.ellipse(9, 38, 4, 2.5, 'body')
    c.ellipse(35, 39, 4, 2.5, 'body')
    c.ellipse(22, 31, 12, 9, 'body')
    c.ellipse(22, 34, 7, 4.5, 'belly', patch=True)
    c.ellipse(22, 16, 12, 10, 'body')
    for x, y in ((15, 27), (29, 28), (22, 25), (16, 11), (28, 11)):
        c.circle(x, y, 1.6, 'spot', patch=True, light=0.9)
    _eyes(c, 22, 13, 9, 3, 4, frame, iris='#ffd23f')
    _mouth(c, 22, 20, frame, 6)
    return c.render()


def dragon(frame, s, body, belly, wing):
    k = s / 44
    c = Canvas(s, s, {'body': body, 'belly': belly, 'wing': wing, 'horn': '#f0e6d8', 'fire': '#ff6a2a',
                      'core': '#ffe066'})
    up = -3 if frame == 'attack' else 0
    for side in (-1, 1):
        pts = [(22 * k + side * 7 * k, 20 * k), (22 * k + side * 21 * k, 4 * k + up), (22 * k + side * 20 * k, 26 * k),
               (22 * k + side * 14 * k, 22 * k), (22 * k + side * 10 * k, 30 * k)]
        c.poly(pts, 'wing', light=0.45)
    c.line([(30 * k, 38 * k), (38 * k, 40 * k), (42 * k, 34 * k)], 'body', width=3.5 * k)
    c.ellipse(14 * k, 41 * k, 4 * k, 2.5 * k, 'body')
    c.ellipse(30 * k, 41 * k, 4 * k, 2.5 * k, 'body')
    c.ellipse(22 * k, 32 * k, 11 * k, 10 * k, 'body')
    c.ellipse(22 * k, 34 * k, 6 * k, 7 * k, 'belly', patch=True)
    for i in range(4):
        y = int((28 + i * 3) * k)
        for x in range(int(18 * k), int(26 * k)):
            if (x + i) % 3 == 0:
                c.px(x, y, '#c85a2a')
    c.poly([(13 * k, 10 * k), (9 * k, 0), (17 * k, 7 * k)], 'horn', light=0.85)
    c.poly([(31 * k, 10 * k), (35 * k, 0), (27 * k, 7 * k)], 'horn', light=0.85)
    c.ellipse(22 * k, 15 * k, 11 * k, 9 * k, 'body')
    _eyes(c, 22 * k, int(11 * k), 8 * k, 4, 5, frame, iris='#ffd23f', angry_always=True)
    if frame == 'attack':
        c.flame(22 * k, 28 * k, 6 * k, 6 * k, 'fire', angle=180)
        c.flame(22 * k, 27 * k, 3 * k, 4 * k, 'core', angle=180)
    else:
        c.pxs(int(19 * k), int(19 * k), ['W.....W'], {'W': '#ffffff'})
    return c.render()


def crystal(frame, s, color, halo=False):
    k = s / 44
    c = Canvas(s, s, {'a': color, 'b': color, 'c': color, 'd': color, 'core': '#ffffff', 'halo': '#ffe27a'})
    if halo:
        c.ellipse(22 * k, 22 * k, 20 * k, 6 * k, 'halo', light=0.9)
        c.ellipse(22 * k, 22 * k, 17 * k, 3.6 * k, 'halo', light=0.9)
        c.clear(lambda xs, ys: ((xs - 22 * k) / (17 * k)) ** 2 + ((ys - 22 * k) / (3.6 * k)) ** 2 < 1)
    top, left, right, bot, mid = (22 * k, 2 * k), (6 * k, 18 * k), (38 * k, 18 * k), (22 * k, 42 * k), (22 * k, 20 * k)
    c.poly([top, left, mid], 'a', light=0.95, grad=0)
    c.poly([top, mid, right], 'b', light=0.7, grad=0)
    c.poly([left, bot, mid], 'c', light=0.5, grad=0)
    c.poly([mid, bot, right], 'd', light=0.3, grad=0)
    c.circle(22 * k, 20 * k, 4 * k, 'core', light=0.95)
    pupil = '#2a1050' if frame == 'idle' else '#ff2a5a'
    c.pxs(int(22 * k) - 1, int(20 * k) - 2, ['.K.', 'KKK', 'KKK', '.K.'], {'K': pupil})
    return c.render(outline_color='#2a1a40')


def bird(frame, s, body, wing):
    c = Canvas(s, s, {'body': body, 'wing': wing, 'beak': '#ffb33a', 'crest': '#ff7a5a', 'belly': '#fff3c8'})
    up = -5 if frame == 'attack' else 0
    c.ellipse(7, 25 + up, 7, 4, 'wing', angle=-35 if frame == 'attack' else -15, light=0.5)
    c.ellipse(37, 25 + up, 7, 4, 'wing', angle=35 if frame == 'attack' else 15, light=0.5)
    for x, a in ((18, -25), (22, 0), (26, 25)):
        c.flame(x, 12, 4, 9, 'crest', angle=a)
    c.ellipse(15, 41, 3, 2, 'beak')
    c.ellipse(29, 41, 3, 2, 'beak')
    c.ellipse(22, 27, 13, 13, 'body')
    c.ellipse(22, 32, 8, 7, 'belly', patch=True)
    _eyes(c, 22, 21, 8, 3, 4, frame)
    c.poly([(19, 27), (25, 27), (22, 32)], 'beak', light=0.8)
    return c.render()


# species id -> (painter, boss)
def species_frames():
    S, B = 44, 60
    table = [
        ('gelatino', lambda f: slime(f, S, '#7ee06d')),
        ('fungotto', lambda f: shroom(f, S, '#e85050')),
        ('scintillo', lambda f: wisp(f, S, '#ff8a3a', '#ffe066')),
        ('re_gelatino', lambda f: slime(f, B, '#5fd06a', crown=True)),
        ('granchietto', lambda f: crab(f, S, '#ff6f5e')),
        ('medusina', lambda f: jelly(f, S, '#d08cff')),
        ('gelatino_blu', lambda f: slime(f, S, '#58a8ff')),
        ('granchio_re', lambda f: crab(f, B, '#e8484a', boss=True)),
        ('radicotto', lambda f: golem(f, S, '#8b6a3e', '#9dff7a', leaves=True)),
        ('pipistrello', lambda f: bat(f, S, '#7a5cc4', '#4b3590')),
        ('gufo', lambda f: owl(f, S, '#6f7fb5', '#c8d4f0', moon=True)),
        ('antico_ceppo', lambda f: golem(f, B, '#6e5232', '#c8ff5a', leaves=True, boss=True)),
        ('salamandra', lambda f: lizard(f, S, '#ff6a3d', '#ffd23f')),
        ('golem', lambda f: golem(f, S, '#5b4a58', '#ffcf3f', accent='#ff7a1a')),
        ('bracepippo', lambda f: bat(f, S, '#4a3040', '#c83a2a', flame=True)),
        ('drago', lambda f: dragon(f, B, '#6c6478', '#ff8a45', '#a83a3a')),
        ('tempestino', lambda f: wisp(f, S, '#6ab8ff', '#e8f8ff')),
        ('cristallo', lambda f: crystal(f, S, '#b48cff')),
        ('grifoncino', lambda f: bird(f, S, '#c8b04a', '#8a7a3a')),
        ('custode', lambda f: crystal(f, B, '#fff0b0', halo=True)),
    ]
    return table
