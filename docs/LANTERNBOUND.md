# Lanternbound — spedizione HD-2D giocabile

Dal 23 novembre 2026, **Servizi → Spedizione extra** apre Lanternbound dal Borgo Lanterna. Il firmware e l’eseguibile Linux usano la stessa UI C11/LVGL. [Roadmap](ROADMAP.md)

![Borgo, combattimento e reliquia](img/lantern/panoramica.png)

## Cosa si può giocare

In Lanternbound **Parti** apre il primo bivio con l’assetto indicato sul pulsante. **Assetto** permette di scegliere fra Sentinella, Piromante e Predone. La spedizione attraversa quattro stanze e il boss. Prima di entrare puoi confrontare i nemici dei due percorsi. La scoperta del bestiario avviene all’incontro. Sei specie ordinarie hanno comportamenti distinti e il Guardiano del Faro usa un attacco pesante e un doppio colpo. [Prove della partenza diretta](roadmap/pacing-evidence/README.md)

Nella stanza evento si sceglie Focolare (25 PV o 12 PM) oppure Specchio (offrire 12 PV, avendone più di 12, per Risonanza piena e 8 PM; rifiutare dà 12 PM). Dopo ogni scontro ordinario vengono proposte due reliquie diverse non possedute. A tre slot si può confrontare e sostituire una delle possedute; si può anche rinunciare alla ricompensa. Le sei reliquie modificano parata, danno ai feriti, guardia, scudo iniziale, bruciatura e cura proseguendo. Dopo il boss si conserva un progetto fra le reliquie della run: dalla collezione si può equipaggiare una reliquia iniziale per le spedizioni successive. Occupa uno dei tre slot. Una vittoria senza reliquie termina senza assegnare un progetto. [Regole e prove dello scambio](RELIC-CHOICES.md).

La quarta stanza offre **Sentiero** o **Bastione**. Il primo costa 0 PV e vale 10 oro. Il Bastione costa 12 PV all’ingresso e, se sconfiggi il Granchio di Marea da 88 PV, vale 25 oro. Una spedizione completa rende **80 oro sul Sentiero** o **95 oro col Bastione**. Rientrare conserva tutto, mentre una sconfitta sottrae `floor(oro / 4)`. L’accredito avviene una volta sola anche al riavvio. **Riprova** ripristina la spedizione conservando profilo, oro, progetti, bestiario e opzioni. Fame, stamina e livello del Classico appartengono al profilo fuori dalla spedizione. [Catture, misure e prove del Bastione](roadmap/bastion-evidence/README.md)

Il riepilogo mostra l’oro accreditato e, dopo la sconfitta, la quota persa. Dopo il Faro distingue un progetto nuovo da uno già noto. Puoi aprire la collezione sul progetto conservato o equipaggiarlo direttamente dal riepilogo e ripartire. Il riavvio ripristina esito e reliquia iniziale. [Catture e prove del rientro](roadmap/return-evidence/README.md)

Nel menu `...` trovi mappa, bestiario, collezione e opzioni. **Borgo** riporta al percorso principale. Concludi eventuali cacce o dungeon in corso per aprire una nuova spedizione. Dopo il riavvio la spedizione si riprende da **Servizi**.

![Tre assetti e boss con grafica propria](img/lantern/assetti-e-guardiano.png)

## Controlli e regole

| Azione | Risultato |
|---|---|
| Toccare un nemico vivo | Cambia bersaglio e barra PV mostrata. |
| Attacco | Apre l’anello; tocco o pressione iniziale di BOOT giudica il timing usando il tempo dell'evento. |
| Abilità | Due mosse: offensiva da 6 PM e supporto da 5 PM; disabilitate se manca la risorsa. |
| Guardia | Recupera 4 PM e dimezza i danni del turno nemico successivo. |
| Parata | Dopo 650 ms di intento, toccare quando l’onda raggiunge il cerchio centrale. |
| Parata perfetta | Una carica di Risonanza, massimo 3. L’offensiva a 3 cariche le consuma per l’effetto dell’assetto. |
| II / BOOT durante la scelta | Pausa; II resta raggiungibile anche durante il timing. |
| Riprendi | Conserva fase e tempo trascorso; 650 ms di preparazione prima della ripresa. |
| PWR / riavvio | Salva la spedizione in pausa; occorre Riprendi. |

