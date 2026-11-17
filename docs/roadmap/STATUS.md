# Registro di avanzamento

Aggiornamento: 17 novembre 2026. Baseline pubblica prima del lotto: `0.4.0-preview.1`, commit `d6f7f97e4d3cbb2bda94c8d34fcb62a3f2ed1560`.

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
| Nuovi avatar modulari classici | 0/40; sistema precedente mantenuto |
| Aspetti legacy distinti per famiglia/grado | 24 combinazioni; rami fratelli condivisi |
| Assetti Lanternbound nel nuovo trattamento | 3/3; sistema separato |
| Nuovo collaudo sul Pocket | Aperto |

## Prossimo lotto: R2.0

**R2.0:** prototipo del rig e mapping per ID classe esatto, con Guerriero e Mago. Prima risolvere ancoraggi, canali estetici, cache e layout in una scena completa; poi produrre gli otto costumi base. Nessuna produzione di sedici evoluzioni prima che il rig sia verificato.
