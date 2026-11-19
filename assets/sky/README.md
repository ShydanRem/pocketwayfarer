# Guglie del Cielo e Cittadella Celeste

OpenAI ImageGen ha prodotto i due fondali e le otto sagome originali il 19 novembre 2026. Gli originali e i prompt sono conservati e usano i termini applicabili dello strumento di generazione.

| Sorgente | Generazione | Impiego |
|---|---|---|
| [Fondali](source/environments.png) · [Prompt](source/environments-prompt.txt) | `exec-7e1d6eb5-a19f-4a58-b85b-05069b2578b3.png` | Terrazza di arenaria viola sopra nubi color corallo. Nella Cittadella, pietra verde petrolio e sfere armillari sotto una vetrata. |
| [Tavola dei mostri](source/monsters.png) · [Prompt](source/monsters-prompt.txt) | `exec-a5f0103e-48e0-4a54-b54b-4a2227346117.png` | Otto sagome su fondo trasparente. I ritagli inferiori seguono i margini reali delle forme più larghe. |

`python3 scripts/build_sky_art.py` genera fondali a 233 × 233 e sagome indicizzate RGB565. I fondali sono ingranditi di 2× a schermo. L’alfa dei mostri viene convertito con soglia 128. La mappa del dungeon riusa il fondale della Cittadella con una velatura. [Manifesto e hash](manifest.json) · [Catture e prove](../../docs/roadmap/sky-evidence/README.md)