L’anello gira in 1400 ms e scade dopo 2800 ms. Le finestre complete Bene/Ottimo/Perfetto sono 220/140/90 ms; il timing assistito le allarga del 50%. Il danno usa moltiplicatori 0,4/1/1,25/1,5 senza secondo critico automatico. La parata lascia passare 100/70/45/20% prima di guardia e scudi. Movimento ridotto ferma il ciclo di pose ambientali, mantenendo i segnali necessari al timing.

Sentinella: 120 PV/24 PM, Baluardo aggiunge 30 scudo e cura 6 PV; Risonanza aggiunge scudo. Piromante: 96/36, Incendio brucia per due turni; Risonanza potenzia la bruciatura. Predone: 104/28, Preparazione aggiunge scudo e danno al prossimo colpo; Risonanza aggiunge un colpo. Le offensive da 6 PM aumentano del 50% il danno del timing. Il boss ha 176 PV.

## Grafica e limiti del trattamento

Sono importate **61 immagini runtime**. Due fondali, tre assetti a sette pose, sette creature a quattro pose e sei reliquie, più quattro fotogrammi duplicati della Sentinella per compatibilità con la tabella attori. I tre assetti condividono la griglia 88×88. Ogni assetto usa la propria palette e mantiene lo stesso piano dei piedi fra le pose.

Fondali nativi 466×466 a 1×; attori e reliquie su griglia 88×88 con scala intera. Nel combattimento eroe, avversario singolo e boss usano 2×, le coppie 1× con aree di selezione separate. Il fondale lascia una fascia quieta dietro le sagome, con ombre di contatto LVGL. Profondità e luce dei fondali sono dipinte, senza renderer 3D o illuminazione dinamica. Controlli, testi, font e barre sono oggetti LVGL effettivi. I font DejaVu sono rasterizzati a 20/22 px con accenti italiani. [Ricerca, confronto e limiti della revisione](ART-CLARITY.md). Sorgenti, prompt, licenze, registro e hash: [assets/lantern](../assets/lantern/README.md).

I segnali essenziali sono visivi. Il percorso audio dell’hardware resta da verificare.

## Prima spedizione e feedback

La prima lotta presenta una sola indicazione per volta: Attacco, arco dorato, intento e onda di parata. Un timing almeno Bene insegna l’azione; un miss mantiene il suggerimento. Guardia e abilità si apprendono usandole. Il suggerimento di Risonanza è confermabile con OK; a tre cariche sono evidenziati Abilità e poi il colpo offensivo. Nessun comando è bloccato dalla guida. L’apprendimento resta fra retry e riavvii; `Opzioni` consente di nascondere i suggerimenti o ripeterli.

La posa di carica resta visibile mentre giri sull’anello. Al tocco segue il contatto, poi il recupero. In difesa si vedono l’ingresso in guardia e la posa tenuta durante l’onda. Taglio dorato della Sentinella, brace circolare del Piromante e doppio taglio del Predone accompagnano il colpo. Parata perfetta, scudo, bruciatura e Risonanza hanno messaggi distinti. Movimento ridotto usa pose fisse di carica, contatto e guardia, con i segnali di timing visibili. [Pose e verifica R7.1](roadmap/combat-poses-evidence/README.md)

![Guida, parata e Risonanza nella UI nativa](img/lantern/guida-e-risonanza.png)

![Effetti dei tre assetti](img/lantern/impatto-dei-tre-eroi.png)

![Carica, impatto e parata in tre combattimenti sul Bastione](img/lantern/pose-lanternbound.png)

Gli effetti usano il tempo della fase salvata e si fermano con pausa/PWR. L’input dietro la pausa è escluso anche ai bordi delle immagini. I controlli verificano il contenimento dei testi nei pulsanti e confrontano byte per byte il framebuffer incrementale con un ridisegno completo. [Verifiche della guida contestuale](ONBOARDING-FEEDBACK.md)

## Correzioni dell’audit e salvataggi

Dal formato **v4** si conservano le lezioni apprese, l’attore del feedback e gli effetti. Il formato **v5** aggiunge i progressi di classe e legge v1–v4. Conserva progetti, bestiario, oro, stato della spedizione e transazione del percorso principale. [Formati, aggiornamento e ripristino](SALVATAGGI.md)

