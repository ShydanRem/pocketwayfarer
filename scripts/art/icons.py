"""16x16 icons for items and the interface, drawn with the same shading engine."""
from canvas import Canvas

S = 16


def C(m):
    return Canvas(S, S, m)


def berry():
    c = C({'b': '#5aa0ff', 'leaf': '#4fc35a', 'glow': '#d8f4ff'})
    c.circle(6, 10, 4, 'b')
    c.circle(11, 9, 3.6, 'b')
    c.circle(8.5, 5.5, 3.3, 'b')
    c.ellipse(11, 3, 3, 1.5, 'leaf', angle=-30)
    c.px(5, 8, '#ffffff'); c.px(10, 7, '#ffffff'); c.px(7, 4, '#ffffff')
    return c


def bread():
    c = C({'crust': '#d98a3a', 'top': '#f4b860', 'honey': '#ffd23f'})
    c.ellipse(8, 9.5, 7, 5, 'crust')
    c.ellipse(8, 8, 6, 3.5, 'top', patch=True)
    c.ellipse(8, 5.5, 3.5, 1.6, 'honey', light=0.9)
    c.px(6, 9, '#a85a20'); c.px(9, 10, '#a85a20'); c.px(11, 8, '#a85a20')
    return c


def stew():
    c = C({'bowl': '#8a5a3a', 'soup': '#e8803a', 'bits': '#7fd36e', 'steam': '#ffffff'})
    c.ellipse(8, 9, 7, 5, 'bowl', clip=lambda xs, ys: ys >= 8)
    c.ellipse(8, 8.5, 6.4, 2.2, 'soup', light=0.8)
    c.px(6, 8, '#7fd36e'); c.px(9, 9, '#ffe066'); c.px(11, 8, '#7fd36e')
    for x in (5, 8, 11):
        c.px(x, 4, '#e8e8f8'); c.px(x + 1, 3, '#e8e8f8'); c.px(x, 2, '#e8e8f8')
    return c


def candy():
    c = C({'star': '#ffd23f', 'wrap': '#ff7ac8'})
    c.poly([(1, 8), (4, 6), (4, 10)], 'wrap', light=0.8)
    c.poly([(15, 8), (12, 6), (12, 10)], 'wrap', light=0.8)
    pts = []
    import math
    for i in range(10):
        r = 5.5 if i % 2 == 0 else 2.6
        a = math.radians(-90 + i * 36)
        pts.append((8 + r * math.cos(a), 8.4 + r * math.sin(a)))
    c.poly(pts, 'star', light=0.8)
    c.px(7, 6, '#ffffff')
    return c


def chili():
    c = C({'red': '#ff3a2a', 'stem': '#4fb34a'})
    c.line([(4, 5), (6, 10), (10, 13), (13, 13)], 'red', width=3.4)
    c.line([(4, 5), (5, 2), (7, 1)], 'stem', width=1.6)
    c.px(5, 6, '#ffb0a0'); c.px(7, 10, '#ffb0a0')
    return c


def kelp():
    c = C({'k': '#2fbf8f', 'k2': '#58e0a8'})
    c.line([(5, 15), (4, 11), (6, 7), (4, 3), (5, 1)], 'k', width=2.6)
    c.line([(10, 15), (11, 11), (9, 7), (11, 3)], 'k2', width=2.6)
    return c


def sprout():
    c = C({'soil': '#8a5a3a', 'stem': '#4fb34a', 'leaf': '#7fe36a'})
    c.ellipse(8, 13, 6, 2.5, 'soil')
    c.line([(8, 12), (8, 6)], 'stem', width=1.6)
    c.ellipse(5, 5, 3.5, 2, 'leaf', angle=-30)
    c.ellipse(11, 5, 3.5, 2, 'leaf', angle=30)
    return c


def potion(color='#ff4f7a'):
    c = C({'glass': '#d8e8ff', 'liq': color, 'cork': '#b07a4a'})
    c.rect(6, 1, 4, 3, 'cork', light=0.6)
    c.rect(6.5, 3.5, 3, 3, 'glass', light=0.85)
    c.circle(8, 10.5, 5, 'glass', light=0.85)
    c.ellipse(8, 11.5, 4, 3.4, 'liq', clip=lambda xs, ys: ys >= 9)
    c.px(6, 9, '#ffffff'); c.px(6, 10, '#ffffff')
    return c


