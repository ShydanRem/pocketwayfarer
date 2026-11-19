# Pocket Wayfarer

**Un borgo illuminato, una strada da scegliere, un faro da riaccendere.**

Un RPG fantasy HD-2D per **Waveshare ESP32-S3-Touch-AMOLED-1.75**, con schermo rotondo 466×466, touch e tasti. La nuova esperienza **Lanternbound** include cinque stanze, tre eroi, reliquie con effetti distinti e un ritorno al borgo che prepara il viaggio successivo.

![Schermate reali: borgo, combattimento e reliquia](docs/img/lantern/panoramica.png)

**[Installa dal browser](https://shydanrem.github.io/pocketwayfarer/)** · **[Manuale illustrato](docs/GUIDA.md)** · **[Tutta la documentazione](docs/README.md)**

**[Roadmap operativa](docs/ROADMAP.md)** · [Stato del lotto attivo](docs/roadmap/STATUS.md) · [Direzione artistica delle 40 classi](docs/CLASS-ART-BIBLE.md)

> **0.8.0-preview.1** — Nuova interfaccia del personaggio nel Classico. Creazione, scheda eroe, abilità, classi ed equipaggiamento condividono il trattamento dell’archivio. [Prove R5.1](docs/roadmap/classic-ui-evidence/README.md) · [Roadmap](docs/ROADMAP.md)

## Sul Pocket, in pochi passaggi

1. Apri l’installer da **Chrome o Edge su computer** e collega il Pocket con un cavo USB dati.
2. Conferma il modello, premi **Collega e installa** e scegli la porta USB.
3. Segui il dialogo e attendi il completamento. Per aggiornare una build Pocket Wayfarer con lo stesso layout, lascia **Erase device** disattivato.

Il pacchetto include firmware, grafica e font: non servono compilazione, microSD o configurazione Wi-Fi. La pagina verifica i tre binari prima di abilitare l’installazione. La cancellazione completa elimina i salvataggi; prima di cambiare gioco o layout consulta [Backup e aggiornamenti](docs/SALVATAGGI.md).

**Compatibilità:** soltanto Waveshare ESP32-S3-Touch-AMOLED-1.75, CO5300, CST9217, 16 MB flash, 8 MB PSRAM. Schede dal nome simile non sono considerate compatibili automaticamente. [Installazione completa e procedura manuale](docs/INSTALLAZIONE.md).

## Il primo viaggio

**Gioca → Parti → scegli l’assetto → Parti → scegli il cammino.** La guida contestuale insegna attacco, parata e Risonanza mentre giochi; i suggerimenti appresi scompaiono e si possono ripetere dalle opzioni.

| Nella spedizione | La tua scelta |
|---|---|
| Percorsi | Vedi i nemici prima di entrare: un avversario robusto o due più fragili. |
| Combattimento | Bersaglio, attacco a tempo, due abilità e Guardia per recuperare PM e ridurre i danni. |
| Risonanza | Tre parate perfette preparano l’effetto dell’offensiva del tuo assetto. |
| Reliquie | Sei effetti, due proposte per ricompensa; a tre slot confronti e sostituisci, oppure rinunci. |
| Eventi | Focolare per riprendere fiato o Specchio per un patto a costo di PV. |
| Ritorno | Rientri con tutto l’oro; una sconfitta perde `floor(oro / 4)`. Retry senza attesa di stamina. |
| Vittoria | Il Guardiano chiude il viaggio; conservi un progetto equipaggiabile nella run successiva. |

![Sentinella, Piromante, Predone e Guardiano del Faro](docs/img/lantern/assetti-e-guardiano.png)

La Sentinella protegge e prepara, il Piromante applica bruciatura, il Predone sfrutta la Preparazione. Affondi, rinculo, parata perfetta ed effetti distinti rispettano pausa e movimento ridotto. Nel borgo trovi mappa, bestiario, collezione e opzioni IT/EN.

Da **… → Classi** consulti anche le 40 classi classiche, le loro abilità e i percorsi di evoluzione. Le otto basi hanno nuove illustrazioni; le evoluzioni indicano quando mostrano il ritratto della famiglia. L'atlante non cambia il tuo eroe e resta distinto dai tre assetti della spedizione.

Nel Classico, tutte le 40 classi usano i nuovi avatar modulari: razza, pelle, capelli, occhi, volto e oggetti restano quelli del tuo eroe. La tavola seguente mostra il renderer LVGL in uno studio di verifica, non un nuovo luogo giocabile.

![Otto avatar base nel renderer nativo](docs/img/hero_rig/otto-avatar-base.png)

## Controlli essenziali

| Comando | Azione |
|---|---|
| Touch | Bersaglio, menu e timing di attacco/parata. |
| BOOT | Timing alla pressione; durante la scelta in battaglia mette in pausa. Nei menu, indietro al rilascio. |
| II | Pausa anche durante l’anello e la parata. |
| PWR | Salva e spegne/riaccende lo schermo. |

Riprendi conserva la fase e concede 650 ms di preparazione. Timing assistito allarga le finestre del 50%; movimento ridotto mantiene i segnali necessari senza effetti decorativi in movimento. [Regole, eroi e reliquie](docs/GUIDA.md).

## Provare senza dispositivo

Su Linux, con runtime SDL2, CMake e compilatore C/C++:

```sh
bash scripts/play-desktop.sh
```

Mouse = touch, Spazio/Invio = BOOT, Esc = BOOT lungo, P = PWR, F5 = salva/ricarica. È la stessa UI nativa C/LVGL del firmware. Usa `--save build/prova.bin` per un profilo separato. [Requisiti e istruzioni di sviluppo](docs/SVILUPPO.md).

## Il gioco classico resta disponibile

Da **… → Classico** accedi al precedente RPG: creazione dettagliata, 40 classi, equipaggiamento, campi e dungeon, bottega, forgia, missioni e caccia offline. Usa lo stesso profilo migrato; non richiede un downgrade. Il redesign grafico non copre ancora tutte le sue schermate e regioni. Legami nell’RPG e audio non sono funzioni completate della preview.

I difetti dell’audit su incontri interrotti, missioni oro/sociali, morale e bottino a zaino pieno sono coperti da regressioni correnti. [Progettazione classica](docs/RPG-DESIGN.md), [regole implementate e prove](docs/LANTERNBOUND.md).

## Documentazione

| Per… | Documento |
|---|---|
| Installare via USB o manualmente | [Installazione](docs/INSTALLAZIONE.md) |
| Imparare il gioco | [Manuale illustrato](docs/GUIDA.md) |
| Aggiornare, fare backup e ripristinare | [Salvataggi](docs/SALVATAGGI.md) |
| Problemi di browser, porta o schermo | [Risoluzione dei problemi](docs/RISOLUZIONE-PROBLEMI.md) |
| Compilare, verificare o modificare | [Sviluppo](docs/SVILUPPO.md) |
| Gestire sito e pacchetti su GitHub Pages | [Pubblicazione](docs/PUBBLICAZIONE.md) |
| Hardware, memoria e checklist fisica | [Firmware](docs/FIRMWARE.md) |

## Verifiche e provenienza

La suite comprende 55.639 controlli RPG, 1.434 del compagno, 600 spedizioni simulate e 49 catture native Lanternbound con 432 salvataggi validati. Tre ulteriori catture verificano il cerchio con frame da 120 ms. L'atlante aggiunge 80 schede IT/EN e 17 catture, con 208 riferimenti alle abilità e 16 percorsi verificati. Include sanitizer, errori di input/salvataggio, cache, dialoghi, migrazione v1/v2/v3 e confronto del rendering aggiornato con quello completo. [Log e limiti](docs/lantern-evidence/README.md), [prove R1](docs/roadmap/evidence/README.md). Sono verifiche del codice e dei percorsi esercitati, non misure di autonomia o coinvolgimento dei giocatori.

Gli atlas HD-2D originali e i font hanno [sorgenti, prompt e registro](assets/lantern/README.md). [Crediti e licenze dei componenti](THIRD_PARTY.md). L’installazione dal browser si ispira al percorso di [TamaPoke](https://socquique.github.io/TamaPoke/web/) e usa ESP Web Tools; non copia firmware o contenuti del gioco. L’infrastruttura hardware deriva dal lavoro su [PocketD-D](https://github.com/ShydanRem/PocketD-D), senza contenuti o regole D&D.

R2 aggiunge 61.440 combinazioni estetiche, 89.768 composizioni, 147 catture native e prove di continuità/cache con sanitizer. [Prove e limiti R2](docs/roadmap/rig-evidence/README.md) · [Sorgenti del rig](assets/hero-rig/README.md).

Le prime otto avanzate ora hanno anche una propria illustrazione nell’atlante e un costume personale distinto dal ramo fratello.

![Otto avatar avanzati nel renderer nativo](docs/img/hero_rig/otto-avatar-avanzate-a.png)

R3.1 verifica 122.880 combinazioni estetiche e promozioni touch IT/EN con equipaggiamento conservato. [Prove e limiti](docs/roadmap/advanced-a-evidence/README.md).

R3.2 completa Paladino, Sacerdote, Campione, Mistico, Fabbro, Alchimista, Negromante e Stregone.

![Secondo lotto di avatar avanzati, renderer nativo](docs/img/hero_rig/otto-avatar-avanzate-b.png)

Verificati 184.320 combinazioni estetiche, 32 promozioni touch IT/EN e 512 catture native. La cintura decorativa dell’Alchimista conserva il proprio colore cambiando carnagione. [Prove R3.2](docs/roadmap/advanced-b-evidence/README.md).

R4.1 aggiunge Cavaliere del Drago, Signore della Guerra, Arcimago, Cronomante, Signore delle Bestie, Arco della Tempesta, Lama Notturna e Maestro di Lame.

![Prime otto maestre nel renderer nativo](docs/img/hero_rig/otto-avatar-maestre-a.png)

Verificati 245.760 combinazioni estetiche, 1.200 confronti dei rami sotto lo stesso elmo e 16 percorsi touch completi fino alla maestra. La scheda Statistiche mantiene i nomi lunghi dentro il cerchio e separati dall’avatar. [Prove R4.1](docs/roadmap/master-a-evidence/README.md).

R4.2 completa Templare, Ierofante, Gran Maestro, Asceso, Forgiarune, Trasmutatore, Signore della Morte ed Evocavuoto.

![Ultime otto maestre nel renderer nativo](docs/img/hero_rig/otto-avatar-maestre-b.png)

Verificate 307.200 combinazioni estetiche e 1.600 confronti dei rami con lo stesso elmo. I 32 percorsi di promozione in italiano e inglese coprono tutte le maestre e verificano abilità ereditate e salvataggi. [Prove R4.2](docs/roadmap/master-b-evidence/README.md).

![Creazione, Statistiche ed Equipaggiamento nel Classico](docs/img/hero_rig/interfaccia-classico.png)