| Problema della baseline | Comportamento attuale verificato |
|---|---|
| Battaglia classica interrotta può saltare stanze o bloccare il boss | Un riavvio completo ripristina lo snapshot precedente all’ingresso: consumabili, PV/PM, stamina, RNG e nodo padre. L’incontro viene riproposto. Non riprende il timing classico a metà azione. |
| Esito già pagato / scale persi al riavvio | Ricostruisce la navigazione pendente e non paga due volte. Migrazione v1/v2 recupera anche gli incontri orfani. |
| PWR durante timing classico | Congela il tempo; al risveglio concede 650 ms e blocca input anticipati. |
| Morale diverso fra aggiornamenti frequenti e offline | Stesso risultato per minuto o ora, usando confini temporali assoluti. |
| Piccoli premi non contano per missioni oro | Accredito centralizzato con resto modulo 50 persistente e saturazione sicura. |
| Missioni sociali non completabili | Escluse dalla generazione e sostituite nei salvataggi con obiettivi disponibili. |
| Caccia perde equipaggiamenti con zaino pieno | Raccolta interamente rifiutata senza mutazioni; bottino conservato finché si libera spazio, con messaggio in UI. |

Le regressioni correnti sono in [test_reliability.c](../firmware/rpg/host/test_reliability.c). L’[audit](audit/REVIEW-2026-11-16.md) e le sue sonde rimangono documenti storici della baseline `df85308`.

L'[audit del 17 novembre](DEVICE-AUDIT-2026-11-17.md) corregge inoltre il cerchio fermo con frame oltre 100 ms, giudizio touch arretrato e BOOT al rilascio, ripresa del clock, input di risveglio, sovrapposizione dei bersagli, cache immagini, toast pendenti, dialoghi e retry del salvataggio. Le finestre e le regole di danno restano quelle indicate sopra.

## Verifiche eseguite

| Verifica | Evidenza |
|---|---|
| Asset | Importazione deterministica di 61 immagini e due font, con originali e prompt conservati. |
| Core preesistente | 55.639 controlli RPG e 1.434 del compagno con ASan/UBSan. |
| Spedizione | **600 partite simulate**, 52.485 snapshot salva/ripristina e 51.509 transizioni. 288 run di base, 288 con tutti i progetti iniziali e politiche rinuncia/sostituzione (6 × 3 × 8 × 2), 24 vittorie senza reliquie. Verificate le venti combinazioni di tre slot. Incontrate tutte le sette specie incluso boss. |
| Affidabilità | Regressioni oro/morale/missioni/caccia/transazione/navigazione e migrazione v1/v2/v3 superate. |
| UI nativa | **73 catture e 620 salvataggi validati**. Touch/BOOT, cinque stanze, ripartenza, tre assetti, appoggio delle pose, pausa e movimento ridotto in IT/EN. |
| Regressioni dispositivo/UI | Tre catture del cerchio con frame da 120 ms; touch/BOOT a tempo dell'evento; scadenza, salvataggio lento/fallito, 20 s di schermo spento; 220 immagini stabili, 44 dialoghi e 42 specie di gemme. Sono scenari simulati sullo stesso codice nativo. |
| Sanitizer UI | LVGL e UI compilati/eseguiti con ASan/UBSan senza errori rilevati. Leak scan finale disattivato perché l’app mantiene la scena attiva. Buffer immagini controllati anche su ricostruzioni ripetute del borgo. |
| Memoria immagini host | Picco dei buffer durante il percorso nativo Lanternbound in [misure R7.1](roadmap/combat-poses-evidence/README.md). |
| Desktop interattivo | Avvio, percorso, guida, timing e PWR/ripresa osservati nella precedente preview; migrazione desktop v3 verificata con SDL dummy. La nuova revisione ha catture e input LVGL automatici, senza una nuova prova manuale completa. |
| Firmware | Build ESP32-S3 riuscita con ESP-IDF 5.5.1. Il flash sul Pocket resta da verificare. [Dimensione e SHA-256](roadmap/combat-poses-evidence/firmware.json) |

