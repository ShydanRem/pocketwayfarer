# Installa Pocket Wayfarer sul Pocket

Il percorso più semplice è **USB → browser → installa**. Non servono Arduino IDE, ESP-IDF, una microSD o un pacchetto di sprite separato. L’ispirazione per questo percorso è l’[installer di TamaPoke](https://socquique.github.io/TamaPoke/web/); il firmware installato qui è Pocket Wayfarer e usa i propri file e le proprie partizioni.

![La schermata che troverai al primo avvio](img/lantern/01-titolo.png)

## Prima di collegarlo

Occorrono un computer, una connessione Internet, un cavo USB che trasmetta dati e il modello **Waveshare ESP32-S3-Touch-AMOLED-1.75**: display rotondo 466×466 CO5300, touch CST9217, ESP32-S3 con 16 MB flash e 8 MB PSRAM.

La build non dichiara compatibilità con modelli dal nome simile, come 1.75C, 1.8 o 2.06. Il riconoscimento di ESP32-S3 da parte dell’installer identifica il chip, non tutti i componenti della scheda: verifica il nome sul prodotto.

Per usare la porta USB dal browser servono Web Serial e una pagina HTTPS. Chrome o Edge su computer sono il percorso consigliato; il sito rileva le funzioni disponibili e mostra un’alternativa quando mancano. Safari, altri browser o un telefono possono comunque leggere il manuale e scaricare il pacchetto.

Se sul Pocket c’è già un gioco che vuoi conservare, leggi prima [Backup e cambio di firmware](SALVATAGGI.md). I profili di TamaPoke o di altri giochi non vengono convertiti in un eroe Pocket Wayfarer.

## Installazione dal browser

1. Apri l’[installer GitHub Pages](https://shydanrem.github.io/pocketwayfarer/) da Chrome o Edge.
2. Collega il Pocket direttamente al computer con un cavo USB dati. Chiudi monitor seriali, Arduino IDE e altre pagine che stanno usando la stessa porta.
3. Nella sezione **Installa**, controlla versione e modello e spunta la conferma della scheda. Il sito controlla dimensioni e SHA-256 dei tre file prima di abilitare il pulsante.
4. Premi **Collega e installa** e scegli la porta del Pocket nella finestra del browser. Se non compare, passa alla [guida USB](RISOLUZIONE-PROBLEMI.md#la-porta-usb-non-compare).
5. Nel dialogo di ESP Web Tools scegli l’installazione. Quando viene proposto **Erase device**, lascia la casella disattivata per aggiornare Pocket Wayfarer mantenendo lo stesso layout. Se scegli di cancellare completamente il dispositivo, perdi i salvataggi presenti.
6. Conferma e attendi il completamento della scrittura. Lascia collegato il cavo e mantieni aperta la pagina fino alla conferma finale.
7. Il Pocket normalmente riparte al termine. Se rimane in modalità download, effettua un reset secondo la procedura della tua scheda, senza tenere premuto BOOT.

La prima installazione di un firmware diverso può richiedere una cancellazione completa per ripartire da una memoria pulita: esegui prima un backup se vuoi poter tornare indietro. Per un aggiornamento di Pocket Wayfarer con le stesse partizioni, il pacchetto ordinario non scrive nella partizione dei salvataggi.

Il dialogo di ESP Web Tools può mostrare pulsanti in inglese, come **Install**, **Next** ed **Erase device**. Il gioco permette di scegliere italiano o inglese.

## Primo avvio

Un Pocket nuovo mostra il titolo. Tocca **Gioca** per partire subito, oppure **Personalizza** per creare l’eroe nel percorso classico. Nel borgo scegli **Parti**, seleziona l’assetto e poi il cammino.

Se stai aggiornando un profilo esistente, il gioco può aprire direttamente il borgo, una spedizione in pausa o il dungeon classico recuperato. Non è necessario creare un nuovo eroe.

![Borgo Lanterna](img/lantern/02-borgo.png)

I primi combattimenti includono suggerimenti contestuali. Il [manuale illustrato](GUIDA.md) spiega attacco, parata, Guardia e Risonanza.

## Installazione manuale

Scarica **il pacchetto completo** dall’installer ed estrailo in una cartella. Dentro trovi `bootloader.bin`, `partitions.bin`, `app.bin`, manifest, metadati, istruzioni e `SHA256SUMS.txt`. Non usare soltanto `app.bin` per una prima installazione.

Con Python ed esptool 4.x, dalla cartella estratta, sostituisci `PORTA` con la porta reale (`COM5`, `/dev/ttyACM0` o `/dev/cu.usbmodem…`):

```sh
python -m pip install esptool==4.11.0
python -m esptool --chip esp32s3 --port PORTA --baud 460800 write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader.bin 0x8000 partitions.bin 0x10000 app.bin
```

Il comando non richiede una cancellazione completa. Le sintassi di esptool 5 possono usare nomi con trattini, per esempio `write-flash`: consulta la documentazione della versione che hai installato.

Per controllare i file, su Linux/macOS con `sha256sum` disponibile:

```sh
sha256sum -c SHA256SUMS.txt
```

Su macOS è disponibile anche `shasum -a 256`; su PowerShell puoi usare `Get-FileHash -Algorithm SHA256`. Confronta i risultati con il file incluso nel pacchetto.

## Cosa è verificato

La pagina e il pacchetto controllano progetto, versione nel binario, chip, impostazioni flash, partizioni, dimensioni e hash. Gli indirizzi vengono dalla build ESP-IDF: `0x0`, `0x8000`, `0x10000`; non vengono copiati da una ricetta Arduino. Non è incluso un `boot_app0.bin`, perché questo progetto usa una singola partizione `factory`, senza OTA.

La build corrente è una preview verificata su PC; la scrittura e il successivo funzionamento su un Pocket fisico restano da collaudare. Il sito dichiara esplicitamente questo limite.

Fonti tecniche: [ESP Web Tools](https://esphome.github.io/esp-web-tools/), [Waveshare: flashing e recovery](https://docs.waveshare.com/ESP32-S3-Touch-AMOLED-1.75/Firmware-Flashing), [esptool](https://docs.espressif.com/projects/esptool/en/latest/esp32s3/).
