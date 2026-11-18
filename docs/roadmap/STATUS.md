# Registro di avanzamento

Aggiornamento: 18 novembre 2026. Consegna corrente: `0.6.0-preview.2`.

## Consegna R1 — atlante delle classi, 0.4.1-preview.1

**Implementato:** atlante nativo da Borgo → … → Classi; 40 schede e otto percorsi di famiglia; abilità dai dati reali; otto illustrazioni base; chiusura touch/BOOT; nessun cambio classe dall'atlante. Le 32 evoluzioni dichiarano il ritratto di famiglia.

**Verifiche host superate:** 80 schede IT/EN, 208 riferimenti alle abilità locali, 16 percorsi; ritorno circolare, selezione di nodi con scorrimento, chiusura, hook PWR/ripresa e confronto del profilo con la simulazione temporale attesa. Sono state prodotte 17 catture native. Suite completa, ASan/UBSan dell'atlante, revisione indipendente di testi/scroll e compilazione ESP-IDF superate. Queste prove non certificano la latenza fisica.

**Distribuzione:** pacchetto locale, quattro documenti HTML, collegamenti, hash e ZIP verificati; nove test Python e cinque JavaScript superati. La pubblicazione ricostruisce tutto dal commit e verifica i download via HTTP. Il risultato di CI e distribuzione è consultabile nel [workflow](https://github.com/ShydanRem/pocketwayfarer/actions/workflows/pages.yml); versione e commit installabili sono identificati da [release.json](https://shydanrem.github.io/pocketwayfarer/firmware/release.json). [Log della consegna](evidence/README.md).

## Copertura, senza sommare categorie diverse

| Categoria | Completamento attuale |
|---|---|
| Regole delle classi classiche | 40/40 già presenti nel core |
| Illustrazioni base R1 | 8/8 importate, mostrate e verificate nell'atlante |
| Illustrazioni specifiche delle evoluzioni | **16/32**, tutte le avanzate; le sedici maestre esplicitano il riferimento familiare |
| Nuovi avatar modulari classici | **24/40**, otto basi e tutte le sedici avanzate; aspetto e oggetti conservati |
| Fallback legacy delle evoluzioni | 16 maestre, otto combinazioni famiglia/grado; rami fratelli ancora condivisi fino a R4 |
| Assetti Lanternbound nel nuovo trattamento | 3/3; sistema separato |
| Nuovo collaudo sul Pocket | Aperto |

## Consegna R2 — rig e otto basi, 0.5.0-preview.1

**R2.0 completato:** prototipo Guerriero/Mago, chiave classe esatta, preset 44/88/176 pixel e cache con proprietà dei widget. Revisione indipendente prima dell'ammissione delle altre sei basi.

**R2.1 completato:** tutti i canali estetici conservati, quattro espressioni e cinque pose distinte; 61.440 combinazioni estetiche, 16 tipi di equipaggiamento/cinque fasce, save v4 e fallback delle 32 evoluzioni verificati. Torso/piedi/accessori restano statistici.

**R2.2 completato su host:** otto costumi nel renderer delle schermate reali. Creazione IT/EN, schede, borgo, equip, dungeon e formazioni fino a tre nemici/boss/élite Lv99; 147 catture, test di continuità e ASan/UBSan superati. Picco immagini 788.618 byte; firmware locale 3,26 MiB su 6 MiB. [Prove e limiti R2](rig-evidence/README.md).

**Variazione motivata:** i tre canali visibili di equipaggiamento sono stati anticipati da R5 perché gli oggetti legacy non erano coerenti con il nuovo corpo. Nessuna nuova meccanica, famiglia, struttura persistente o modifica degli ID. Animazione definitiva con maschere degli arti resta R7; ambienti/mostri/UI restanti R5. Gate hardware aperto.

## Consegna R3.1 — prime otto avanzate, 0.6.0-preview.1

**Completato su host:** Cavaliere(8), Berserker(9), Elementalista(10), Arcanista(11), Ramingo(12), Cecchino(13), Assassino(14), Duellante(15). Ogni ID ha costume/default headwear e illustrazione specifica; nessuna nuova regola o cambio degli ID. Le altre 24 classi mantengono il fallback esplicito.

**Accettazione:** 122.880 combinazioni estetiche, 180.336 composizioni e 400 confronti dei fratelli con elmo identico (minimo 143 pixel alfa distinti sotto la testa); ASan/UBSan superati. Catture native: 281 del rig, 33 dell'atlante, 24 delle promozioni. Sedici promozioni touch IT/EN, 32 tentativi sotto soglia respinti e 16 roundtrip v4, con aspetto ed equipaggiamento invariati rispetto al core. Revisione indipendente colore/grigio, pose, HUD e promozioni superata.

**Correzioni necessarie:** footer della battaglia su due righe per evitare il taglio dei valori a tre cifre; 71.556 pixel opachi dei glifi verificati dentro il cerchio e sotto i piedi, anche con profili livello 99 IT/EN. Layout nemici/comandi separato applicato anche ai costumi legacy: la vecchia barra del nemico singolo attraversava arma/corpo. Il rig rimane 16/40, non si contano nuovi avatar da questa correzione.

**Budget:** firmware locale 3.504.752 byte /3,34 MiB su 6 MiB, picco buffer immagini rig 788.618 byte. [Prove R3.1](advanced-a-evidence/README.md). Posa finale/maschere degli arti restano R7 e il collaudo fisico resta aperto.

## Consegna R3.2 — tutte le avanzate, 0.6.0-preview.2

**Completato su host:** Paladino (16), Sacerdote (17), Campione (18), Mistico (19), Fabbro (20), Alchimista (21), Negromante (22), Stregone (23). R3 è completo: 24/40 avatar modulari e illustrazioni specifiche; solo le sedici maestre restano in fallback. Nessuna modifica alle regole, agli ID o ai salvataggi v4.

**Accettazione:** 184.320 combinazioni estetiche, 270.909 composizioni, 18.240 controlli di continuità e 800 confronti dei fratelli sotto lo stesso elmo (minimo 119 pixel alfa distinti). Catture native: 415 rig, 49 atlante, 48 promozioni, totale 512. Trentadue promozioni touch IT/EN, 64 soglie respinte e 32 roundtrip v4. Revisione indipendente delle otto nuove classi superata; suite e ASan/UBSan verdi.

**Correzione palette:** sette pixel dell'ampolla viola dell'Alchimista ereditavano il colore della pelle. La regione decorativa ora usa un ruolo fisso, con identità palette RGB565+ruolo. Il PNG sorgente resta intatto; 1.444 confronti verificano la cintura fra le cinque carnagioni. Il test rifiuta il vecchio asset con 28 errori attesi. Questa regressione finale e le verifiche dopo la correzione sono state completate dal lead in prosecuzione sequenziale.

**Budget:** firmware locale 3.590.432 byte / 3,42 MiB su 6 MiB; picco immagini rig 788.618 byte. [Prove R3.2](advanced-b-evidence/README.md). Animazioni definitive e collaudo fisico rimangono aperti.

## Prossimo lotto: R4.1 — prime otto maestre

Ordine: Cavaliere del Drago (24), Signore della Guerra (25), Arcimago (26), Cronomante (27), Signore delle Bestie (28), Arco della Tempesta (29), Lama Notturna (30), Maestro di Lame (31). Obiettivo: `0.7.0-preview.1`; nessuna maestra viene conteggiata prima dell'integrazione e delle prove.

1. Definire due firme del costume indipendenti da tinta, copricapo e arma, seguendo la bibbia; produrre costume, copricapo e illustrazione per ogni ID.
2. Estendere metadata e test al grado UI 2, conservando tutti i canali estetici e gli oggetti posseduti. Non introdurre famiglie o array persistenti nuovi.
3. Provare il percorso completo base → avanzata → maestra. Verificare le soglie 45/40, il reset job previsto dal core, abilità ereditate, save v4 e ritorno corretto alla schermata.
4. Catturare nomi lunghi e dialoghi IT/EN, cinque pose, equipaggiamento, battaglie e grigio/same-helmet; correggere il layout prima dell'ammissione se necessario.
5. Chiudere con matrice cosmetica, sanitizer, memoria/flash e distribuzione verificata. Poi R4.2 completa le altre otto maestre ID 32–39. R5/R7 e gate hardware restano distinti.
