# Lanternbound art sources

The `0.4.0-preview.1` clarity revision replaces the village, arena, three loadouts and seven enemy species with original images generated using OpenAI ImageGen on 2026-11-17. Composition and runtime checks are documented in [ART-CLARITY.md](../../docs/ART-CLARITY.md). These are AI-generated sources followed by deterministic mechanical import, not assets claimed to be hand-drawn. Applicable generation-tool terms apply; no third-party stock license or independent license grant is asserted. No artwork from Octopath Traveler or Final Fantasy is included.

## Active sources

| Source | Generated source identity | Layout |
|---|---|---|
| `source/prism/environments.png` | `exec-3f855ead-5f2c-4257-b941-0960256aaf8f.png` | Quiet-stage edit, 2 backgrounds, 1774×887 |
| `source/prism/loadouts.png` | `exec-99272326-4b66-4fe5-88f8-c037954ad9da.png` | Layout-repaired edit, 4 poses × 3 heroes, 1448×1086 RGBA |
| `source/prism/enemies.png` | `exec-a93d5e0b-6991-4e14-9092-980ab0c6f303.png` | Layout-repaired edit, 4 poses × 7 species, 948×1659 RGBA |
| `source/relics.png` | `exec-5703c2e6-fa2a-4af4-a367-4c317a760f3d.png` | Original retained 3×2 rewards, 1536×1024 RGBA |

Exact generation and correction prompts are retained alongside each source. The busy first environment proposal (`exec-87c864fc-9001-4d5d-879e-ffd64457b560.png`) is retained under `source/prism/rejected/` for review and is **not imported**. Initial hero/enemy generations required a layout repair because weapons/poses crossed cell boundaries; the final imported sheets pass transparent-border validation. Their initial identities were `exec-a250b3a5-33f8-4b7e-ab1e-0ec6cb233061.png` and `exec-d7e94457-0c82-415d-93d1-7135505196d4.png`.

`scripts/build_lantern_art.py` leaves source PNGs intact. Backgrounds become native 466×466 indexed RGB565 images, displayed at 1×. Actor rows use a shared scale, foot anchor at row 84 of an 88×88 canvas, nearest-neighbor sampling and alpha threshold 128. A single palette of at most 96 colors is shared by all four poses of each row, without dithering; backgrounds/relics use at most 240 colors. Four poses are idle, breathing, attack and guard/hit, not a full locomotion/combo set.

Enemy rows are not equally tall. `source/prism/layout.json` records row boundaries measured in transparent gutters; the importer rejects opaque border pixels instead of silently cropping them. Columns use proportional boundaries, preserving non-divisible source dimensions. This repair is distinct from editing the image creatively, which was performed with ImageGen.

**52 runtime entries:** 2 backgrounds, 44 actor frames and 6 relics. `ln_actors[0]` duplicates the new Sentinel for compatibility; root keeper and wisp occupy the other two rows. `ln_loadouts` holds the three heroes; `ln_bestiary` holds wolf, crab, bat, moth and lighthouse automaton. Hero/single enemy/boss use 2× in combat; enemy pairs use 1× with separate touch boxes. This treatment uses painted depth and procedural LVGL effects, not a dynamic 3D scene.

## Retained historical sources

The five original atlases remain unchanged under `source/`: `environments.png` (`exec-f932f43c-0eaf-4502-ba85-43d41fa58164.png`), `actors.png` (`exec-92d3614e-be85-4976-bbed-c69fe97c7c52.png`), `loadouts.png` (`exec-edea722c-2502-4dcb-a968-a4153bcf5ee6.png`), `bestiary.png` (`exec-1cb4119f-5c42-4925-bb72-d4b115a76af7.png`) and the still-active relics. Their hashes remain in `manifest.json`; active replacement hashes are under `active_prism_sources`. They are provenance/history, not all active game imagery. The classic game's 40 classes and paper-doll combinations retain their prior artwork.

## Fonts, checks and limits

`fonts/body.ttf` is DejaVu Sans; `fonts/title.ttf` is DejaVu Serif Bold. Unmodified source fonts from `fonts-dejavu-core` are rasterized at native 20px/22px with 4-bit coverage, including Italian accents/punctuation. `fonts/LICENSE.txt` retains the Bitstream Vera license and notices. Runtime symbols are `ln_font_body` and `ln_font_title`.

```sh
python3 scripts/build_lantern_art.py
python3 scripts/build_lantern_art.py --check
```

Validation includes source format/opacity/border checks, deterministic output comparison, native LVGL compilation, 49 scripted captures plus 3 timing regressions, framebuffer comparison, ASan/UBSan and an ESP-IDF build. Native hero alpha pixels are checked against HUD boundaries to prevent covered feet. Static conversion does not certify physical frame time, touch latency, battery use or readability at the device's diameter. Pixel-cluster refinement and richer animation remain art work, not automatically certified by these checks.

Combat slash/ring/particle effects and two-layer contact shadows are original procedural C/LVGL graphics, with no external asset dependency. Motion uses saved phase time and honors pause/reduced motion. No paid-provider, Tripo/Leonardo job or canonical `game-dev` package was used; that CLI is unavailable here. ImageGen and the local deterministic importer supply the asset workflow. Full provenance is in `asset-register.csv` and `manifest.json`.
