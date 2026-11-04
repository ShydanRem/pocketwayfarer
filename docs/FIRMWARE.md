# Firmware

## Hardware

Waveshare **ESP32-S3-Touch-AMOLED-1.75**: ESP32-S3R8 (8 MB PSRAM), 16 MB flash,
AMOLED 466×466 CO5300 su QSPI, touch CST9217, espansore TCA9554 (tasto PWR),
PMIC AXP2101, RTC PCF85063, tasto BOOT su GPIO0.

Pin e sequenza di inizializzazione del pannello sono quelli già verificati
sull'hardware in PocketD-D (`firmware/main/board.c`, derivato dal BSP
Waveshare, Apache-2.0). Nessun registro di carica/tensione dell'AXP2101 viene
scritto.

## Toolchain

* ESP-IDF **5.5.x** (`. $IDF_PATH/export.sh`)
* LVGL **9.4** e driver fissati a commit precisi in `firmware/dependencies.json`;
  `python3 scripts/firmware-deps.py` li scarica in `firmware/.deps` (sparse
  checkout, nessun component registry).

```bash
python3 scripts/firmware-deps.py
```

```bash
cd firmware && idf.py set-target esp32s3 && idf.py build
```

```bash
idf.py -p /dev/ttyACM0 flash monitor
```

## Due giochi, un firmware

`firmware/main` è indipendente dal gioco: parla con l'interfaccia "app" di
`common/platform.h`. `-DWF_GAME=rpg` (predefinito) compila l'RPG, `-DWF_GAME=pet`
il compagno della v0.1. Ognuno salva con la propria chiave NVS.

## Memoria

| | |
|---|---|
| App | partizione `factory` da 6 MB |
| Salvataggio | partizione NVS `wf_nvs` (256 KB), un blob versionato con CRC32 (~1,4 KB per l'RPG), scritto in modo atomico da NVS |
| Grafica | sprite indicizzati 8 bit in flash, espansi a 3× in PSRAM (RGB565 + alfa) alla prima visualizzazione e liberati a ogni cambio schermata; l'eroe è composto a runtime da 13 livelli ritagliati |
| Binario | RPG ~1,9 MB, compagno ~1,6 MB |
| Rendering | LVGL a buffer parziale (466×48 righe, DMA interna), lettura touch ogni 10 ms per i minigiochi di timing |

## Tempo

L'RTC PCF85063 (0x51) fornisce i secondi reali; se l'oscillatore risulta
fermo viene inizializzato a una data di default. Tra una lettura e l'altra si
usa `esp_timer`, che continua a contare in light sleep. Se l'RTC non risponde
il tempo riparte dall'accensione (la simulazione resta coerente finché il
dispositivo non viene spento del tutto). L'ora del giorno (giorno/tramonto/notte)
si corregge dal Diario senza toccare la simulazione.

## Energia

* PWR o 45 s di inattività (fuori da lotte, minigiochi e Legami): salvataggio,
  radio spenta, pannello spento.
* A schermo spento: light sleep a passi di 40 ms; risveglio da touch (INT su
  GPIO11), BOOT (GPIO0) o PWR. Con USB collegata niente light sleep (per il monitor).
* Wi-Fi/ESP-NOW acceso solo in *Legami* e durante i duelli.

## Controlli su PC

`bash scripts/check.sh`:

1. verifica che la grafica generata sia aggiornata (`build_art.py --check`);
2. compila ed esegue i test del core con AddressSanitizer/UBSan;
3. compila la vera UI LVGL per PC ed esegue i preview dei due giochi, che
   giocano uno scenario completo (per l'RPG: creazione dell'eroe, città,
   lotta con anello e parata, cambio classe, equip, bottega, forgia, dungeon,
   boss, caccia automatica, inglese) e salvano gli screenshot in `docs/img`.

## Collaudo

Non ancora eseguito sull'hardware. Da verificare, in ordine:

- [ ] avvio, luminosità, tocco (posizioni dei pulsanti sul bordo)
- [ ] fluidità di anello e parata (frame rate, latenza del tocco)
- [ ] tasti BOOT/PWR, spegnimento e risveglio, consumi in light sleep
- [ ] RTC: ora conservata a dispositivo spento con batteria
- [ ] salvataggio dopo stacco improvviso dell'alimentazione
- [ ] fluidità della composizione dell'eroe e dei cambi schermata
- [ ] (compagno v0.1) Legami tra due Pocket: vicini, saluto, dono, duello
