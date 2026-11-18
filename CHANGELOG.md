# Note di versione

## 0.6.0-preview.2 — 18 novembre 2026

- Completato R3: tutte le sedici avanzate hanno costumi modulari, copricapi default e illustrazioni specifiche. Il secondo lotto aggiunge Paladino, Sacerdote, Campione, Mistico, Fabbro, Alchimista, Negromante e Stregone. Copertura totale: **24/40 avatar e illustrazioni**, con le sedici maestre ancora in fallback esplicito.
- Aspetto, oggetti realmente equipaggiati, regole delle classi e salvataggi v4 conservati. I rami restano distinguibili anche sotto lo stesso elmo, senza arma o mano secondaria.
- Corretta la cintura dell'Alchimista: alcune ombre dell'ampolla viola ereditavano il colore della pelle. La palette ora distingue colore e ruolo semantico; una regione decorativa resta fissa senza alterare il PNG sorgente. Regressione di 1.444 confronti; l'asset precedente produce 28 errori attesi.
- Verificati 184.320 combinazioni estetiche, 270.909 composizioni, 800 confronti dei fratelli e 512 catture native: 415 rig, 49 atlante, 48 promozioni. Trentadue promozioni touch IT/EN, 64 soglie respinte e 32 roundtrip dei salvataggi.
- Firmware locale 3,42 MiB su 6 MiB; documentazione e installer aggiornati. Pose ancora preliminari e collaudo fisico aperto. [Prove R3.2](docs/roadmap/advanced-b-evidence/README.md).

## 0.6.0-preview.1 — 18 novembre 2026

- R3.1: Cavaliere, Berserker, Elementalista, Arcanista, Ramingo, Cecchino, Assassino e Duellante hanno costumi modulari e illustrazioni specifiche. Copertura reale: 16/40 avatar e 16/40 illustrazioni; le altre 24 classi restano esplicitamente nel trattamento precedente.
- Mapping per ID esatto con metadata famiglia/grado; aspetto, equipaggiamento e salvataggi v4 conservati. Rami fratelli distinguibili sotto lo stesso elmo, senza dipendere dal colore o dall'arma.
- Verifiche dedicate: 122.880 combinazioni estetiche, 180.336 composizioni, 400 confronti di sagoma dei fratelli; 281 catture del rig, 33 dell'atlante e 24 delle promozioni. Sedici promozioni touch IT/EN, 32 soglie respinte e 16 salvataggi riletti.
- Corretto il footer della battaglia che tagliava PV/PM a tre cifre: due righe leggibili, con 71.556 pixel di glifi verificati dentro il cerchio e sotto i piedi. Layout nemici/comandi separato anche per i costumi legacy, la cui barra del nemico singolo attraversava l'eroe.
- Suite, sanitizer e build ESP32-S3 verificati; pacchetto e documentazione aggiornati. Pose ancora parametriche preliminari e collaudo fisico aperto. [Prove R3.1](docs/roadmap/advanced-a-evidence/README.md).

## 0.5.0-preview.1 — 18 novembre 2026

- Nuovi avatar modulari per tutte le otto classi base classiche: costumi distinti, quattro razze, cinque carnagioni, sei capelli/otto colori, occhi e variante del volto conservati. Mapping per ID classe esatto, senza cambiare save v4 o regole delle classi.
- Armi, mano secondaria e copricapi coerenti con il rig e con gli oggetti realmente posseduti; cinque fasce di materiale. Gli altri slot restano statistici. Le 32 evoluzioni mantengono il renderer precedente fino ai lotti R3/R4.
- Pose parametriche separate dalle espressioni: riposo/respiro, attacco, guardia e colpito. Movimento ridotto rispettato. Non sono ancora animazioni finali con maschere dedicate degli arti.
- Cache con descrittori stabili e proprietà legata ai widget; nessuna crescita continua durante 1.000 cambi d'aspetto. Layout aggiornato in creazione, borgo, equipaggiamento e battaglia; livelli dei nemici e volto dell'Occultista non vengono coperti da comandi/falce.
- 61.440 combinazioni estetiche, 89.768 composizioni e 147 catture native verificate, con controlli di continuità, input sui bersagli, salvataggi, pausa/ripresa e confronto del rendering. Dettagli e limiti nelle [prove R2](docs/roadmap/rig-evidence/README.md). Collaudo fisico ancora aperto.

## 0.4.1-preview.1 — 17 novembre 2026