Log in [lantern-evidence](lantern-evidence/). La strategia automatica usa offensive quando possibile e attacchi base altrimenti, senza riempire risorse fra incontri salvo l’evento. Senza reliquia iniziale, con timing sempre Bene, vincono 20/24 Sentinelle, 16/24 Piromanti e 15/24 Predoni; sempre Ottimo/Perfetto 24/24, sempre Mancato 0/24. Le 288 run con progetto iniziale usano Ottimo, le 24 senza reliquie Perfetto. Questa è taratura del modello, non una misura del divertimento.

**Game Maker AI**, nella precedente verifica del perimetro completo: **Game brief score: 95/100**, con avviso euristico sugli aggettivi. È un controllo del testo. Il modello di ondate resta `incomplete` per questo RPG; non si inventano velocità di salto/fuoco né si adotta il suo punteggio generico. Output e brief: [implementation-game-maker.json](audit/implementation-game-maker.json), [implementation-brief.en.md](audit/implementation-brief.en.md). Il flag mobile è soltanto il proxy touch dello schema, non un porting telefono. L’incremento corrente usa [Game Fix Prompt e criteri dedicati](ONBOARDING-FEEDBACK.md), senza interpretare un controllo del brief come prova della correzione. Flussi Design Game Studio applicati a revisione, sistemi, direzione artistica, asset, feedback, playtest e preparazione della build. La CLI `game-dev` non è disponibile in questo ambiente; asset prodotti con ImageGen e importatore locale, senza job di provider a pagamento.

## Giocare sul desktop Linux

Richiede un desktop grafico Linux, libreria runtime SDL2, compilatore C/C++, CMake, Python 3 con Pillow/NumPy. Dalla radice:

```sh
bash scripts/play-desktop.sh
```

Lo script verifica gli header SDL2 2.32.4 ufficiali tramite SHA-256, li mette in `build/desktop-deps/`, compila e apre la UI. Non installa librerie di sistema. Il gioco salva per default in `build/desktop-save.bin`; `--save percorso.bin` consente un profilo distinto. Il percorso padre deve esistere.

| Desktop | Dispositivo |
|---|---|
| Click sinistro | Touch |
| Spazio / Invio | BOOT breve |
| Esc | BOOT lungo |
| P | PWR, spegni/accendi |
| F5 | Salva e ricarica |

La finestra mantiene una superficie logica 466×466 con bande laterali: ridimensionarla non sposta i bersagli del mouse. La chiusura salva il profilo caricato. Non è un porting browser.

## Compilare e verificare

```sh
bash scripts/check.sh
```

Nel workspace gestito, CMake è anche in `/workspace/.pocketwayfarer-tools/python/bin`; lo script desktop lo rileva automaticamente. Per gli altri comandi:

```sh
export PATH=/workspace/.pocketwayfarer-tools/python/bin:$PATH
```

Per la verifica UI con sanitizer, usare una cartella catture separata:

```sh
cmake -S firmware/host -B build/host/asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer'
cmake --build build/host/asan --target preview_lantern -j4
mkdir -p build/host/shots-lantern-asan
ASAN_OPTIONS=detect_leaks=0 build/host/asan/preview_lantern build/host/shots-lantern-asan
python3 scripts/previews.py build/host/shots-lantern-asan docs/img/lantern
```

Con ESP-IDF 5.5.x attivato, dalla cartella `firmware`:

```sh
idf.py -B ../build/firmware-hd2d \
  -D SDKCONFIG="$PWD/../build/firmware-hd2d/sdkconfig" build
```

Il build è in `build/firmware-hd2d/`; `pocketwayfarer.bin` è l’app, non un’immagine flash completa. Su dispositivo collegato, usando la stessa configurazione:

```sh
idf.py -B ../build/firmware-hd2d -p /dev/ttyACM0 flash monitor
```

## Prossima verifica di prodotto

Collaudo sul dispositivo: leggibilità al diametro reale, tocco/tasti, frame time e latenza in attacco/parata, spegnimento/ripresa, memoria totale e consumi. Poi cinque persone nuove: comprensione del timing, uso di guardia/abilità e decisione spontanea di iniziare un’altra spedizione. Durata 3–5 minuti e risposta sotto 100 ms restano obiettivi da misurare. Nessuna misura fisica o di coinvolgimento viene dedotta dai test automatici.
