# Class illustrations — base and first advanced batches

Eight original representative illustrations generated with OpenAI ImageGen on 2026-11-17. Source: `source/base-classes.png`, generated identity `exec-ee986e68-ed60-45bf-ae12-58e2a8490899.png`, 1774 × 887 RGBA, four columns and two rows. The retained `source/base-classes-prompt.txt` is the exact generation prompt. `assets/lantern/source/prism/loadouts.png` was supplied as a visual-world reference; the eight figures are new representatives, not new animations of those three loadouts.

Order: Warrior, Mage, Archer, Rogue, Cleric, Monk, Artificer, Occultist. These are **class-atlas illustrations**, not the player's customized avatar. At the R1 baseline the 32 advanced/master cards used an explicit family reference; R3.1 below replaces the first eight, leaving 24 references. No commercial game's characters or assets are included; generated imagery is recorded under the applicable generation-tool terms, without asserting an independent stock license or hand-drawn authorship.

`scripts/build_class_art.py` mechanically slices proportional cell boundaries, rejects opaque cell edges, normalizes all eight figures with one scale and foot anchor, fits silhouettes within 76 logical pixels on an 88 × 88 canvas, samples nearest-neighbor, thresholds alpha at 128 and quantizes each illustration to at most 96 colors without dithering. It emits indexed RGB565 into `firmware/rpg/ui/art/class_art.c/.h`. The source PNG remains intact. Source hash, measured bounds and transform settings are in `manifest.json`.

```sh
python3 scripts/build_class_art.py
python3 scripts/build_class_art.py --check
python3 scripts/class_inventory.py --check
```

The atlas shows one expanded 176 × 176 illustration at a time. Native acceptance reads actual alpha pixels against header/badge boundaries and tests all 40 cards in both languages, skill references, family nodes, closure and preservation of a non-default customized hero. Native rendering, sanitizer and firmware evidence is recorded with the R1 delivery. Source conversion alone does not certify device latency, physical readability, animation or the future modular avatar system.

Production rules and remaining work: [CLASS-ART-BIBLE.md](../../docs/CLASS-ART-BIBLE.md), [ROADMAP.md](../../docs/ROADMAP.md), [class ledger](../../docs/roadmap/classes.csv).

## R3.1 — exact advanced IDs 8–15

Eight additional original representatives generated with OpenAI ImageGen on 2026-11-18: Knight, Berserker, Elementalist, Arcanist, Ranger, Sniper, Assassin, Duelist. Final source `source/advanced-a.png` is 1774 × 887 RGBA, identity `exec-5103872b-f962-4f88-bf6e-a6227f49a34e.png`. Exact creation/alpha-repair prompts are retained alongside it. References were the approved base illustrations and new modular costume/headwear sheets; illustrations match their large costume shapes while remaining separate from the player's saved appearance.

Initial `exec-4a4fe69d-521e-4d59-b554-7f5c83c7901f.png` had a semi-opaque white background (alpha 132–252) and was rejected, retained as `source/rejected/advanced-a-white.png`. ImageGen repaired alpha; no local painting or background reconstruction was used. The final import validates transparent cell edges and thresholds at 128.

The importer now emits 16 exact-ID illustrations, preserving the original base batch and using a shared scale within each eight-class batch. IDs 16–39 explicitly use a family reference. `manifest.json` lists both sheets, class IDs, bounds, scale, source hashes and prompt hashes. [Native R3.1 evidence](../../docs/roadmap/advanced-a-evidence/README.md) covers all 80IT/EN cards, 33 captures and no change to the personalized hero.
