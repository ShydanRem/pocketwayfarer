"""16x16 icons for the RPG: items (type x tier), consumables, rooms, skills, UI."""
import math

from canvas import Canvas
import icons as P

METAL = ['#c88a4a', '#9aa0b0', '#c8d0e0', '#8ae0f0', '#f4e0a0']
WOOD = ['#a8703a', '#c89a5a', '#7a3a2a', '#3a3040', '#c8d0ff']
LEATHER = ['#a8703a', '#8a5a3a', '#4a8a5a', '#c84a3a', '#3a3a4a']
CLOTH = ['#d8d0c0', '#c8a0e8', '#5a5ad8', '#3a2a8a', '#e8f0ff']
GEM = ['#c8803a', '#d8d8e8', '#ffd23f', '#e8f0ff', '#fff0a0']
GLOW = ['#7ae8ff', '#8aff9a', '#ff8ac8', '#c88aff', '#ffe066']


def C(m):
    return Canvas(16, 16, m)


def sword(t):
    c = C({'m': METAL[t], 'g': '#ffd23f', 'h': '#5a3a2a'})
    c.line([(3, 13), (13, 3)], 'm', width=2.4, light=0.9)
    c.line([(3, 9), (7, 13)], 'g', width=1.8); c.line([(1, 15), (4, 12)], 'h', width=2)
    return c


def axe(t):
    c = C({'m': METAL[t], 'w': '#8a5a3a'})
    c.line([(3, 15), (10, 2)], 'w', width=1.8)
    c.ellipse(11, 5, 4.5, 4, 'm', angle=30)
    return c


def staff(t):
    c = C({'w': WOOD[t], 'g': GLOW[t]})
    c.line([(4, 15), (11, 4)], 'w', width=1.8)
    c.circle(12, 3.5, 3, 'g', light=0.95)
    return c


def bow(t):
    c = C({'w': WOOD[t], 's': '#e8e8e8'})
    c.ellipse(7, 8, 5, 7.5, 'w', clip=lambda xs, ys: xs < 7.5)
    c.clear(lambda xs, ys: ((xs - 7) / 3.2) ** 2 + ((ys - 8) / 6) ** 2 < 1)
    c.line([(7, 1), (7, 15)], 's', width=1, light=0.95)
    return c


def dagger(t):
    c = C({'m': METAL[t], 'g': '#ffd23f', 'h': '#3a2a3a'})
    c.line([(4, 12), (11, 5)], 'm', width=2.2, light=0.9)
    c.line([(3, 9), (7, 13)], 'g', width=1.5); c.line([(2, 14), (4, 12)], 'h', width=1.8)
    return c


def mace(t):
    c = C({'m': METAL[t], 'h': '#6a4a2a'})
    c.line([(3, 15), (9, 6)], 'h', width=1.8)
    c.circle(10.5, 5, 3.6, 'm')
    for a in range(0, 360, 60):
        c.px(10.5 + 4.6 * math.cos(math.radians(a)), 5 + 4.6 * math.sin(math.radians(a)), METAL[t])
    return c


def knuckle(t):
    c = C({'m': METAL[t], 's': '#c89a7a'})
    c.rect(3, 6, 10, 7, 's', round_=2, light=0.7)
    for x in (4, 7, 10):
        c.circle(x + 1, 6, 1.6, 'm', light=0.9)
    return c


def hammer(t):
    c = C({'m': METAL[t], 'w': '#8a5a3a'})
    c.line([(4, 15), (9, 5)], 'w', width=1.8)
    c.rect(6, 1, 9, 6, 'm', round_=1)
    return c


def scythe(t):
    c = C({'m': METAL[t], 'w': '#3a2a3a'})
    c.line([(4, 15), (8, 2)], 'w', width=1.6)
    c.ellipse(11, 4, 5.5, 2.6, 'm', angle=20, clip=lambda xs, ys: ys < 5)
    return c


def shield(t):
    c = C({'m': METAL[t], 'f': '#3a5ab8'})
    c.poly([(2, 2), (14, 2), (14, 8), (8, 15), (2, 8)], 'f', light=0.6)
    c.poly([(5, 4.5), (11, 4.5), (11, 8), (8, 12), (5, 8)], 'm', light=0.85, patch=True)
    return c


def quiver(t):
    c = C({'l': LEATHER[t], 'a': '#e8e8e8', 'f': '#e84a4a'})
    c.rect(4, 5, 6, 10, 'l', round_=1, light=0.6)
    for x in (5, 7, 9):
        c.line([(x, 5), (x + 3, 1)], 'a', width=1)
        c.px(x + 3, 1, '#e84a4a')
    return c


