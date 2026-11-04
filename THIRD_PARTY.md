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
