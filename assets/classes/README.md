# Class illustrations — bases, advanced and first masters

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

At R3.1 the importer emitted 16 exact-ID illustrations, preserving the original base batch and using a shared scale within each eight-class batch. At that baseline, IDs 16–39 used a family reference. `manifest.json` lists both sheets, class IDs, bounds, scale, source hashes and prompt hashes. [Native R3.1 evidence](../../docs/roadmap/advanced-a-evidence/README.md) covers all 80IT/EN cards, 33 captures and no change to the personalized hero.

## R3.2 — exact advanced IDs 16–23

Eight original illustrations generated with OpenAI ImageGen on 2026-11-18: Paladin, Priest, Champion, Mystic, Blacksmith, Alchemist, Necromancer, Warlock. Final source `source/advanced-b.png`, 1774 × 887 RGBA, identity `exec-b96271a7-b2d7-4116-9dba-ad5845a01c6a.png`. Creation and layout-repair prompts are retained. References were the approved first advanced illustrations and the new costume/headwear sheets.

Initial `exec-61a00576-43e8-47f0-90b4-68920c36db0c.png` passed the binary transparent-edge test but placed figures too close to cell borders. It was rejected for insufficient gutters, retained unaltered as `source/rejected/advanced-b-tight-gutters.png`. ImageGen regenerated a more generous layout with complete silhouettes; the importer does not reconstruct or paint missing art locally.

At R3.2 there were 24 exact-ID portraits, preserving each existing batch's scale and geometry. Master IDs 24–39 still used the explicit family reference. Source/prompt hashes, identities, bounds and batch scales are in `manifest.json`. [R3.2 evidence](../../docs/roadmap/advanced-b-evidence/README.md) records that baseline's 80 IT/EN cards, 49 native captures and unchanged player profile.

The generation briefs use the descriptive names Smith/Sorcerer for IDs 20/23; canonical UI names are Blacksmith/Warlock from the existing core table. Exact prompts remain unaltered.

## R4.1 — exact master IDs 24–31

Eight original representatives generated with OpenAI ImageGen on 2026-11-18: Dragon Knight, Warlord, Archmage, Chronomancer, Beastmaster, Stormbow, Nightblade and Blademaster. Source `source/master-a.png`, 1774 × 887 RGBA, identity `exec-fca3f64a-0534-46e6-bcdf-c970dde9f98c.png`; exact prompt `source/master-a-prompt.txt`. The approved advanced-A illustration sheet was the only image reference; the prompt defines matching large costume forms for both modular and representative art.

The source passed cell-edge alpha and native header/badge containment checks without a repair job. It is retained unaltered, with no local painting. Mechanical import preserves previous batches and registers one common scale/foot baseline for the new eight. Coverage is now **32 exact illustrations**, with explicit family references only for IDs32–39. Source identity, hashes, measured bounds, scale and prompt hash are recorded in `manifest.json`. [R4.1 evidence](../../docs/roadmap/master-a-evidence/README.md) covers all 80 IT/EN cards and 64 native atlas captures; physical testing remains open.

The master generation briefs use descriptive names Beast Lord/Storm Archer/Night Blade/Blade Master for IDs 28–31; canonical English UI names remain Beastmaster/Stormbow/Nightblade/Blademaster from the core table. Exact prompts are retained unaltered.
