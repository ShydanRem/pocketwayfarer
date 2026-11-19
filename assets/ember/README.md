# Caverne di Brace e Fucina Infernale

OpenAI ImageGen ha prodotto i fondali e otto sagome originali il 19 novembre 2026. Gli originali e i prompt sono conservati e usano i termini applicabili dello strumento di generazione.

| Sorgente | Generazione | Impiego |
|---|---|---|
| [Fondali](source/environments.png) · [Prompt](source/environments-prompt.txt) | `exec-ed4242fe-0774-47e6-8bcc-db1ba0686062.png` | Basalto e lava ai lati a sinistra. Forgia in prospettiva, archi e metallo incandescente a destra. Pavimento di pietra fredda dietro gli attori in entrambe. |
| [Tavola dei mostri](source/monsters.png) · [Prompt](source/monsters-prompt.txt) | `exec-e4a3f676-7372-4521-9379-c6a201023ae2.png` | Otto sagome con alfa, ordinate per riga e ritagliate lungo i margini trasparenti. |

`python3 scripts/build_ember_art.py` genera i fondali a 233 × 233 e le sagome indicizzate RGB565. Il display espande i fondali di 2×. I mostri vengono separati con una soglia alfa prima di ridurre e indicizzare i colori. La mappa riusa il fondale della Fucina con una velatura. [Manifesto](manifest.json) · [Catture e prove](../../docs/roadmap/ember-evidence/README.md)
