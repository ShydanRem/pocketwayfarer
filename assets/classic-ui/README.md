# The Classic archive backdrop

OpenAI ImageGen produced the archive interior on 19 November 2026. The original is retained as `source/archive.png` (1254 × 1254, opaque). Generation identity: `exec-25d30469-b61d-43ce-8a3e-100647888b62.png`. The exact description is in `source/archive-prompt.txt`. The artwork uses the applicable generation-tool terms. The interface uses the DejaVu typefaces already distributed with Lanternbound. [Font license](../lantern/fonts/LICENSE.txt)

The archive keeps lamps and shelf detail near the edge of the circular screen. Its central wall is dark so that the hero, labels and controls remain legible. The source image contains scenery only. LVGL draws the actual controls and character layers.

The importer samples the original at 466 × 466 with nearest-neighbor, quantizes to 240 colors without dithering and emits an indexed RGB565 sprite. Rebuild with `python3 scripts/build_classic_ui_art.py`. The host suite checks the generated data and manifest against the source and prompt. [Native captures and evidence](../../docs/roadmap/classic-ui-evidence/README.md)