def tome(t):
    c = C({'b': CLOTH[t], 'p': '#f4e8c8', 'g': '#ffd23f'})
    c.rect(3, 2, 10, 12, 'b', round_=1, light=0.6)
    c.rect(4, 3, 2, 10, 'p', light=0.9)
    c.circle(9.5, 8, 2, 'g', light=0.9)
    return c


def charm(t):
    c = C({'g': GLOW[t], 'c': GEM[t]})
    c.line([(5, 2), (8, 6), (11, 2)], 'c', width=1)
    c.circle(8, 10, 4, 'g', light=0.9)
    return c


def helm(t):
    c = C({'m': METAL[t], 'p': '#c83a3a'})
    c.ellipse(8, 9, 6.5, 6, 'm', clip=lambda xs, ys: ys < 12)
    c.rect(2, 10, 12, 3, 'm', light=0.6)
    c.flame(8, 4, 3, 5, 'p')
    return c


def hood(t):
    c = C({'l': LEATHER[t]})
    c.ellipse(8, 9, 6.5, 7, 'l')
    c.clear(lambda xs, ys: ((xs - 8) / 3.5) ** 2 + ((ys - 11) / 4) ** 2 < 1)
    return c


def circlet(t):
    c = C({'c': GEM[t], 'g': GLOW[t]})
    c.ellipse(8, 9, 6.5, 3, 'c', light=0.85)
    c.clear(lambda xs, ys: ((xs - 8) / 5) ** 2 + ((ys - 9) / 1.6) ** 2 < 1)
    c.circle(8, 6.5, 1.8, 'g', light=0.95)
    return c


def plate(t):
    c = C({'m': METAL[t], 'd': '#5a5a6a'})
    c.poly([(3, 3), (13, 3), (14, 8), (12, 15), (4, 15), (2, 8)], 'm', light=0.7)
    c.line([(8, 4), (8, 14)], 'd', width=1, light=0.5)
    return c


def leather(t):
    c = C({'l': LEATHER[t], 'b': '#3a2a2a'})
    c.poly([(3, 3), (13, 3), (13, 15), (3, 15)], 'l', light=0.65)
    c.clear(lambda xs, ys: (abs(xs - 8) < 2) & (ys < 6))
    c.rect(3, 10, 10, 1.5, 'b', light=0.4)
    return c


def robe(t):
    c = C({'r': CLOTH[t], 't': '#ffd23f'})
    c.poly([(5, 2), (11, 2), (14, 15), (2, 15)], 'r', light=0.65)
    c.line([(8, 3), (8, 15)], 't', width=1)
    return c


def boots(t):
    c = C({'l': LEATHER[t]})
    c.rect(3, 3, 4, 9, 'l', light=0.6); c.rect(3, 10, 6, 4, 'l', round_=1)
    c.rect(9, 4, 4, 8, 'l', light=0.5); c.rect(9, 11, 6, 3.5, 'l', round_=1, light=0.5)
    return c


def ring(t):
    c = C({'c': GEM[t], 'g': GLOW[t]})
    c.circle(8, 10, 5, 'c')
    c.clear(lambda xs, ys: (xs - 8) ** 2 + (ys - 10) ** 2 < 8)
    c.circle(8, 4.5, 2.5, 'g', light=0.95)
    return c


def amulet(t):
    c = C({'c': GEM[t], 'g': GLOW[t]})
    c.line([(3, 1), (8, 8), (13, 1)], 'c', width=1)
    c.poly([(8, 7), (12, 11), (8, 15), (4, 11)], 'g', light=0.85)
    return c


ITEM_TYPES = [sword, axe, staff, bow, dagger, mace, knuckle, hammer, scythe, shield, quiver, tome, charm,
              helm, hood, circlet, plate, leather, robe, boots, ring, amulet]


# ---- consumables ------------------------------------------------------------
def potion(color):
    c = C({'gl': '#d8e8ff', 'l': color, 'k': '#b07a4a'})
    c.rect(6, 1, 4, 3, 'k', light=0.6)
    c.circle(8, 10, 5, 'gl', light=0.85)
    c.ellipse(8, 11, 4, 3.4, 'l', clip=lambda xs, ys: ys >= 9)
    c.px(6, 9, '#ffffff')
    return c


def antidote():
    c = potion('#7ae86a')
    c.pxs(7, 9, ['.W.', 'WWW', '.W.'], {'W': '#ffffff'})
    return c


def pie():
    c = C({'c': '#e8a85a', 'f': '#c84a3a'})
    c.ellipse(8, 10, 7, 4.5, 'c')
    for x in (4, 8, 12):
        c.px(x, 9, '#c84a3a'); c.px(x - 1, 10, '#8a3a2a')
    return c


