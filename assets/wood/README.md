# Bosco Sussurrante e Radici Antiche

OpenAI ImageGen ha prodotto i due fondali e le otto sagome originali il 19 novembre 2026. Gli originali e i prompt sono conservati e usano i termini applicabili dello strumento di generazione.

| Sorgente | Generazione | Impiego |
|---|---|---|
| [Fondali](source/environments.png) · [Prompt](source/environments-prompt.txt) | `exec-5dc57456-88f7-4a8e-ab8d-0e4b7aeb9172.png` | Radura color salvia e chioma indaco a sinistra. Radici alte, pietra desaturata e funghi ai lati a destra. |
| [Tavola dei mostri](source/monsters.png) · [Prompt](source/monsters-prompt.txt) | `exec-ac81c07d-cf99-4045-80af-d8676b446745.png` | Otto sagome ordinate per riga. I ritagli seguono i margini reali. Fata oscura e Antico Ceppo sono isolati da componenti alfa connesse. |

`python3 scripts/build_wood_art.py` genera i fondali a 233 × 233 e le sagome indicizzate RGB565. La mappa riusa il fondale delle Radici Antiche con una velatura. Il nuovo importatore condiviso produce gli stessi file per la Riva, inclusi gli hash registrati. [Manifesto](manifest.json) · [Catture e prove](../../docs/roadmap/wood-evidence/README.md)
