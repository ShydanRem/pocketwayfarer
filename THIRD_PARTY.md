# Componenti di terze parti

| Componente | Licenza | Uso |
|---|---|---|
| [LVGL](https://github.com/lvgl/lvgl) 9.4 | MIT | interfaccia grafica |
| [ESP-IDF](https://github.com/espressif/esp-idf) | Apache-2.0 | SDK del firmware |
| esp_lcd_co5300 ([esp-iot-solution](https://github.com/espressif/esp-iot-solution)) | Apache-2.0 | driver pannello |
| esp_lcd_touch ([esp-bsp](https://github.com/espressif/esp-bsp)) | Apache-2.0 | driver touch |
| esp_lcd_touch_cst9217 ([Waveshare](https://github.com/waveshareteam/Waveshare-ESP32-components)) | Apache-2.0 | driver touch |
| Inizializzazione display/touch | Apache-2.0 | adattata dal BSP Waveshare ESP32-S3-Touch-AMOLED-1.75 tramite PocketD-D |

Le dipendenze non sono incluse nel repository: `scripts/firmware-deps.py` le
scarica ai commit indicati in `firmware/dependencies.json`.

Grafica (compagni, creature, icone, scenari) e font 5×7 sono originali,
generati dagli script in `scripts/art`.

Il redesign HD-2D usa inoltre asset originali generati con OpenAI ImageGen,
con prompt, sorgenti e trasformazioni registrati in `assets/lantern/`.
I font DejaVu Sans e DejaVu Serif Bold, rasterizzati per la nuova UI, sono
distribuiti con licenza Bitstream Vera e modifiche DejaVu in pubblico dominio;
testo e avvisi completi in `assets/lantern/fonts/LICENSE.txt`.

Il solo host desktop opzionale usa [SDL2](https://github.com/libsdl-org/SDL)
(licenza zlib). `scripts/desktop-deps.py` verifica l’archivio ufficiale 2.32.4
con SHA-256 e ne estrae header e `LICENSE.txt` in `build/desktop-deps/`; collega
la libreria runtime presente nel sistema. SDL2 non entra nel firmware ESP32.

Il sito distribuisce **ESP Web Tools 10.5.2**, Apache-2.0, dal pacchetto npm
ufficiale con integrità SHA-512 fissata. Bundle, licenza e avvisi delle
dipendenze runtime sono in `web/vendor/esp-web-tools/`; il file
`THIRD_PARTY_NOTICES.md` conserva le licenze e gli header originali acquisiti
dagli archivi npm indicati nel lock della release. `receipt.json` registra
gli hash dei file distribuiti. Il font DejaVu del titolo del sito conserva
la stessa licenza già inclusa in `assets/lantern/fonts/LICENSE.txt`.

L’icona a lanterna del sito è un SVG originale del progetto. Le immagini
del sito provengono dalle catture LVGL in `docs/img/lantern/`; il build
registra la loro origine in `assets/image-sources.json` del sito generato.
TamaPoke è citato come riferimento per il flusso di installazione, senza
riutilizzare grafica, firmware o contenuti Pokémon.