def ore():
    c = C({'r': '#7a7a8a', 'o': '#ffc83a'})
    c.poly([(2, 12), (5, 4), (11, 3), (14, 9), (11, 14), (4, 15)], 'r', light=0.6)
    for x, y in ((6, 7), (10, 9), (8, 12)):
        c.px(x, y, '#ffe07a'); c.px(x + 1, y, '#ffc83a')
    return c


def scroll_return():
    c = P.scroll()
    c.pxs(6, 6, ['.B.', 'BBB', '.B.'], {'B': '#5aa0ff'})
    return c


CONSUMABLES = [lambda: potion('#ff4f7a'), lambda: potion('#ff2a8a'), lambda: potion('#5ad8ff'), antidote,
               P.bread, P.stew, pie, ore, scroll_return]


# ---- rooms, elements, skills, ui ----------------------------------------------
def skull(color='#f0ead8'):
    c = C({'b': color})
    c.ellipse(8, 7, 6, 5.5, 'b')
    c.rect(5, 11, 6, 3, 'b', light=0.7)
    c.pxs(5, 6, ['KK..KK', 'KK..KK'], {'K': '#2a1a2a'})
    c.pxs(6, 12, ['K.K.'], {'K': '#2a1a2a'})
    return c


def crossed_swords():
    c = C({'m': '#dfe8ff', 'h': '#8a5a3a'})
    c.line([(2, 14), (13, 3)], 'm', width=2, light=0.9)
    c.line([(14, 14), (3, 3)], 'm', width=2, light=0.9)
    c.line([(1, 15), (3, 13)], 'h', width=2); c.line([(15, 15), (13, 13)], 'h', width=2)
    return c


def chest():
    c = C({'w': '#a8703a', 'g': '#ffcc33'})
    c.rect(2, 6, 12, 9, 'w', round_=1)
    c.rect(2, 3, 12, 5, 'w', round_=2, light=0.8)
    c.rect(2, 7, 12, 1.5, 'g', light=0.8)
    c.rect(7, 8, 2, 3, 'g', light=0.95)
    return c


def trap():
    c = C({'m': '#c8c8d8'})
    for x in (3, 7, 11):
        c.poly([(x - 2, 14), (x + 1, 4), (x + 4, 14)], 'm', light=0.8)
    return c


def fountain():
    c = C({'s': '#9aa0b8', 'w': '#5ac8f0'})
    c.ellipse(8, 12, 7, 3, 's')
    c.ellipse(8, 11.5, 5.5, 1.8, 'w', light=0.9)
    c.rect(7, 4, 2, 8, 's', light=0.7)
    for x, y in ((5, 4), (11, 4), (8, 2)):
        c.px(x, y, '#8ae8ff')
    return c


def merchant():
    c = P.bag()
    c.pxs(6, 9, ['$'], {'$': '#ffd23f'})
    return c


def event():
    c = C({'p': '#c88aff'})
    c.pxs(4, 2, ['.PPPP.', 'PP..PP', '....PP', '...PP.', '..PP..', '..PP..', '......', '..PP..'], {'P': '#e8c8ff'})
    return c


def stairs():
    c = C({'s': '#9aa0b8'})
    for i in range(4):
        c.rect(2 + i * 3, 12 - i * 3, 14 - i * 3, 3, 's', light=0.5 + i * 0.1)
    return c


def boss():
    c = skull('#ff6a6a')
    c.poly([(3, 3), (3, 0), (5, 2), (8, 0), (11, 2), (13, 0), (13, 3)], 'b', light=0.9)
    return c


ROOMS = [None, crossed_swords, lambda: skull(), chest, trap, fountain, merchant, event, boss, stairs]

EL_COLORS = ['#c8c8d8', '#ff6a3a', '#3aa8ff', '#c89a4a', '#7ae8a0', '#fff0a0', '#a85ae8']


def element(e):
    c = C({'g': EL_COLORS[e]})
    c.poly([(8, 1), (14, 8), (8, 15), (2, 8)], 'g', light=0.75)
    c.px(7, 4, '#ffffff')
    return c


def sk_strike():
    return P.sword()


def sk_multi():
    c = C({'m': '#dfe8ff'})
    for o in (0, 4, 8):
        c.line([(1 + o, 14), (6 + o, 2)], 'm', width=1.6, light=0.9)
    return c


def sk_cleave():
    c = C({'m': '#dfe8ff'})
    c.ellipse(8, 8, 7, 7, 'm', clip=lambda xs, ys: ys < 8)
    c.clear(lambda xs, ys: (xs - 8) ** 2 + (ys - 8) ** 2 < 22)
    c.poly([(1, 8), (4, 6), (3, 10)], 'm', light=0.9)
    return c


