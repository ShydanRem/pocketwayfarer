# Lanternbound art sources

Original HD-2D art generated for Pocket Wayfarer with the available OpenAI ImageGen tool on 2026-11-16, following the user-selected direction in `docs/audit/HD2D-REDESIGN.md`. Five source atlases, exact prompts and the original approved concept are retained. Imagery is recorded as AI-generated under the applicable tool terms; no third-party stock license or independent license grant is asserted.

| Source | Generated source identity | Layout |
|---|---|---|
| `source/environments.png` | `exec-f932f43c-0eaf-4502-ba85-43d41fa58164.png` | 2 backgrounds, 1774×887 |
| `source/actors.png` | `exec-92d3614e-be85-4976-bbed-c69fe97c7c52.png` | 4 frames × 3 actors, 1448×1086 RGBA |
| `source/relics.png` | `exec-5703c2e6-fa2a-4af4-a367-4c317a760f3d.png` | 3×2 rewards, 1536×1024 RGBA |
| `source/loadouts.png` | `exec-edea722c-2502-4dcb-a968-a4153bcf5ee6.png` | 4 frames × Sentinel, Ember Mage, Trickster, 1448×1086 RGBA |
| `source/bestiary.png` | `exec-1cb4119f-5c42-4925-bb72-d4b115a76af7.png` | 4 frames × wolf, crab, bat, moth, brass lighthouse boss, 1122×1402 RGBA |

Sources are never painted over by the importer. `scripts/build_lantern_art.py` mechanically extracts equal fractional grid cells, uses shared scale for all four poses in each actor row, aligns the feet to row 84 of the 88×88 target, samples with nearest-neighbor, thresholds alpha at 128, quantizes to at most 240 colors without dithering, and emits indexed RGB565 C data. Non-divisible source dimensions use proportional cell boundaries, not discarded edge strips. Four poses are idle, breathing, attack and guard/hit, not a full locomotion/combo set.

**52 runtime images:** 2 backgrounds at 233×233, 44 actor frames at 88×88, 6 relics at 88×88. The original four hero frames remain imported but the playable hero uses three distinct loadout rows. Root keeper and ember wisp retain their original atlas rows; five new enemy rows provide the other four ordinary species and the unique boss. Runtime scales are integers; backgrounds use 2×. Their depth and light are painted, not dynamic 3D rendering.

The new appearances do not replace all paper-doll combinations or the art of the 40 classic classes. Existing classic art and detailed customization remain available. Provenance and checks are in `asset-register.csv`; source hashes and deterministic import settings are in `manifest.json`.

`fonts/body.ttf` is DejaVu Sans; `fonts/title.ttf` is DejaVu Serif Bold. Unmodified source fonts from `fonts-dejavu-core` are rasterized at native 20px/22px with 4-bit coverage, including Italian accents/punctuation. `fonts/LICENSE.txt` retains the Bitstream Vera license and notices; the Debian packaging section does not relicense the font files. Runtime symbols are `ln_font_body` and `ln_font_title`.

```sh
python3 scripts/build_lantern_art.py
python3 scripts/build_lantern_art.py --check
```

Validation includes source format/opacity checks, deterministic data comparison, actual LVGL compilation, scripted integration with 45 native captures, and interactive desktop inspection. Static conversion does not certify device frame time, touch latency, battery use or readability at physical diameter. No paid provider, Tripo/Leonardo job or canonical `game-dev` package was used. Game Development Studio CLI is unavailable here; ImageGen and the local deterministic importer provide the asset workflow.

Combat slash/ring/particle effects in `r_lantern_fx.c` are original procedural C graphics, with no additional external assets. Saved phase time drives motion; reduced motion removes actor displacement, motes and defeat fade. The source atlases/fonts and their import hashes are unchanged by this increment.
