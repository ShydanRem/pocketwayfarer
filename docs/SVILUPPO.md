# Compilare, provare e modificare Pocket Wayfarer

Il gioco principale è C11 con UI LVGL 9.4. Il target firmware è ESP32-S3; l’host Linux esegue le stesse regole e schermate attraverso SDL2. Il sito è HTML/CSS/JavaScript statico, pubblicato da GitHub Pages.

## Struttura

| Percorso | Contenuto |
|---|---|
| `firmware/rpg/core/` | Regole deterministiche, salvataggi, Lanternbound, politica dei suggerimenti. |
| `firmware/rpg/ui/` | Schermate LVGL, effetti procedurali e dati grafici generati. |
| `firmware/rpg/host/` | Test del modello, regressioni, preview e host interattivo Linux. |
| `firmware/common/` | Toolkit, rendering, timing e interfaccia verso l’hardware. |
| `firmware/main/` | Display, touch, tasti, RTC, NVS e ciclo ESP-IDF. |
| `assets/lantern/` | Sorgenti grafici, prompt, font, licenze e manifest degli asset. |
| `assets/hero-rig/` | Rig modulare classico: sorgenti, prompt, ancoraggi, palette semantiche e oggetti visibili. |
| `web/` | Sito, manuale HTML e installer con ESP Web Tools distribuito e verificato localmente con il sito. |
| `scripts/` | Importazione asset, test, packaging firmware e sito. |
| `docs/` | Manuali correnti, evidenze e audit storici separati. |

## Provare la UI su Linux

Servono compilatore C/C++, CMake, Python 3 e runtime SDL2. Dalla radice:

```sh
bash scripts/play-desktop.sh
```

Lo script acquisisce gli header SDL2 ufficiali con hash fissato, compila e avvia il gioco. Non installa librerie di sistema. Mouse = touch; Spazio/Invio = BOOT breve; Esc = BOOT lungo; P = PWR; F5 = salva e ricarica. Il profilo predefinito è `build/desktop-save.bin`.

Per una partita separata:

```sh
bash scripts/play-desktop.sh --save build/prova-personale.bin
```

## Test e immagini native

Con Python e CMake disponibili, installa le dipendenze Python (in un ambiente virtuale consigliato):

```sh
python -m pip install -r requirements-art.txt -r requirements-site.txt
```

```sh
bash scripts/check.sh
```

Lo script verifica i dati grafici generati, esegue i test del core e del campionamento input con AddressSanitizer/UBSan e produce le schermate reali della UI in `docs/img`. Le catture sono del motore LVGL, non mockup del sito. La suite del modello include 600 spedizioni; la preview Lanternbound verifica 49 catture e 432 salvataggi. `test_device_ui` aggiunge tre catture di timing, frame lenti, input prima dell'aggiornamento, errori di salvataggio, durata delle cache e dialoghi scorrevoli. Le sue catture sono in `build/host/device-regressions`. Imposta `POCKET_PREVIEW_ROOT=build/prova` per conservare i PNG delle preview fuori dai documenti.

Per rigenerare intenzionalmente i dati HD-2D:

```sh
python3 scripts/build_lantern_art.py
python3 scripts/build_lantern_art.py --check
python3 scripts/build_hero_rig.py --check
```

Il rig aggiunge `test_hero_rig --full` (245.760 combinazioni estetiche),
`test_hero_cache` e `preview_hero_rig` (591 catture) e `preview_promotions` (104 catture). La suite li esegue
automaticamente; dettagli e configurazione sanitizer nelle
[prove R4.1](roadmap/master-a-evidence/README.md). La promozione include 16 percorsi touch completi base → avanzata → maestra, soglie 45/40 e otto abilità ereditate. Le scene Studio del rig sono
strumenti di verifica del renderer, non nuove schermate giocabili.

Mantieni insieme sorgenti, prompt, licenze e manifest. Non sovrascrivere gli originali per correggere un difetto dell’importatore.

## Firmware ESP32-S3

Installa ESP-IDF **5.5.1** dal progetto ufficiale e attiva il suo ambiente. Le dipendenze LVGL/driver sono fissate in `firmware/dependencies.json`:

```sh
python3 scripts/firmware-deps.py
cd firmware
idf.py -B ../build/firmware -D SDKCONFIG="$PWD/../build/firmware/sdkconfig" set-target esp32s3
idf.py -B ../build/firmware -D SDKCONFIG="$PWD/../build/firmware/sdkconfig" build
```

La versione applicativa viene da `VERSION`; CMake mantiene anche la versione numerica del progetto. Usa una cartella build nuova quando cambi target o configurazione. Per flash e monitor seriale su un dispositivo collegato:

```sh
idf.py -B ../build/firmware -p PORTA flash monitor
```

La build alternativa del compagno usa `-DWF_GAME=pet` e una cartella build distinta. Il pacchetto dell’installer è destinato alla build RPG, non alla variante pet.

## Generare il pacchetto installabile e il sito

Dalla radice, dopo il build:

```sh
python3 scripts/vendor-web-tools.py --check
python3 scripts/build-site.py --firmware-build build/firmware
python3 scripts/check-site.py --site build/site
node --test tests/site/install.test.mjs
```

Per una preview locale con modifiche non ancora committate, aggiungi `--allow-dirty` a `build-site.py`: l’installer la etichetterà come build locale. I rilasci automatizzati richiedono sorgenti puliti.

Il sito risultante è in `build/site/`, con firmware, ZIP, hash, immagini e dipendenze locali. Può essere servito come sito statico da `localhost`; non aprirlo come `file://` se vuoi verificare l’installer. Su un proprio computer puoi usare `python3 -m http.server 4173 --directory build/site` e aprire `http://localhost:4173`. Nel workspace gestito si usa invece il servizio `sites-preview start /workspace/pocketwayfarer`, quando disponibile.

`package-firmware.py` legge gli indirizzi da `flasher_args.json`, verifica il binario e la tabella partizioni e rifiuta file fuori layout, versioni incoerenti e sovrapposizioni. La UI ricontrolla dimensioni e SHA-256 e passa a ESP Web Tools esattamente i byte verificati mediante Blob URL.

## Criteri di consegna

I test automatici dimostrano i percorsi esercitati, non il divertimento o la latenza fisica. Per una modifica alle regole esegui i test del core e una regressione pertinente; per una modifica alla UI controlla il rendering nativo. Per l’installer verifica anche percorso GitHub Pages, browser non supportato, integrità fallita e pacchetto completo. Il [collaudo hardware](FIRMWARE.md#collaudo) resta una verifica separata.
