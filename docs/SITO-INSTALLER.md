# Sito e installer: implementazione e verifiche

Il sito sorgente è in `web/`; il build pubblicabile è `build/site/`. Non serve un backend, un account del giocatore o una connessione Wi-Fi sul Pocket. La pagina guida il computer a installare il firmware via USB tramite ESP Web Tools 10.5.2.

## Dal sorgente al dispositivo

1. `VERSION` viene incorporato nel descrittore ESP-IDF dell’applicazione.
2. `package-firmware.py` legge `project_description.json`, `flasher_args.json` e i binari. Verifica progetto/target, versione incorporata, impostazioni del bootloader, quattro partizioni attese, dimensioni e assenza di sovrapposizioni.
3. I tre file, il manifest e i metadati vengono inclusi nel sito e in uno ZIP completo. Il formato dei salvataggi e il commit sorgente sono registrati; una build locale con modifiche viene etichettata esplicitamente.
4. La pagina controlla capacità del browser, modello confermato, metadati, SHA-256, versione/chip e tabella partizioni.
5. Solo dopo i controlli viene caricato il componente ESP Web Tools. Il manifest usa Blob URL dei byte già verificati, evitando un secondo download non vincolato alla verifica.
6. Il browser richiede la porta USB soltanto quando l’utente preme il pulsante. Il dialogo gestisce scrittura ed eventuali errori. La pagina non mostra una finta percentuale o un esito di installazione inventato.

Il manifest usa `new_install_prompt_erase: true`: la cancellazione è una scelta esplicita, non un default automatico imposto dal nostro sito. `new_install_improv_wait_time: 0` evita una falsa procedura di provisioning Wi-Fi per un firmware che non la implementa. I file ordinari non includono la partizione `wf_nvs`.

## Contenuti e dipendenze

La landing presenta il gioco con catture della UI effettiva. `guide.html` è generato dai manuali Markdown, mantenendo immagini, tabelle e procedure in un’unica fonte. `notices.html` conserva font, licenze e avvisi dell’installer. L’icona della lanterna è un SVG originale; nessun asset di TamaPoke viene redistribuito.

ESP Web Tools e le dipendenze browser sono locali al sito. Archivio npm, integrità SHA-512 e hash dei file sono fissati in `web/dependencies.json`, `runtime-notices.json` e nel receipt del vendor. Il sito non dipende da un CDN JavaScript per avviare l’installer.

## Verifiche riproducibili

```sh
python3 scripts/vendor-web-tools.py --check
python3 scripts/build-site.py --firmware-build build/firmware-hd2d --allow-dirty
python3 scripts/check-site.py --site build/site
POCKET_TEST_BUILD=build/firmware-hd2d python3 -m unittest discover -s tests/site -p 'test_*.py'
node --test tests/site/install.test.mjs
```

`check-site.py` controlla lingua, H1, alt delle immagini, link, ancore, percorsi relativi adatti a `/pocketwayfarer/`, risorse CSS, moduli locali, hash dei binari e corrispondenza dei file dentro lo ZIP. I nove test Python rifiutano target/versioni errati, pacchetti incompleti, partizioni sbagliate, header flash e percorsi non validi. I cinque test Node coprono i byte effettivi, il fallimento di integrità e il manifest costruito dai file verificati.

L’anteprima gestita del workspace ha restituito un timeout del relay sulla porta browser; la verifica visuale nel browser locale non viene dichiarata superata. La compilazione firmware, i controlli statici e i test del pacchetto sono indipendenti da quel servizio. La scrittura su hardware resta non eseguita.

La pubblicazione è verificata soltanto quando GitHub Pages restituisce un deployment riuscito; vedere [PUBBLICAZIONE.md](PUBBLICAZIONE.md). Le evidenze del gioco nativo restano in [lantern-evidence](lantern-evidence/README.md).
