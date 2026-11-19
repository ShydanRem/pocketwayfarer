#!/usr/bin/env python3
"""Import Ember Caves, Infernal Forge and the eight Classic ember species."""
import argparse

from build_lumen_art import ROOT
from build_region_art import build_region

SOURCE = ROOT / "assets/ember/source"
SCENES = (("field", "classic_ember"), ("forge", "classic_forge"))
SPECIES = [
    ("SP_SALAMANDER", 51, 33), ("SP_LAVA_GOLEM", 50, 45),
    ("SP_FIRE_BAT", 52, 32), ("SP_GOBLIN_MINER", 44, 49),
    ("SP_IMP", 43, 43), ("SP_HELL_KNIGHT", 42, 50),
    ("SP_ASH_DRAGON", 77, 64), ("SP_DEMON_LORD", 76, 63),
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    for path, contents in build_region("ember", SOURCE, SCENES, SPECIES).items():
        if args.check:
            if not path.exists() or path.read_text() != contents:
                raise SystemExit(f"Stale Ember art: {path.relative_to(ROOT)}")
        else:
            path.write_text(contents)
    print("Ember Caves and Infernal Forge at 233px, eight ember monsters verified")


if __name__ == "__main__":
    main()
