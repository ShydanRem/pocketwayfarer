# Prove del lotto R1

Consegna `0.4.1-preview.1`: atlante delle classi e roadmap. Queste prove estendono quelle della baseline [0.4.0](../../lantern-evidence/README.md), senza presentare illustrazioni come avatar modulari completati.

- `host-checks.txt`: suite completa, importazioni deterministiche e inventario delle classi; core con sanitizer; regressioni del dispositivo/UI; preview classica, compagno, Lanternbound e atlante.
- `atlas-sanitizers.txt`: UI/LVGL dell'atlante con AddressSanitizer/UBSan, 80 schede IT/EN, 208 riferimenti alle abilità **locali**, 16 percorsi, navigazione circolare, nodi scorrevoli, chiusura touch/BOOT e hook PWR/ripresa. Leak scan finale disabilitato perché la scena rimane attiva.
- Il confronto con un profilo non predefinito verifica che classe, capelli, pelle, occhi, volto, oggetti, abilità e spedizione non vengano alterati dalla consultazione, al netto della stessa simulazione temporale attesa.
- I controlli leggono l'alfa dell'immagine realmente espansa per verificare testa e piedi rispetto all'UI. Verificano inoltre rettangoli dei controlli e testi nel cerchio.
- `firmware-build.txt` e `firmware.json`: build ESP-IDF 5.5.1, dimensione/hash del binario locale, save v4 e collaudo fisico esplicitamente non eseguito. CI costruisce il proprio pacchetto esatto dal commit pubblicato.

Le 17 [catture native](../../img/classes/galleria.png) includono otto basi, evoluzioni con etichetta di riferimento familiare, nomi lunghi in inglese, percorso e abilità. Il picco misurato dei buffer immagini del percorso atlante è 620.168 byte, non l'heap totale firmware né una misura di FPS.

La pubblicazione verifica quattro documenti HTML, moduli JavaScript, due immagini native contro i file sorgenti, tre hash firmware e ZIP completo via HTTP. Il risultato effettivo è nel [workflow di build e pubblicazione](https://github.com/ShydanRem/pocketwayfarer/actions/workflows/pages.yml); il file `release.json` dell'installer identifica commit e versione distribuiti.

Questi controlli non misurano precisione fisica del touch, latenza, autonomia, consumo, stabilità prolungata o divertimento. La roadmap mantiene il gate hardware separato.

Nota storica: i log sopra descrivono la consegna originale. La directory delle immagini viene ampliata dalle versioni successive; lo stato corrente e i nuovi conteggi sono nelle [prove R3.1](../advanced-a-evidence/README.md).

Aggiornamento storico: la directory delle immagini è ora ampliata da R4.1; i log sopra restano relativi alla loro consegna. [Stato e prove correnti](../master-a-evidence/README.md).