def tonic():
    return potion('#5ad8ff')


def shard():
    c = C({'a': '#9ff3ff', 'b': '#5ac8ff', 'c': '#3a8aff'})
    c.poly([(8, 1), (13, 7), (8, 15), (3, 7)], 'b', light=0.6)
    c.poly([(8, 1), (13, 7), (8, 8)], 'a', light=0.95, grad=0)
    c.poly([(3, 7), (8, 8), (8, 15)], 'c', light=0.35, grad=0)
    c.px(7, 4, '#ffffff')
    return c


# ---- interface ---------------------------------------------------------
def heart():
    c = C({'h': '#ff4f7a'})
    c.circle(5, 6, 3.6, 'h'); c.circle(11, 6, 3.6, 'h')
    c.poly([(1.6, 7), (14.4, 7), (8, 14)], 'h', light=0.5)
    c.px(4, 4, '#ffffff'); c.px(4, 5, '#ffd0dc')
    return c


def bolt():
    c = C({'y': '#ffd23f'})
    c.poly([(10, 0), (3, 9), (7.5, 9), (5, 16), (13, 6), (8.5, 6)], 'y', light=0.8)
    return c


def food():
    c = C({'meat': '#e0703a', 'bone': '#f4ead8'})
    c.ellipse(7, 7, 5, 4.5, 'meat', angle=-40)
    c.line([(10, 10), (13, 13)], 'bone', width=2)
    c.circle(14, 12.5, 1.4, 'bone'); c.circle(12.5, 14, 1.4, 'bone')
    c.px(5, 5, '#ffc89a')
    return c


def smile():
    c = C({'f': '#ffd23f'})
    c.circle(8, 8, 6.5, 'f')
    c.pxs(5, 5, ['K....K', 'K....K', '......', 'K....K', '.KKKK.'], {'K': '#5a3a1a'})
    return c


def coin():
    c = C({'g': '#ffc83a', 'd': '#e09a20'})
    c.circle(8, 8, 6.5, 'g')
    c.circle(8, 8, 4.2, 'd', patch=True, light=0.45)
    c.pxs(7, 5, ['W', 'WW', 'W.', 'W', 'W', 'W'][:0] or ['.W.', 'WW.', '.W.', '.W.', 'WWW'], {'W': '#fff0b0'})
    return c


def sword():
    c = C({'blade': '#dfe8ff', 'hilt': '#8a5a3a', 'guard': '#ffc83a'})
    c.line([(3, 13), (13, 3)], 'blade', width=2.4, light=0.9)
    c.line([(3, 9), (7, 13)], 'guard', width=2)
    c.line([(1, 15), (4, 12)], 'hilt', width=2.2)
    return c


def burst():
    c = C({'o': '#ff8a3a', 'i': '#ffe066'})
    import math
    pts = []
    for i in range(16):
        r = 7.4 if i % 2 == 0 else 3.6
        a = math.radians(i * 22.5)
        pts.append((8 + r * math.cos(a), 8 + r * math.sin(a)))
    c.poly(pts, 'o', light=0.75)
    c.circle(8, 8, 2.6, 'i', light=1)
    return c


def shield():
    c = C({'s': '#5aa0ff', 'r': '#dfe8ff'})
    c.poly([(2, 2), (14, 2), (14, 8), (8, 15), (2, 8)], 's', light=0.7)
    c.poly([(5, 4.5), (11, 4.5), (11, 8), (8, 12), (5, 8)], 'r', light=0.85, patch=True)
    return c


def run():
    c = C({'w': '#dfe8ff'})
    c.line([(2, 12), (6, 8), (10, 8), (13, 4)], 'w', width=2)
    c.poly([(10, 2), (15, 3), (12, 7)], 'w', light=0.9)
    c.line([(3, 4), (7, 4)], 'w', width=1.2); c.line([(1, 7), (4, 7)], 'w', width=1.2)
    return c


