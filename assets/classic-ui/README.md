# Classic archive and world atlas

OpenAI ImageGen produced the archive interior on 19 November 2026. The original is retained as `source/archive.png` (1254 × 1254, opaque). Generation identity: `exec-25d30469-b61d-43ce-8a3e-100647888b62.png`. The exact description is in `source/archive-prompt.txt`. The artwork uses the applicable generation-tool terms. The interface uses the DejaVu typefaces already distributed with Lanternbound. [Font license](../lantern/fonts/LICENSE.txt)

The archive keeps lamps and shelf detail near the edge of the circular screen. Its central wall is dark so that the hero, labels and controls remain legible. The source image contains scenery only. LVGL draws the actual controls and character layers.

OpenAI ImageGen produced the world atlas on 19 November 2026. The original is `source/world.png` (1254 × 1254, opaque). Generation identity is `exec-b8ec69de-59f9-4dff-8f77-2930bf4b4e2f.png`. The six islands are scenery. LVGL draws region names, touch targets and state indicators over the artwork. [Exact prompt](source/world-prompt.txt) · [Native captures](../../docs/roadmap/services-evidence/README.md)

The importer samples the archive at 466 × 466 and the world atlas at 233 × 233 with nearest-neighbor. Both use up to 240 colors without dithering and indexed RGB565. The display expands the atlas by 2×. Rebuild with `python3 scripts/build_classic_ui_art.py`. The host suite checks the generated data and manifest against the source and prompt. [Manifest](manifest.json) · [Archive captures](../../docs/roadmap/classic-ui-evidence/README.md)
