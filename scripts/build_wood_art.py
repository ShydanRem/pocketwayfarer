#!/usr/bin/env python3
"""Import Whispering Wood, Elder Roots and the eight Classic woodland species."""
import argparse

from build_lumen_art import ROOT
from build_region_art import build_region

SOURCE = ROOT / "assets/wood/source"
SCENES = (("field", "classic_wood"), ("roots", "classic_roots"))
SPECIES = [
    ("SP_TREANT", 43, 42), ("SP_OWL", 37, 46),
    ("SP_DIRE_WOLF", 51, 43), ("SP_SPIDER", 46, 34),
    ("SP_TOXIC_SHROOM", 46, 44), ("SP_DARK_FAIRY", 46, 47),
    ("SP_ELDER_STUMP", 77, 62), ("SP_SPIDER_QUEEN", 77, 64),
]
# Fairy and stump overlap horizontally, with separate alpha components.
CROPS = [
    (0, 0, 443, 423), (443, 0, 834, 423), (834, 0, 1342, 423), (1342, 0, 1774, 423),
    (0, 423, 400, 887), (400, 423, 768, 887), (690, 423, 1322, 887), (1322, 423, 1774, 887),
]
COMPONENT_SEEDS = {"SP_DARK_FAIRY": (575, 640), "SP_ELDER_STUMP": (1040, 630)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    for path, contents in build_region("wood", SOURCE, SCENES, SPECIES,
                                       crops=CROPS, component_seeds=COMPONENT_SEEDS).items():
        if args.check:
            if not path.exists() or path.read_text() != contents:
                raise SystemExit(f"Stale Wood art: {path.relative_to(ROOT)}")
        else:
            path.write_text(contents)
    print("Whispering Wood and Elder Roots at 233px, eight woodland monsters verified")


if __name__ == "__main__":
    main()
