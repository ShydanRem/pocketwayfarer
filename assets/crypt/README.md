# Cripta dei Lumi

Scena e mostri originali prodotti con OpenAI ImageGen il 19 novembre 2026. I PNG originali e i prompt sono conservati e usano i termini applicabili dello strumento di generazione.

| Sorgente | Generazione | Impiego |
|---|---|---|
| [Fondale](source/environment.png) · [Prompt](source/environment-prompt.txt) | `exec-dd0ebdf8-6af3-4bb8-a2cb-96e630e79a80.png` | Basalto viola, vetro votivo e lanterne color ambra. Il centro più uniforme fa risaltare le connessioni della mappa e le sagome del combattimento. |
| [Tavola dei mostri](source/monsters.png) · [Prompt](source/monsters-prompt.txt) | `exec-2df65a86-2b5f-4d33-bed3-644158406c88.png` | Mimic a sinistra, Re delle Ossa a destra. Il primo ha una sagoma bassa in bronzo ossidato, il secondo un profilo più alto in avorio e bordeaux. |

Il fondale è condiviso da mappa e arena. La mappa applica una velatura per aumentare il contrasto dei collegamenti. I nemici mantengono una posa illustrata con gli affondi, i segnali e gli effetti del renderer. `python3 scripts/build_crypt_art.py` rigenera i buffer indicizzati e il manifesto con le dimensioni, gli ID e gli hash. [Manifesto](manifest.json) · [Catture e prove](../../docs/roadmap/crypt-evidence/README.md)
