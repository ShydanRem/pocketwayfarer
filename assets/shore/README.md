# Riva di Vetro e Tempio Sommerso

OpenAI ImageGen ha prodotto i due fondali e le otto creature originali il 19 novembre 2026. I file sorgente sono conservati con i prompt e usano i termini applicabili dello strumento di generazione.

| Sorgente | Generazione | Impiego |
|---|---|---|
| [Fondali](source/environments.png) · [Prompt](source/environments-prompt.txt) | `exec-710ccb4f-8a54-4848-8bed-3b2e98ba88bf.png` | Ardesia al tramonto a sinistra, rovina verde sommersa a destra. Dettagli ai bordi e pavimento uniforme dietro gli attori. |
| [Tavola dei mostri](source/monsters.png) · [Prompt](source/monsters-prompt.txt) | `exec-f6e286bc-9f36-4b65-8e42-9b4788085ad7.png` | Otto sprite con alfa. La seconda riga ha colonne leggermente spostate, ritagliate lungo i margini trasparenti effettivi. |

`python3 scripts/build_shore_art.py` rigenera gli asset indicizzati RGB565. `--resolution 466` genera la variante per il confronto. Rieseguire senza opzioni per ripristinare il formato della preview a 233 × 233 e verificarlo con `--check`. La tavola dei mostri conserva la stessa scala e gli stessi colori nelle due varianti. [Manifesto](manifest.json) · [Confronto nel renderer](../../docs/roadmap/shore-evidence/README.md)
