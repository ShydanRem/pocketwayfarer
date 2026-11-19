# Borgo e Prati di Lumen

OpenAI ImageGen ha prodotto due tavole originali il 19 novembre 2026. I PNG sorgente sono conservati con i prompt esatti e usano i termini applicabili dello strumento di generazione. I caratteri DejaVu sono quelli già distribuiti con Lanternbound. [Licenza dei font](../lantern/fonts/LICENSE.txt)

| Sorgente | Generazione | Uso |
|---|---|---|
| [Ambienti](source/environments.png) · [Prompt](source/environments-prompt.txt) | `exec-36c3534e-f8e1-4d30-8b90-873be7fbf497.png` | Borgo a sinistra, radura a destra. Due fondali a 466 × 466. La variante notturna del borgo viene oscurata dal renderer. |
| [Mostri](source/monsters.png) · [Prompt](source/monsters-prompt.txt) | `exec-12407ac2-5165-4123-9866-084cba449e8c.png` | Tavola 4 × 2 con trasparenza, ordinata per riga come nel manifesto. |

Nel borgo la pietra fredda della piazza separa l’eroe dalle lanterne e dai tetti color ambra. Nei Prati le isole e gli alberi restano in alto e ai margini. Il terreno verde petrolio lascia spazio al lupo blu, alla fiamma calda di Scintillo e alle altre silhouette. LVGL compone il fondale con pannelli, barre, attori e indicatori.

`python3 scripts/build_lumen_art.py` ripete l’importazione con riduzione nearest-neighbor. I fondali hanno al massimo 240 colori, i mostri 96 e una maschera alfa con soglia 128. Il manifesto registra hash, riquadri sorgente e dimensioni finali. La suite confronta i file generati e rifiuta celle senza sagoma o con pixel opachi sui bordi. [Manifesto](manifest.json) · [Catture nel renderer](../../docs/roadmap/lumen-evidence/README.md)