def bag():
    c = C({'b': '#c8803a', 'flap': '#a85a28', 'buckle': '#ffd23f'})
    c.line([(5, 5), (6, 2), (10, 2), (11, 5)], 'flap', width=1.6)
    c.rect(2, 5, 12, 10, 'b', round_=3)
    c.rect(2, 5, 12, 5, 'flap', round_=2, light=0.55)
    c.rect(7, 8, 2, 3, 'buckle', light=0.9)
    return c


def map_():
    c = C({'p': '#f0dca8', 'p2': '#d8b878', 'x': '#ff4f4f'})
    c.poly([(1, 3), (5, 1), (10, 3), (15, 1), (15, 13), (10, 15), (5, 13), (1, 15)], 'p', light=0.8)
    c.poly([(5, 1), (10, 3), (10, 15), (5, 13)], 'p2', light=0.6, patch=True)
    for i in range(3):
        c.px(3 + i * 2, 10 - i, '#8a6a3a')
    c.pxs(10, 5, ['X.X', '.X.', 'X.X'], {'X': '#ff4f4f'})
    return c


def link():
    c = C({'a': '#5ad8ff', 'b': '#ff8ac8'})
    for r, m in ((6.5, 'a'), (4.2, 'b')):
        pass
    c.pxs(1, 2, ['..AAAAAAAAAA..', '.A..........A.', 'A...BBBBBB...A', '...B......B...',
                 '..B..AAAA..B..', '....A....A....', '.......WW.....', '......WWWW....',
                 '......WWWW....', '.......WW.....'], {'A': '#5ad8ff', 'B': '#8ff0c8', 'W': '#ffffff'})
    return c


def moon():
    c = C({'m': '#ffe9a0'})
    c.circle(8, 8, 6.5, 'm')
    c.clear(lambda xs, ys: (xs - 11.5) ** 2 + (ys - 5.5) ** 2 < 30)
    c.px(13, 3, '#ffffff'); c.px(14, 9, '#ffffff')
    return c


def sun():
    c = C({'s': '#ffc83a', 'r': '#ffe066'})
    import math
    for i in range(8):
        a = math.radians(i * 45)
        c.line([(8 + 5 * math.cos(a), 8 + 5 * math.sin(a)), (8 + 7.4 * math.cos(a), 8 + 7.4 * math.sin(a))], 'r', width=1.6)
    c.circle(8, 8, 4.2, 's')
    return c


def broom():
    c = C({'h': '#a8703a', 'b': '#ffd88a'})
    c.line([(13, 1), (7, 9)], 'h', width=1.8)
    c.poly([(5, 7), (9, 10), (6, 15), (1, 13)], 'b', light=0.75)
    c.px(3, 12, '#c8a050'); c.px(5, 13, '#c8a050')
    return c


def ball():
    c = C({'r': '#ff5a5a', 'w': '#ffffff', 'y': '#ffd23f'})
    c.circle(8, 8, 6.5, 'r')
    c.ellipse(8, 8, 6.5, 2.2, 'w', patch=True, light=0.95)
    c.ellipse(8, 8, 2.2, 6.5, 'y', patch=True, light=0.9)
    return c


def dumbbell():
    c = C({'w': '#7a7a9a', 'bar': '#c8c8e0'})
    c.line([(4, 8), (12, 8)], 'bar', width=2)
    c.rect(1, 3, 4, 10, 'w', round_=1)
    c.rect(11, 3, 4, 10, 'w', round_=1)
    return c


def gift():
    c = C({'box': '#ff5a8a', 'rib': '#ffe066'})
    c.rect(2, 7, 12, 8, 'box', light=0.6)
    c.rect(1, 5, 14, 3, 'box', light=0.8)
    c.rect(7, 5, 2, 10, 'rib', light=0.9)
    c.ellipse(5.5, 3.5, 2.6, 1.6, 'rib', angle=20)
    c.ellipse(10.5, 3.5, 2.6, 1.6, 'rib', angle=-20)
    return c


