from pixel import grid, check

SKIN = {'L': '#ffe8d0', 'S': '#f6c49c', 's': '#cc8a66', 't': '#94523e'}
BASE = {'K': '#20152a', 'E': '#20306a', 'W': '#ffffff'}

MAGE_PAL = dict(BASE, **SKIN, **{
    'G': '#fff2b0', 'H': '#f0cc60', 'h': '#c08a30', 'j': '#7a5020',      # blonde
    'e': '#b05ae8',                                                       # violet eyes
    'V': '#8a70f0', 'U': '#5a3cc8', 'u': '#36208a', 'v': '#1e1050',       # robe and hat
    'Y': '#ffd860', 'y': '#b8802a',                                       # gold trim
    'O': '#d8fbff', 'o': '#6ae0ff', 'q': '#2a8ac8',                       # orb
    'D': '#8a5a34', 'd': '#55331c',                                       # staff wood
    'P': '#ff8ab0',                                                       # blush / sash
})

MAGE = [
    # 0         1         2         3
    # 01234567890123456789012345678901
    "..................KK............",  # 0
    "................KKUuK...........",  # 1
    "..............KKVUUuK...........",  # 2
    ".....KK......KVVUUuuK...........",  # 3
    "....KOoK....KVVUUUuuK...........",  # 4
    "...KOOooK..KVUUUUuuvK...........",  # 5
    "...KoOoqK.KVUUUUUuuvK...........",  # 6
    "....KooK.KVVUUUUUuuvK...........",  # 7
    ".....KK.KVUUUUUUUuuuvK..........",  # 8
    ".....KDKYYYYYYYYYYYYyyK.........",  # 9
    "....KKDKKyyyyyyyyyyyyyyKK.......",  # 10
    "...KGGKDKKHHHHHHHHHhhhjjK.......",  # 11
    "...KGHHKDKHHGHHHHHHhhjjjK.......",  # 12
    "..KGHHSKDKSSLSSSSShhhhjjjK......",  # 13
    "..KHHSSKDSKKKSSSKKSShhjjjK......",  # 14
    "..KHSSSKDSKeESSSKeSSShjjjjK.....",  # 15
    "..KHSSSKDSKEESSSKESSShhjjjK.....",  # 16
    "..KHSSSKDSSSSSSSSSSSsshjjjK.....",  # 17
    "..KHhSSKDSPSSSSSSSSssShhjjK.....",  # 18
    "..KHhKKKDKSSSSttSSSssKhhjjK.....",  # 19
    "..KHhKSKDKKSSSSSSssKKKhhjjK.....",  # 20
    "..KhhKSSDKVKKsssssKKUKhjjK......",  # 21
    "...KhKSSDKVVYYYYYYYUUuKjjK......",  # 22
    "...KhKSSDKVVVUUUUUUUuuKjK.......",  # 23
    "....KKSSDKVVUUUUUUUUuuuKK.......",  # 24
    ".....KKKDKVVUUUPPPUUUuuK........",  # 25
    "......KDKVVUUUUPPPUUUuuuK.......",  # 26
    "......KDKVVUUUUUPUUUUuuuK.......",  # 27
    "......KDKVVUUUUUPUUUUuuuuK......",  # 28
    "......KDKVVUUUUUPUUUUUuuuK......",  # 29
    "......KDKVVUUUUUPUUUUUuuuuK.....",  # 30
    "......KDKVUUUUUUUUUUUUuuuuK.....",  # 31
    "......KDKVUUUUUUUUUUUUuuuuvK....",  # 32
    "......KDKVUUUUUUUUUUUUUuuuvK....",  # 33
    "......KDKVUUUUUUUUUUUUUuuuuvK...",  # 34
    "......KDKUUUUUUUUUUUUUUUuuuvK...",  # 35
    "......KDKVUUUUUUUUUUUUUUuuuuvK..",  # 36
    "......KDKVUUUUUUUUUUUUUUUuuuvK..",  # 37
    "......KDKUUUUUUUUUUUUUUUUuuuvK..",  # 38
    "......KDKUUUUUUUUUUUUUUUUuuuuvK.",  # 39
    "......KDKUUUUUUUUUUUUUUUUuuuuvK.",  # 40
    "......KDKYYYYYYYYYYYYYYYYYyyyyK.",  # 41
    "......KDKyyyyyyyyyyyyyyyyyyyyyK.",  # 42
    "......KDKKKKKKKKKKKKKKKKKKKKKKK.",  # 43
    "......KDK....KdddK...KdddK......",  # 44
    "......KdK...KddddK...KddddK.....",  # 45
    "......KKK...KKKKKK...KKKKKK.....",  # 46
    "................................",  # 47
]
check(MAGE, 32)

