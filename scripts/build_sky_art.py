#!/usr/bin/env python3
"""Import Sky Spires, Celestial Citadel and the final eight Classic species."""
import argparse

from build_lumen_art import ROOT
from build_region_art import build_region

SOURCE = ROOT / "assets/sky/source"
SCENES = (("spires", "classic_spires"), ("citadel", "classic_citadel"))
SPECIES = [
    ("SP_STORM_WISP", 37, 33), ("SP_GRIFFIN", 56, 46),
    ("SP_CRYSTAL", 27, 46), ("SP_HARPY", 50, 47),
    ("SP_SKY_KNIGHT", 43, 55), ("SP_CRYSTAL_GOLEM", 54, 47),
    ("SP_GUARDIAN", 66, 66), ("SP_STAR_DRAGON", 78, 67),
]
# The lower row has broader boss sprites. Split along the actual transparent
# gutters so the Warden's plate and the dragon's wings remain whole.
CELLS = [
    (0, 0, 444, 444), (444, 0, 887, 444), (887, 0, 1330, 444), (1330, 0, 1774, 444),
    (0, 444, 400, 887), (400, 444, 845, 887), (845, 444, 1228, 887), (1228, 444, 1774, 887),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    for path, contents in build_region("sky", SOURCE, SCENES, SPECIES, crops=CELLS).items():
        if args.check:
            if not path.exists() or path.read_text() != contents:
                raise SystemExit(f"Stale Sky art: {path.relative_to(ROOT)}")
        else:
            path.write_text(contents)
    print("Sky Spires and Celestial Citadel at 233px, eight sky monsters verified")


if __name__ == "__main__":
    main()
