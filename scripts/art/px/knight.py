from pixel import grid, check

PAL = {
    'K': '#20152a',
    'L': '#ffe2c4', 'S': '#f2b58a', 's': '#c47c58', 't': '#8a4a3c',
    'G': '#f0a050', 'H': '#bc5e2c', 'h': '#7c3418', 'j': '#4a1a12',
    'E': '#20306a', 'e': '#5a8ae8',
    'M': '#f4f8ff', 'A': '#b4c2dc', 'a': '#7684a6', 'B': '#454c6c',
    'Y': '#ffd860', 'y': '#c08a2a',
    'P': '#ff7a6a', 'R': '#d43c44', 'r': '#86202e',
    'N': '#4c5ca6', 'n': '#2c346a',
    'D': '#9a6236', 'd': '#5c3418',
}

KNIGHT = [
    # 0         1         2         3
    # 01234567890123456789012345678901
    "................................",  # 0
    "..........KK....KKK.............",  # 1
    "..........KGK.KKGGHK..KK........",  # 2
    "........KKGGHKGGHHHHKKHK........",  # 3
    ".......KGGGHHHHHHHHHHHHK........",  # 4
    "......KGGHHHHHHHHHHHhHHHK.......",  # 5
    ".....KGGHHHHHHHHHHHhhhHHHK......",  # 6
    ".....KGHHHHHHHHHHHHhhhjhHK......",  # 7
    "....KGHHHHhHHHHHhhhhhjjhhK......",  # 8
    "....KHHHhHhhHHhhhhhhjjjhK.......",  # 9
    "....KHHhSShhSShhSShhhjjhK.......",  # 10
    "...KHHhSLSSSSSSSSSShhjhhK.......",  # 11
    "...KHhSLLSSSSSSSSSSShhhK........",  # 12
    "...KhSSKKKSSSKKSSSSShhhK........",  # 13
    "....KSSKeESSSKeSSSSssShK........",  # 14
    "...KSSSKEESSSKESSSSsssK.........",  # 15
    "...KSSSSSSSSSSSSSSssssK.........",  # 16
    "....KtSSSSSSSSSSSssssK..........",  # 17
    ".....KSSSSttSSSSsssSK...........",  # 18
    "......KKSSSSSSSssKKK............",  # 19
    "......KKAKKsssssKKBK............",  # 20
    "....KKMMAAKKKKKKaaaBKKK.........",  # 21
    "...KMMMAAAaYYYYaaaBBRRrK........",  # 22
    "..KMMAAAAaaAAAAAAaBBRRRrK.......",  # 23
    "..KMAAAAaaBMAAAAAaaBKRRrrK......",  # 24
    "..KAAaaaaBKMMAAAAAaaBKRRrrK.....",  # 25
    "...KBBBBBKSKMAAAAAaaBKKRRrK.....",  # 26
    "..KMKAAaKSSKAAAYAAaaBKBKRrrK....",  # 27
    "..KMKAaaKSSKAAYYYAaaBKBBKRrK....",  # 28
    "..KMKAaKSSKKAAAYAAaaBKaBBKRrK...",  # 29
    "..KMKaKSSK.KAAAAAaaaBKaaBKRrK...",  # 30
    ".KKMKKKsSK.KBBBBBBBBBKaaBKRrrK..",  # 31
    "KYYYYYKKK.KDDdYyDDDDdKaBBKRrrK..",  # 32
    ".KyyyK....KRRRRRRrrrKKBBK.KRrK..",  # 33
    "..KdK.....KPRRRRRrrrK.KKK.KRrrK.",  # 34
    "..KdK......KPRRRRrrK......KRrrK.",  # 35
    "..KYK......KNNKKNNnnK.....KRrrK.",  # 36
    "...K.......KNNNKKNNnnK....KRrrK.",  # 37
    "..........KNNNnK.KNNnnK...KrrrK.",  # 38
    "..........KNNNnK.KNNNnK...KrrK..",  # 39
    ".........KAAANnK.KAAANnK..KrK...",  # 40
    ".........KMAAaK..KMAAaaK..KK....",  # 41
    ".........KMAaaK...KMAaaK........",  # 42
    ".........KAAaaK...KAAaaK........",  # 43
    "........KDDDddK...KDDDddK.......",  # 44
    ".......KDDDDDddK..KDDDDddK......",  # 45
    ".......KKKKKKKKK..KKKKKKKKK.....",  # 46
    "................................",  # 47
]
check(KNIGHT, 32)
# sword raised in front of the body: blade on columns 2-3, rows 4..26
_rows = [list(r) for r in KNIGHT]
for y in range(4, 27):
    _rows[y][1] = 'K'
    _rows[y][2] = 'M' if y > 5 else 'K'
    _rows[y][3] = 'A' if y > 5 else 'K'
    _rows[y][4] = 'K' if _rows[y][4] in '.K' else _rows[y][4]
_rows[4][2] = _rows[4][3] = '.'
_rows[5][2] = 'K'; _rows[5][3] = 'K'
KNIGHT = [''.join(r) for r in _rows]


def knight():
    return grid(KNIGHT, PAL, 32)
