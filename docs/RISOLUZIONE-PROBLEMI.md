# Se qualcosa non funziona

Comincia dal sintomo. Per problemi di porta o collegamento non è necessario cancellare ripetutamente la flash.

## Il pulsante di installazione non compare

- Apri la pagina da un computer in un browser con Web Serial, per esempio Chrome o Edge.
- Usa l’indirizzo HTTPS del sito; `file://` non è adatto. Per lo sviluppo locale usa `localhost`.
- Controlla il modello e spunta la conferma: il pulsante compare dopo la verifica dei file.
- Se la pagina segnala firmware mancante o SHA-256 non valida, non tentare di usare un singolo file scaricato altrove. Ricarica e riprova, oppure scarica un pacchetto completo verificabile. Segnala il messaggio esatto.

## La porta USB non compare

1. Prova un altro cavo **dati**: un cavo di sola ricarica può alimentare lo schermo senza creare una porta.
2. Collega direttamente al computer, evitando per la prima prova hub o adattatori.
3. Chiudi monitor seriali, IDE e altre schede del browser connesse al Pocket. Una porta può essere già occupata.
4. Controlla se il sistema operativo vede il dispositivo: Gestione dispositivi su Windows, Informazioni di sistema su macOS, dispositivi USB/seriali su Linux.
5. Su Linux verifica i permessi della porta e l’appartenenza al gruppo seriale previsto dalla distribuzione. Potrebbe essere necessario uscire e rientrare nella sessione.

La procedura di recovery indicata da Waveshare usa **BOOT tenuto premuto durante RESET**: premi e rilascia RESET, poi rilascia BOOT. Individua i pulsanti sulla documentazione della tua esatta revisione; non confondere PWR, che nel gioco controlla lo schermo, con RESET. Se il reset automatico non riesce, segui la [guida ufficiale di flashing](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75/Firmware-Flashing) e il [troubleshooting Waveshare](https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75/blob/main/docs/troubleshooting.md).

## Il cerchio si ferma o il timing sembra in ritardo

La `0.4.0-preview.1` corregge un errore che scartava il tempo dei fotogrammi oltre 100 ms, oltre al giudizio touch basato sul fotogramma precedente e a BOOT giudicato al rilascio. Aggiorna conservando il profilo secondo [SALVATAGGI.md](SALVATAGGI.md). Nei minigiochi premi BOOT quando vuoi giudicare: non serve aspettare il rilascio. Le correzioni sono coperte da regressioni native; la latenza fisica del nuovo build resta da misurare. [Cause e prove](DEVICE-AUDIT-2026-11-17.md).

## La scrittura si interrompe

Tieni aperta la pagina, usa una connessione USB diretta e riprova dopo aver liberato la porta. Con la procedura manuale puoi ridurre il baud rate da `460800` a `115200`. Annota l’errore prima di scegliere un’eventuale cancellazione: per una semplice connessione fallita non è il primo rimedio.

## Installazione conclusa, schermo nero

Premi PWR una volta per escludere che lo schermo sia spento. Se il dispositivo è ancora in modalità download, riavvia senza BOOT premuto. Controlla di aver usato il modello giusto e **tutti e tre** i file del pacchetto, con i loro indirizzi.

Se il problema rimane, acquisisci i log di avvio via seriale e segnala la versione del pacchetto. Non installare binari di schede dal nome simile per tentativi. Questa preview non ha ancora concluso il collaudo fisico, quindi schermo, latenza e alimentazione vanno verificati sul dispositivo.

## Dopo l’aggiornamento vedo una nuova partita

Controlla se era stata selezionata **Erase device**, se provenivi da un altro firmware o se è cambiato il layout delle partizioni. Consulta [salvataggi e ripristino](SALVATAGGI.md). La pagina non conserva una copia del tuo eroe: può essere ripristinato solo da un backup compatibile.

## La parata o l’attacco sembrano difficili

In **… → Opzioni** attiva **Timing assistito**. Puoi anche attivare **Movimento ridotto** e ripetere i suggerimenti. La parata si giudica quando l’onda incontra il cerchio centrale; il tasto BOOT può sostituire il tocco. Le misure di latenza sul Pocket fisico sono ancora da eseguire.

## Non posso cominciare una nuova spedizione

Concludi la caccia o il dungeon classico già attivo. Se la caccia non si raccoglie, libera spazio per gli equipaggiamenti: con zaino pieno il bottino rimane conservato e la riscossione viene rifiutata interamente.

## Segnalare un problema utile

Apri una [issue](https://github.com/ShydanRem/pocketwayfarer/issues) includendo versione, modello esatto, sistema operativo/browser per i problemi USB, passaggi effettuati, messaggio d’errore e foto dello schermo o log di avvio pertinenti. Indica se era una prima installazione, un aggiornamento o un cambio da un altro gioco. Non allegare per default un’immagine completa della flash.
