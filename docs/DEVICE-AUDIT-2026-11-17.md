# Audit di timing, input e UI — 17 novembre 2026

Baseline: `1abf1da3eefb6ae8100ab148ae35dfc56314d90c`, versione `0.3.0-preview.2`. Il giocatore ha segnalato un cerchio di combattimento che non si stringeva e input imprecisi sul Pocket. I difetti sotto sono stati riprodotti nel codice e nella UI LVGL; non sono misure effettuate sul dispositivo.

## Cause e correzioni

| Difetto riprodotto | Correzione e regressione |
|---|---|
| Ogni fotogramma oltre 100 ms azzerava il delta di Lanternbound. Con 30 aggiornamenti da 120 ms il cerchio restava largo 430 px e il tempo di parata a zero. | Il tempo reale trascorso viene consumato anche nei frame lenti. Il test da 120 ms fa avanzare e terminare la parata. Tre catture mostrano 430 → 262 → 162 px a 0/600/960 ms. |
| Il touch usava il tempo dell'ultimo aggiornamento, mentre BOOT fisico giudicava al rilascio. Un evento a 700 ms poteva usare i precedenti 620 ms. | Touch e BOOT aggiornano il tempo al momento dell'evento. Il timing fisico usa la pressione iniziale; il rilascio non genera una seconda azione. Entrambi giudicano Perfetto nella regressione 620+80 ms. |
| Un evento arrivato dopo la scadenza poteva agire sulla fase successiva. | La fase viene aggiornata e l'evento scaduto consumato. Nessun attacco o pausa involontaria nella fase seguente. |
| Salvataggio sincrono alla transizione poteva sottrarre tempo alla fase appena iniziata. | L'origine viene aggiornata dopo la transizione; la regressione inietta 80 ms di salvataggio senza accorciare la nuova fase. |
| Il clock LVGL si fermava con lo schermo spento; al risveglio il recupero del tempo poteva far scadere il timing classico. | Clock monotono basato su `esp_timer`, callback di ripresa prima di timer/rendering e 650 ms di preparazione con input anticipati esclusi. |
| Il tasto di pausa non aggiornava l'inattività; una pausa dopo 45 secondi poteva spegnere subito il display. | I tasti registrano attività. Percorso, evento e ricompensa consentono invece lo spegnimento per inattività. |
| Touch o tasto che svegliavano il dispositivo potevano diventare comandi di gioco; errori I²C potevano simulare un rilascio. | Attesa di un rilascio valido al risveglio, debounce del rilascio fisico e conservazione dello stato touch quando una lettura non è disponibile. |
| I rettangoli trasparenti dei due nemici si sovrapponevano. | Layout condiviso fra immagini, effetti e selezione; test touch di entrambi e del punto prima intercettato. |
| La cache di 160 immagini poteva esaurirsi con 220 combinazioni di icone e riutilizzare un descrittore ancora vivo. | Descrittori stabili per tutta la scena, liberati dopo la cancellazione degli oggetti. Test con 220 immagini e ripetuti cambi scena. |
| Un toast cancellato dal cambio scena lasciava un puntatore pendente. | La callback di cancellazione azzera l'identità; venti cicli verificano che un nuovo toast non cancelli un oggetto estraneo. |
| Dialoghi lunghi avevano pulsanti fuori dal cerchio o contenuti iniziali irraggiungibili. Gemme limitate alle prime sei specie. | Pannello verticale scorrevole, allineamento iniziale, pulsanti da 56 px, Chiudi esplicito e tutte le 42 specie. Verificati 44 dialoghi, 176 azioni raggiunte e 67 click esterni rifiutati; ultima gemma raggiunta con swipe e incastonata. |
| Un commit NVS fallito poteva cancellare lo stato “da salvare”. | Il backend restituisce l'esito; il gioco mantiene i dati sporchi e riprova dopo tre secondi. Test di errore iniettato e successivo recupero. |

![Tre istanti reali della parata](img/device/galleria.png)

## Riproduzione e prove

La regressione portabile di campionamento è [test_device_input.c](../firmware/host/test_device_input.c). Le prove con la UI effettiva sono [test_device_ui.c](../firmware/rpg/host/test_device_ui.c) e [preview_lantern.c](../firmware/rpg/host/preview_lantern.c). Entrambe usano il codice del prodotto, non una riscrittura del combattimento. `scripts/check.sh` esegue i test; [SVILUPPO.md](SVILUPPO.md) descrive i prerequisiti.

Log correnti in [lantern-evidence](lantern-evidence/README.md), inclusi test UI con AddressSanitizer/UBSan e build ESP-IDF. Salvataggi nel formato **v4** e partizioni invariati. Finestre di giudizio e danni non sono stati allargati per mascherare l'errore.

## Limiti e prossimo collaudo fisico

Queste prove riproducono frame lenti, ordinamento degli eventi, errori di salvataggio e uso della UI. Non certificano latenza del pannello, frequenza effettiva dei frame, precisione elettrica del touch, consumi o stabilità di una lunga sessione sul Pocket. La build emette sul monitor seriale, ogni dieci secondi, campioni, media, massimo e conteggio dei cicli attivi oltre 100 ms: sono tempi del ciclo, non FPS dichiarati.

Sul dispositivo occorre ancora verificare: parata senza congelamenti in più incontri; touch e BOOT alla pressione; pausa/PWR/risveglio senza comandi involontari; selezione separata dei nemici; leggibilità e risposta del touch sul bordo. I difetti non ricordati dal giocatore non sono considerati automaticamente risolti da questo audit.
