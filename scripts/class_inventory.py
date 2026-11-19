#!/usr/bin/env python3
"""Keep the production ledger aligned with the existing 40-class core table."""
import argparse
import csv
import io
import re
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'docs/roadmap/classes.csv'


def inventory():
    rig=(ROOT/'firmware/rpg/ui/art/hero_rig_art.h').read_text()
    art=(ROOT/'firmware/rpg/ui/art/class_art.h').read_text()
    rig_count=int(re.search(r'#define HERO_RIG_CLASS_COUNT (\d+)',rig)[1])
    art_count=int(re.search(r'#define CLASS_ART_COUNT (\d+)',art)[1])
    assert rig_count==art_count==40, 'Review admitted class coverage'
    data=(ROOT/'firmware/rpg/core/data.c').read_text().split('class_names[RPG_CLASS_COUNT] = {',1)[1].split('\n};',1)[0]
    names=re.findall(r'\{T\("([^"]+)", "([^"]+)"\), T',data)
    assert len(names)==40, 'Review roadmap when the class schema changes'
    out=io.StringIO()
    writer=csv.writer(out,lineterminator='\n')
    writer.writerow(['id','italiano','english','family_id','tier','parent_id','branch','illustration','player_sprite','next_milestone'])
    for c,(it,en) in enumerate(names):
        family=c if c<8 else (c-8)//2 if c<24 else (c-24)//2
        tier=1 if c<8 else 2 if c<24 else 3
        parent=-1 if c<8 else family if c<24 else c-16
        writer.writerow([c,it,en,family,tier,parent,'base' if c<8 else 'A' if c%2==0 else 'B',
                         'base_imported' if c<8 else 'specific_imported' if c<art_count else 'explicit_family_reference',
                         'modular_exact_class' if c<rig_count else 'legacy_family_tier',
                         'R5' if c<rig_count else 'R4'])
    return out.getvalue()


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--check',action='store_true');args=ap.parse_args()
    data=inventory()
    if args.check: assert DEST.read_text()==data, 'stale class production ledger'
    else: DEST.parent.mkdir(parents=True,exist_ok=True);DEST.write_text(data)
    print('class ledger:40 IDs,8 families,16 advanced and16 master paths checked')


if __name__=='__main__': main()
