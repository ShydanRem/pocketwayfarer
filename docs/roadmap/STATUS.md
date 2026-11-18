# Registro di avanzamento

Aggiornamento: 18 novembre 2026. Consegna corrente: `0.5.0-preview.1`.

## Consegna R1 — atlante delle classi, 0.4.1-preview.1

**Implementato:** atlante nativo da Borgo → … → Classi; 40 schede e otto percorsi di famiglia; abilità dai dati reali; otto illustrazioni base; chiusura touch/BOOT; nessun cambio classe dall'atlante. Le 32 evoluzioni dichiarano il ritratto di famiglia.

**Verifiche host superate:** 80 schede IT/EN, 208 riferimenti alle abilità locali, 16 percorsi; ritorno circolare, selezione di nodi con scorrimento, chiusura, hook PWR/ripresa e confronto del profilo con la simulazione temporale attesa. Sono state prodotte 17 catture native. Suite completa, ASan/UBSan dell'atlante, revisione indipendente di testi/scroll e compilazione ESP-IDF superate. Queste prove non certificano la latenza fisica.

**Distribuzione:** pacchetto locale, quattro documenti HTML, collegamenti, hash e ZIP verificati; nove test Python e cinque JavaScript superati. La pubblicazione ricostruisce tutto dal commit e verifica i download via HTTP. Il risultato di CI e distribuzione è consultabile nel [workflow](https://github.com/ShydanRem/pocketwayfarer/actions/workflows/pages.yml); versione e commit installabili sono identificati da [release.json](https://shydanrem.github.io/pocketwayfarer/firmware/release.json). [Log della consegna](evidence/README.md).

## Copertura, senza sommare categorie diverse

| Categoria | Completamento attuale |
|---|---|
| Regole delle classi classiche | 40/40 già presenti nel core |
| Illustrazioni base R1 | 8/8 importate, mostrate e verificate nell'atlante |
| Illustrazioni specifiche delle evoluzioni | 0/32; etichetta di riferimento familiare esplicita |
| Nuovi avatar modulari classici | **8/40**, le otto basi; personalizzazione e oggetti conservati |
| Fallback legacy delle evoluzioni | 32 classi, 16 combinazioni famiglia/grado; rami fratelli condivisi fino a R3/R4 |
| Assetti Lanternbound nel nuovo trattamento | 3/3; sistema separato |
| Nuovo collaudo sul Pocket | Aperto |

## Consegna R2 — rig e otto basi, 0.5.0-preview.1

**R2.0 completato:** prototipo Guerriero/Mago, chiave classe esatta, preset 44/88/176 pixel e cache con proprietà dei widget. Revisione indipendente prima dell'ammissione delle altre sei basi.

**R2.1 completato:** tutti i canali estetici conservati, quattro espressioni e cinque pose distinte; 61.440 combinazioni estetiche, 16 tipi di equipaggiamento/cinque fasce, save v4 e fallback delle 32 evoluzioni verificati. Torso/piedi/accessori restano statistici.

**R2.2 completato su host:** otto costumi nel renderer delle schermate reali. Creazione IT/EN, schede, borgo, equip, dungeon e formazioni fino a tre nemici/boss/élite Lv99; 147 catture, test di continuità e ASan/UBSan superati. Picco immagini 788.618 byte; firmware locale 3,26 MiB su 6 MiB. [Prove e limiti R2](rig-evidence/README.md).

**Variazione motivata:** i tre canali visibili di equipaggiamento sono stati anticipati da R5 perché gli oggetti legacy non erano coerenti con il nuovo corpo. Nessuna nuova meccanica, famiglia, struttura persistente o modifica degli ID. Animazione definitiva con maschere degli arti resta R7; ambienti/mostri/UI restanti R5. Gate hardware aperto.

## Prossimo lotto: R3.1 — otto avanzate

Ordine vincolante: Cavaliere (8), Berserker (9), Elementalista (10), Arcanista (11), Ramingo (12), Cecchino (13), Assassino (14), Duellante (15).

1. Fissare due firme di forma per ramo nella bibbia; verificare che restino sotto un casco equipaggiato.
2. Produrre costume/default headgear modulari e illustrazione specifica per ciascun ID; mantenere gli ancoraggi del rig accettato.
3. Provare subito un'intera scena per i due rami della stessa famiglia: niente ammissione basata solo sul foglio sorgente.
4. Estendere test/registro, compatibilità cosmetica e gear, catture IT/EN/grigio, promozione effettiva e save v4 senza cambiare i requisiti del core.
5. Chiudere il lotto con build e budget misurati, poi R3.2: Paladino/Sacerdote, Campione/Mistico, Fabbro/Alchimista, Negromante/Stregone (ID16–23).

Le 16 maestre e le 32 illustrazioni specifiche delle evoluzioni non sono ancora completate. Non si salta a R4 prima dell'accettazione delle avanzate.
