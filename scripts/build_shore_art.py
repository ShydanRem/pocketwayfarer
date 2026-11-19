#!/usr/bin/env python3
"""Import Glass Shore, Sunken Temple and the eight Classic coastal species."""
import argparse

from build_lumen_art import ROOT
from build_region_art import build_region

SOURCE = ROOT / "assets/shore/source"
SPECIES = [
    ("SP_CRAB", 48, 30), ("SP_JELLY", 37, 43),
    ("SP_BLUE_SLIME", 48, 30), ("SP_GOBLIN", 45, 50),
    ("SP_SERPENT", 51, 51), ("SP_DEEP_GHOST", 45, 50),
    ("SP_CRAB_KING", 77, 63), ("SP_LEVIATHAN", 78, 64),
]
# The lower row is offset in the original atlas. Split at the clear gutters.
CROPS = [
    (left, top, right, bottom)
    for cuts, top, bottom in [((0, 443, 887, 1331, 1774), 0, 423),
                             ((0, 440, 839, 1327, 1774), 423, 887)]
    for left, right in zip(cuts, cuts[1:])
]


def build(resolution):
    scenes = (("field", "classic_shore"), ("temple", "classic_temple"))
    return build_region("shore", SOURCE, scenes, SPECIES, resolution=resolution, crops=CROPS)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--resolution", type=int, choices=(233, 466), default=233,
                        help="sample 466 for the full-size benchmark")
    args = parser.parse_args()
    for path, contents in build(args.resolution).items():
        if args.check:
            if not path.exists() or path.read_text() != contents:
                raise SystemExit(f"Stale Shore art: {path.relative_to(ROOT)}")
        else:
            path.write_text(contents)
    print(f"Glass Shore and Sunken Temple at {args.resolution}px, eight coastal monsters verified")


if __name__ == "__main__":
    main()
