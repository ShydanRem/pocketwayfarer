# Firmware

Per mettere il gioco sul Pocket senza compilare, usa la [guida di installazione USB dal browser](INSTALLAZIONE.md). Per backup e aggiornamenti leggi [SALVATAGGI.md](SALVATAGGI.md).

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

L'RPG include ora la prima spedizione HD-2D Lanternbound, con accesso al gioco
classico dal borgo. Regole, catture effettive e istruzioni del build dedicato:
[LANTERNBOUND.md](LANTERNBOUND.md). Il formato RPG v4 legge v1/v2/v3 e conserva lo stato completo della spedizione,
incluso il timing in pausa. Aggiunge reliquia iniziale, bestiario, frazioni
oro delle missioni e snapshot transazionale per gli incontri classici. Un
riavvio ripropone l’incontro classico dallo stato precedente all’ingresso;
la nuova spedizione riprende invece l’esatta fase salvata, in pausa. La v4
conserva anche lezioni apprese e preferenza dei suggerimenti; il feedback
registra l’attore coinvolto e la Risonanza usata. Gli effetti visivi derivano
dal tempo della fase e non introducono timer di regole separati.

## Memoria

| | |
|---|---|
| App | partizione `factory` da 6 MB |
| Salvataggio | partizione NVS `wf_nvs` (256 KB), un blob versionato con CRC32 (2808 byte per l’RPG v4, header incluso), scritto in modo atomico da NVS |
| Grafica | sprite indicizzati 8 bit in flash, espansi in PSRAM (RGB565 + alfa) e liberati al cambio schermata. HD-2D: fondali 466×466 a 1×, attori 88×88 a 1×/2×; nel classico l'eroe resta composto da livelli ritagliati |
| Binario | RPG con Lanternbound, quaranta avatar modulari e ambienti del Classico fino al Tempio Sommerso. Circa 4,57 MiB con 24% libero nella partizione da 6 MiB. Il compagno è una build alternativa. [Dimensione e hash](roadmap/shore-evidence/firmware.json) |
| Rendering | LVGL a buffer parziale (466×48 righe, DMA interna), lettura touch ogni 10 ms per i minigiochi di timing |

## Tempo

L'RTC PCF85063 (0x51) fornisce i secondi reali; se l'oscillatore risulta
fermo viene inizializzato a una data di default. Tra una lettura e l'altra si
usa `esp_timer`, che continua a contare in light sleep. Se l'RTC non risponde
il tempo riparte dall'accensione (la simulazione resta coerente finché il
dispositivo non viene spento del tutto). L'ora del giorno (giorno/tramonto/notte)
si corregge dal Diario senza toccare la simulazione.

Il clock LVGL legge continuamente i millisecondi di `esp_timer`, anche durante lo spegnimento del pannello. Prima del rendering si aggiorna la simulazione; touch e BOOT aggiornano inoltre il timing all'istante dell'evento. BOOT usa il fronte di pressione nei minigiochi. Il risveglio richiede un rilascio prima di accettare nuovi comandi. [Audit e regressioni](DEVICE-AUDIT-2026-11-17.md).

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
   giocano scenari completi (Lanternbound: 45 catture, 306 salvataggi validati; per l’RPG classico: creazione dell'eroe, città,
   lotta con anello e parata, cambio classe, equip, bottega, forgia, dungeon,
   boss, caccia automatica, inglese) e salvano gli screenshot in `docs/img`.

Per una prova interattiva dello stesso codice nativo, su Linux con SDL2:

```sh
bash scripts/play-desktop.sh
```

Dettagli e controlli: [LANTERNBOUND.md](LANTERNBOUND.md#giocare-sul-desktop-linux).
Dimensione esatta e hash della build: [firmware.json](lantern-evidence/firmware.json).

## Collaudo

Non ancora eseguito sull'hardware. Da verificare, in ordine:

- [ ] avvio, luminosità, tocco (posizioni dei pulsanti sul bordo)
- [ ] fluidità di anello e parata (frame rate, latenza del tocco)
- [ ] tasti BOOT/PWR, spegnimento e risveglio, consumi in light sleep
- [ ] RTC: ora conservata a dispositivo spento con batteria
- [ ] salvataggio dopo stacco improvviso dell'alimentazione
- [ ] fluidità della composizione dell'eroe e dei cambi schermata
- [ ] (compagno v0.1) Legami tra due Pocket: vicini, saluto, dono, duello
