# Pubblicare sito e firmware con GitHub Pages

Il sito è pensato per il repository `ShydanRem/pocketwayfarer`. La pagina e i binari vengono dallo **stesso commit**, evitando di pubblicare istruzioni nuove con firmware vecchio. Il workflow controlla i sorgenti, compila con ESP-IDF 5.5.1, prepara un pacchetto completo e costruisce il sito statico.

## Attivazione iniziale del repository

Questa operazione si fa una volta, con un account autorizzato a gestire GitHub Pages:

1. Nel repository apri **Settings → Pages**.
2. In **Build and deployment → Source** scegli **GitHub Actions**.
3. Integra il workflow nel branch predefinito del repository. Non è richiesto che si chiami `main`: il workflow usa il branch predefinito restituito da GitHub.
4. Apri **Actions → Build firmware and publish website** e avvia il workflow, oppure esegui un push sul branch predefinito.

L’indirizzo standard previsto è `https://shydanrem.github.io/pocketwayfarer/`. Si considera pubblicato soltanto quando il job di deployment termina con successo e restituisce l’URL; la sola presenza dei file HTML non prova che il sito sia online.

Il token ordinario di GitHub Actions non abilita Pages per la prima volta. Il job rileva l’impostazione mancante e la segnala, senza fingere una pubblicazione riuscita. Il [workflow ufficiale configure-pages](https://github.com/actions/configure-pages) documenta i permessi necessari per l’abilitazione iniziale.

## Cosa genera il workflow

- Build ESP32-S3 della sola modalità RPG, con versione letta da `VERSION`.
- `bootloader.bin`, `partitions.bin`, `app.bin` agli indirizzi effettivi della build.
- `manifest.json` per ESP Web Tools, con scelta esplicita della cancellazione e nessuna attesa di provisioning Wi-Fi.
- `release.json`, con versione, commit, stato preview, formato salvataggi, dimensioni e SHA-256.
- ZIP completo con istruzioni e hash.
- Sito statico con screenshot nativi, font/licenze e bundle locale ESP Web Tools.

La cartella del sito pubblicato è `build/site/`. I file di sviluppo, i salvataggi desktop e le credenziali non vengono copiati nel sito. Le dipendenze browser sono fissate da `web/dependencies.json` e accompagnate dagli avvisi delle dipendenze runtime.

## Aggiornare il gioco

1. Aggiorna `VERSION` e le note di rilascio. Mantieni coerente la versione numerica in `firmware/CMakeLists.txt` quando cambia la serie.
2. Esegui i test pertinenti e aggiorna gli screenshot quando cambia la UI.
3. Integra le modifiche nel branch predefinito. Il workflow ricompila e ripubblica il pacchetto completo.
4. Controlla i job e apri l’URL restituito dal deployment. La pagina deve mostrare la nuova versione, non una stringa scritta a mano in HTML.
5. Finché il collaudo fisico non è completato, mantieni lo stato **preview** e `hardware_tested: false`.

Le pull request costruiscono e verificano il sito ma non pubblicano su Pages. Il job di deployment usa l’ambiente `github-pages` e i permessi `pages: write` e `id-token: write`; le fasi di build hanno soltanto accesso in lettura al repository.

## Controlli prima di pubblicare

```sh
bash scripts/check.sh
python3 scripts/vendor-web-tools.py --check
python3 scripts/build-site.py --firmware-build build/firmware
python3 scripts/check-site.py --site build/site
node --test tests/site/install.test.mjs
```

Il build rifiuta per default modifiche non committate. `--allow-dirty` serve a una preview locale e viene registrato nei metadati; non deve diventare una scorciatoia nel workflow pubblico.

## Ripristino di un sito precedente

Usa un commit noto del repository e riesegui lo stesso workflow dal branch predefinito dopo aver preparato un revert revisionabile. Il sito e il firmware devono essere ripristinati insieme. Un downgrade del firmware sul Pocket richiede anche un formato di salvataggio compatibile: non basta tornare indietro con l’HTML.

## Riferimenti

- [GitHub Pages: pubblicazione con workflow](https://docs.github.com/en/pages/getting-started-with-github-pages/using-custom-workflows-with-github-pages).
- [ESP Web Tools: manifest e cancellazione](https://esphome.github.io/esp-web-tools/).
- [Installer TamaPoke](https://socquique.github.io/TamaPoke/web/), riferimento per il percorso USB dal browser; non vengono copiati firmware, grafica o contenuti del gioco.
