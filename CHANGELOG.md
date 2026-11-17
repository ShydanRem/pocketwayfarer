# Note di versione

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