- Atlante consultabile da Borgo → … → Classi: 40 schede IT/EN, abilità proprie delle classi e otto alberi di evoluzione con nodi selezionabili.
- Otto nuove illustrazioni rappresentative delle classi base. Le 32 evoluzioni indicano quando mostrano il ritratto della famiglia; l'avatar personale mantiene aspetto ed equipaggiamento esistenti.
- Dettagli scorrevoli con Chiudi sempre raggiungibile, ritorno con BOOT e consultazione senza cambiare classe, assegnare abilità o ricompense.
- Roadmap operativa, registro delle 40 classi e direzione artistica per il futuro rig modulare. Pagina Roadmap sul sito e immagini native nel manuale.
- Verifiche dedicate per 80 schede bilingui, 208 riferimenti alle abilità locali e 16 percorsi; 17 catture native e profilo personalizzato invariato. Salvataggi v4 e partizioni invariati; collaudo fisico ancora aperto.

## 0.4.0-preview.1 — 17 novembre 2026

- Nuove sorgenti per borgo, arena, tre eroi e sette specie nemiche; composizione meno rumorosa dietro le sagome, palette condivise fra pose, fondali nativi 466×466 e ombre di contatto. Ricerca e confronto documentati in `docs/ART-CLARITY.md`.
- Corretto il cerchio bloccato con aggiornamenti oltre 100 ms. Touch e BOOT giudicano il tempo dell'evento; BOOT agisce alla pressione. Input scaduti e preparazione non attivano la fase seguente.
- Clock continuo, ripresa prima dei timer, protezione dagli input di risveglio e debounce del rilascio. Salvataggi lenti non consumano la nuova fase; commit falliti mantengono lo stato da salvare e vengono ritentati.
- Bersagli separati; cache con descrittori stabili; correzione dei toast pendenti. Dialoghi scorrevoli entro il display, pulsante Chiudi e tutte le 42 specie di gemme.
- 49 catture native con 432 salvataggi e tre nuove catture del cerchio; regressioni e sanitizer superati. Formato salvataggi v4 e partizioni invariati. Il nuovo firmware deve ancora essere collaudato fisicamente.

## 0.3.0-preview.2 — 17 novembre 2026

- Le ricompense continuano a offrire due reliquie anche a tre slot: confronto degli effetti e sostituzione esplicita di una reliquia posseduta.
- È possibile rinunciare alla ricompensa. Indietro/BOOT annullano il confronto; riavvio e PWR non rimuovono oggetti non confermati.
- Vincere senza reliquie conclude regolarmente il viaggio, accredita l’oro una sola volta e non assegna un progetto inesistente.
- Salvataggi invariati nel formato v4; guida illustrata e sito aggiornati. Il collaudo fisico resta da completare.

## 0.3.0-preview.1 — 16 novembre 2026

### Lanternbound

- Nuova spedizione HD-2D nella UI C/LVGL: tre assetti con grafica distinta, cinque stanze, sei specie ordinarie e Guardiano del Faro originale.
- Timing di attacco/parata, due abilità per assetto, Guardia, Risonanza e sei reliquie combinabili. Due eventi a scelta e progetti equipaggiabili alla partenza.
- Borgo con mappa, bestiario, collezione, guida e opzioni IT/EN. Suggerimenti contestuali persistenti e ripetibili; effetti dei colpi, rinculo, parata perfetta e movimento ridotto.
- Ripresa completa in pausa della spedizione; retry e accredito dell’oro protetto dalla doppia riscossione.

### Affidabilità

- Salvataggio v4 compatibile in lettura con v1/v2/v3.
- Recupero delle battaglie classiche interrotte dallo snapshot precedente all’ingresso e ricostruzione degli esiti/navigazione pendenti.
- Correzioni per morale, missioni oro, obiettivi sociali indisponibili e raccolta della caccia con zaino pieno.

### Installazione e documentazione

- Sito statico per GitHub Pages con installer USB basato su ESP Web Tools, distribuito localmente e verificato tramite hash.
- Pacchetto completo derivato dalla build ESP-IDF, controlli di chip/versione/partizioni, ZIP e SHA-256. Nessuna microSD richiesta.
- Manuale illustrato in italiano, procedure di installazione, aggiornamento, backup, recupero e sviluppo.
- Workflow che costruisce dal medesimo commit sito e firmware. L’attivazione iniziale di Pages resta un’impostazione del proprietario del repository.

### Stato

Preview verificata su PC e compilata con ESP-IDF 5.5.1. Il collaudo fisico, la misura di consumi/latenza e le prove con giocatori non sono ancora completati. Nessuna compatibilità automatica con altre schede ESP32-S3 o varianti dal nome simile.