ARCH_PAL = dict(BASE, **SKIN, **{
    'G': '#ff9a6a', 'H': '#e05a3a', 'h': '#a0301e', 'j': '#5a1810',       # red hair
    'e': '#3ab86a',
    'F': '#7ad06a', 'C': '#3e9a4a', 'c': '#246034', 'x': '#123a20',       # hood and tunic
    'D': '#c48a4c', 'd': '#8a5a2c', 'b': '#55331a',                       # leather, bow
    'Z': '#f0e8d8',                                                       # bowstring
    'Y': '#ffd860',
})

ARCHER = [
    # 0         1         2         3
    # 01234567890123456789012345678901
    "................................",  # 0
    "...........KKKKKKK..............",  # 1
    ".........KKFCCCCCcKK............",  # 2
    "........KFFCCCCCCCccK...........",  # 3
    ".......KFCCCCCCCCCcccK..........",  # 4
    "......KFCCCCCCCCCCcccxK.........",  # 5
    "......KFCCKKKKKKCCCccxK.........",  # 6
    ".....KFCCKGGHHHHKCCccxxK........",  # 7
    ".....KFCKGGHHHHHHKCccxxK........",  # 8
    ".....KCCKGHHSSHHhKCcccxK........",  # 9
    "..K..KCKHHSLSSSSShKcccxK........",  # 10
    "..KbK.KKHSSLSSSSSShKccxK........",  # 11
    "..KbK.KKSKKSSSKKSShKccxK........",  # 12
    "..KbK..KSKeESSKeSSSKccxK........",  # 13
    "..KdbK.KSKEESSKESSsKcxxK........",  # 14
    "...KdK.KSSSSSSSSSssKcxxK........",  # 15
    "...KdK.KtSSSSSSSssKcxxK.........",  # 16
    "...KdbK.KSSStSSSsKKcxK..........",  # 17
    "....KdK..KKSSSssKKcxxK..........",  # 18
    "....KdK.KKCKsssKCCcxxK..KK......",  # 19
    "....KdKKFCCCKKKCCCccxKKZZK......",  # 20
    "....KdKFFCCCDDCCCCcccKdZK.......",  # 21
    "....KdKFCCCCCDDCCCccxKdK........",  # 22
    "....KdKFCCCCCCDDCCccxKdK........",  # 23
    "...KZdKKCCKCCCCDDCcxxKdK........",  # 24
    "...KZKSSKKSKCCCCDDcxxKdK........",  # 25
    "...KZKSSSKSKCCCCCDDxxKdK........",  # 26
    "...KZKKSSSKKCCCCCCcxxKdK........",  # 27
    "...KZdKKKKKKDDDYDDDDDKbK........",  # 28
    "...KZdK...KCCCCCCCcccxK.........",  # 29
    "...KZdK...KFCCCCCCcccxK.........",  # 30
    "...KZdK...KFCCCCCCccxxK.........",  # 31
    "...KZdK....KCCKKCCcxxK..........",  # 32
    "...KZdK....KDDKKDDddK...........",  # 33
    "...KZdK....KDDKKDDddK...........",  # 34
    "...KZdK...KDDdK.KDDddK..........",  # 35
    "...KZdK...KDDdK.KDDDdK..........",  # 36
    "...KZdK...KDDdK.KDDDdK..........",  # 37
    "...KZbK..KbDDdK..KbDDdK.........",  # 38
    "...KZbK..KbbDdK..KbbDdK.........",  # 39
    "....KbK..KbbddK..KbbddK.........",  # 40
    "....KbK..KbbddK..KbbddK.........",  # 41
    "....KbK.KbbbddK..KbbbddK........",  # 42
    ".....KK.KbbbbddK.KbbbbddK.......",  # 43
    "........KbbbbbddK.KbbbbbddK.....",  # 44
    "........KKKKKKKKK.KKKKKKKKK.....",  # 45
    "................................",  # 46
    "................................",  # 47
]
check(ARCHER, 32)


def mage():
    return grid(MAGE, MAGE_PAL, 32)


def archer():
    return grid(ARCHER, ARCH_PAL, 32)
