# Aggiornamenti, backup e ripristino

Il profilo è nella flash del Pocket, non nel browser. Pocket Wayfarer salva in una partizione dedicata, chiamata `wf_nvs`; la pagina di installazione non riceve né carica il tuo eroe su un server.

## Aggiornare Pocket Wayfarer

1. Metti il gioco in pausa e usa PWR per far salvare il profilo prima di procedere.
2. Se il profilo è importante, esegui il backup descritto sotto.
3. Nell’installer usa il pacchetto completo e lascia **Erase device** disattivato.
4. Al termine, riavvia normalmente. Le spedizioni interrotte ripartono in pausa.

Questa regola vale per le build Pocket Wayfarer con il layout corrente. Il pacchetto scrive bootloader, tabella partizioni e applicazione; non contiene file destinati a `wf_nvs`, a `0x610000` per `0x40000` byte. La build controlla che gli intervalli scritti non si sovrappongano a quella partizione.

**Una cancellazione completa elimina anche i salvataggi.** L’opzione viene proposta dal dialogo e resta una scelta dell’utente; il sito non la attiva automaticamente.

## Cambiare gioco, per esempio da TamaPoke

Installare Pocket Wayfarer sostituisce il firmware precedente e la sua tabella partizioni. Un profilo di un altro gioco non è compatibile con Pocket Wayfarer. Conservare soltanto la nostra `wf_nvs` non protegge i dati di un firmware che usa un layout diverso.

Per poter tornare al gioco precedente, conserva **un backup completo della flash da 16 MB**, oltre ai file eventualmente presenti sulla sua microSD e alle istruzioni di ripristino di quel progetto. La microSD non è usata da questa build di Pocket Wayfarer.

## Preparare esptool

I comandi seguenti usano esptool 4.x e non vengono eseguiti dalla pagina web. Installa Python sul computer e, in un ambiente Python adatto:

```sh
python -m pip install esptool==4.11.0
```

Sostituisci `PORTA` con la porta reale. Chiudi gli altri programmi che la stanno usando. Se la connessione non riesce, segui [la guida USB/recovery](RISOLUZIONE-PROBLEMI.md#la-porta-usb-non-compare) prima di cancellare dati.

### Backup del solo profilo Pocket Wayfarer

Per il layout corrente:

```sh
python -m esptool --chip esp32s3 --port PORTA read_flash 0x610000 0x40000 pocketwayfarer-save.bin
```

Il risultato deve avere **262.144 byte**: è l’intera partizione, non il solo blob dell’eroe. Contiene i dati di gioco delle build che usano questa partizione. Conserva una copia separata e annota versione del firmware e data.

### Backup completo prima di cambiare firmware

```sh
python -m esptool --chip esp32s3 --port PORTA read_flash 0x0 0x1000000 pocket-completo.bin
```

Il risultato deve avere **16.777.216 byte**. Include firmware, partizioni e dati; può contenere anche configurazioni del gioco precedente. Conservalo localmente e non allegarlo automaticamente a una segnalazione pubblica.

## Ripristinare

Per ripristinare il solo profilo, usa lo stesso dispositivo e una build Pocket Wayfarer compatibile con quel layout e formato. Prima conserva un backup dello stato attuale. Il seguente comando **sostituisce i dati correnti nella partizione di gioco**:

```sh
python -m esptool --chip esp32s3 --port PORTA write_flash 0x610000 pocketwayfarer-save.bin
```

Un backup completo si riscrive a `0x0`, seguendo le impostazioni flash e le istruzioni del firmware da cui proviene. Ripristina anche il vecchio firmware e il vecchio layout: non è un’importazione di progressi dentro il gioco nuovo.

## Formati e riavvio

La versione corrente scrive il formato **v4** e legge v1, v2 e v3. Il blob RPG v4 è di 2808 byte, header con CRC compreso; il backup della partizione è più grande perché conserva l’intera NVS. Non provare a scrivere il blob da 2808 byte come se fosse un’immagine NVS.

| Situazione | Comportamento |
|---|---|
| Lanternbound, PWR o riavvio | Ripristina la fase salvata in pausa; Riprendi concede 650 ms. |
| Riavvio durante il confronto di una reliquia | Riapre le due proposte con l’assetto intatto. La sostituzione è salvata soltanto dopo la conferma. |
| Riavvio durante una battaglia classica non conclusa | Ripristina lo stato precedente all’ingresso e ripropone l’incontro, con consumabili e risorse coerenti. |
| Risultato classico già pagato | Ricostruisce la navigazione senza assegnare di nuovo il premio. |
| Profilo vecchio v1/v2/v3 | Viene migrato senza sostituire l’eroe. I vincitori di una precedente v3 possono ripetere i suggerimenti dalle opzioni. |
| Firmware vecchio con un profilo v4 | Non è una combinazione supportata. Un downgrade richiede un backup compatibile con la vecchia versione. |

Se una NVS è illeggibile o appartiene a un altro layout, l’inizializzazione può ricrearla; la conservazione del profilo non è una promessa valida per qualunque cambio di firmware. I controlli CRC proteggono l’integrità del blob, non sostituiscono un backup.