def trophy():
    c = C({'g': '#ffc83a'})
    c.ellipse(8, 5, 5.5, 5, 'g', clip=lambda xs, ys: ys >= 1)
    c.clear(lambda xs, ys: ys < 2)
    c.rect(7, 9, 2, 3, 'g', light=0.6)
    c.rect(4, 12, 8, 3, 'g', light=0.5)
    c.circle(2.5, 5, 2, 'g', light=0.5); c.clear(lambda xs, ys: (xs - 2.5) ** 2 + (ys - 5) ** 2 < 1.1)
    c.circle(13.5, 5, 2, 'g', light=0.5); c.clear(lambda xs, ys: (xs - 13.5) ** 2 + (ys - 5) ** 2 < 1.1)
    return c


def scroll():
    c = C({'p': '#f4e2b0', 'r': '#c88a4a'})
    c.rect(3, 2, 10, 12, 'p', light=0.8)
    c.ellipse(8, 2, 6, 1.6, 'r'); c.ellipse(8, 14, 6, 1.6, 'r')
    for y in (5, 8, 11):
        for x in range(5, 11):
            c.px(x, y, '#a88a5a')
    return c


def back():
    c = C({'w': '#ffffff'})
    c.poly([(1, 8), (7, 2), (7, 5), (14, 5), (14, 11), (7, 11), (7, 14)], 'w', light=0.9)
    return c


def egg():
    c = C({'e': '#fff0d2', 's': '#73e0d0'})
    c.ellipse(8, 8.5, 5.5, 7, 'e')
    c.circle(6, 7, 1.5, 's', patch=True); c.circle(10, 11, 1.8, 's', patch=True)
    return c


def sparkle():
    c = C({'w': '#ffffff'})
    c.pxs(4, 1, ['...W...', '...W...', '..WWW..', 'WWWWWWW', '..WWW..', '...W...', '...W...'], {'W': '#fff6c0'})
    c.pxs(1, 10, ['.W.', 'WWW', '.W.'], {'W': '#ffffff'})
    return c


def zzz():
    c = C({'z': '#c8d8ff'})
    c.pxs(1, 1, ['......ZZZZ', '........Z.', '.......Z..', '......ZZZZ', 'ZZZZZ.....',
                 '...Z......', '..Z.......', '.Z........', 'ZZZZZ.....'], {'Z': '#e0e8ff'})
    return c


def star():
    import math
    c = C({'s': '#ffd23f'})
    pts = []
    for i in range(10):
        r = 7.4 if i % 2 == 0 else 3.4
        a = math.radians(-90 + i * 36)
        pts.append((8 + r * math.cos(a), 8.6 + r * math.sin(a)))
    c.poly(pts, 's', light=0.8)
    c.px(7, 6, '#ffffff'); c.px(7, 7, '#fff6c0')
    return c


def storm():
    c = C({'c': '#6a6a8a', 'y': '#ffe066'})
    c.circle(5, 7, 3.6, 'c'); c.circle(10, 6, 4.4, 'c'); c.circle(12.5, 9, 3, 'c')
    c.ellipse(8, 9.5, 6.5, 2.5, 'c')
    c.poly([(9, 10), (6, 14), (8, 14), (7, 16), (11, 12), (9, 12)], 'y', light=0.9)
    return c


def mess():
    c = C({'d': '#a8885a', 'd2': '#8a6a4a', 'leaf': '#7aa84a'})
    c.ellipse(8, 12, 6.5, 3.2, 'd')
    c.ellipse(6, 10, 3.5, 2.6, 'd2')
    c.ellipse(10.5, 9.5, 3, 2.4, 'd')
    c.ellipse(11, 12, 2.4, 1.4, 'leaf', angle=-20)
    c.pxs(3, 2, ['.#...#', '#...#.', '.#...#'], {'#': '#c8b89a'})
    return c


ITEMS = [berry, bread, stew, candy, chili, kelp, sprout, potion, tonic, shard]
UI = [('heart', heart), ('bolt', bolt), ('food', food), ('smile', smile), ('coin', coin), ('sword', sword),
      ('burst', burst), ('shield', shield), ('run', run), ('bag', bag), ('map', map_), ('link', link),
      ('moon', moon), ('sun', sun), ('broom', broom), ('ball', ball), ('dumbbell', dumbbell),
      ('gift', gift), ('trophy', trophy), ('scroll', scroll), ('back', back), ('egg', egg),
      ('sparkle', sparkle), ('zzz', zzz), ('star', star), ('storm', storm), ('mess', mess)]