def sk_bolt():
    return P.bolt()


def sk_nova():
    return P.burst()


def sk_heal():
    c = C({'g': '#7ae86a'})
    c.rect(6, 2, 4, 12, 'g', light=0.8)
    c.rect(2, 6, 12, 4, 'g', light=0.8)
    return c


def sk_buff():
    c = C({'g': '#ffd23f'})
    c.poly([(8, 1), (14, 8), (10, 8), (10, 15), (6, 15), (6, 8), (2, 8)], 'g', light=0.85)
    return c


def sk_debuff():
    c = C({'p': '#a85ae8'})
    c.poly([(8, 15), (14, 8), (10, 8), (10, 1), (6, 1), (6, 8), (2, 8)], 'p', light=0.75)
    return c


def sk_drain():
    c = P.heart()
    c.pxs(6, 5, ['K..K', '.KK.'], {'K': '#5a1a3a'})
    return c


def sk_summon():
    return skull('#c8f0ff')


def sk_passive():
    c = C({'g': '#8ae8ff'})
    c.circle(8, 8, 6.5, 'g')
    c.circle(8, 8, 3, 'g', light=0.95)
    return c


SKILL_KINDS = [sk_strike, sk_multi, sk_cleave, sk_bolt, sk_nova, sk_heal, sk_buff, sk_debuff, sk_drain, sk_summon, sk_passive]


def anvil():
    c = C({'m': '#6a6a7a'})
    c.poly([(1, 4), (15, 4), (12, 8), (4, 8)], 'm', light=0.75)
    c.rect(6, 8, 4, 4, 'm', light=0.5)
    c.rect(3, 12, 10, 3, 'm', light=0.6)
    return c


def bed():
    c = C({'w': '#a8703a', 'b': '#e8e8f0', 'r': '#c84a5a'})
    c.rect(1, 8, 14, 4, 'r', light=0.7)
    c.rect(1, 6, 5, 3, 'b', light=0.9)
    c.rect(1, 12, 2, 3, 'w'); c.rect(13, 12, 2, 3, 'w')
    return c


def crown():
    c = C({'g': '#ffd23f', 'r': '#e84a6a'})
    c.poly([(2, 13), (2, 4), (5, 8), (8, 3), (11, 8), (14, 4), (14, 13)], 'g', light=0.85)
    c.px(8, 9, '#e84a6a')
    return c


def auto():
    c = C({'a': '#8ae8ff'})
    c.ellipse(8, 8, 6.5, 6.5, 'a')
    c.clear(lambda xs, ys: (xs - 8) ** 2 + (ys - 8) ** 2 < 16)
    c.clear(lambda xs, ys: (xs > 8) & (ys < 6))
    c.poly([(9, 1), (14, 4), (9, 7)], 'a', light=0.9)
    return c


def lock():
    c = C({'m': '#c8c0a0'})
    c.ellipse(8, 6, 4, 4.5, 'm')
    c.clear(lambda xs, ys: ((xs - 8) / 2.2) ** 2 + ((ys - 6) / 2.8) ** 2 < 1)
    c.rect(3, 7, 10, 8, 'm', round_=1, light=0.75)
    return c


def drop():
    c = C({'b': '#5aa0ff'})
    c.flame(8, 14, 9, 13, 'b')
    return c


def gem():
    c = C({'p': '#ff6ad8', 'q': '#ffb0f0'})
    c.poly([(4, 3), (12, 3), (15, 7), (8, 15), (1, 7)], 'p', light=0.75)
    c.poly([(4, 3), (12, 3), (10, 7), (6, 7)], 'q', light=0.95)
    return c


def plus():
    c = C({'g': '#8ee86a'})
    c.rect(6, 2, 4, 12, 'g', light=0.85)
    c.rect(2, 6, 12, 4, 'g', light=0.85)
    return c


def star_badge():
    return P.star()


UI = [('back', P.back), ('coin', P.coin), ('heart', P.heart), ('sp', drop), ('stamina', P.bolt), ('food', P.food),
      ('morale', P.smile), ('bag', P.bag), ('map', P.map_), ('link', P.link), ('anvil', anvil), ('bed', bed),
      ('crown', crown), ('scroll', P.scroll), ('gift', P.gift), ('auto', auto), ('skull', lambda: skull()),
      ('lock', lock), ('gem', gem), ('plus', plus), ('star', star_badge), ('sword', P.sword), ('shield', P.shield),
      ('run', P.run), ('sparkle', P.sparkle), ('zzz', P.zzz), ('trophy', P.trophy), ('stairs', stairs),
      ('chest', chest), ('moon', P.moon), ('sun', P.sun)]
