# Registro di avanzamento

Aggiornato il 19 novembre 2026. Consegna corrente `0.8.0-preview.8`. [Roadmap operativa](../ROADMAP.md)

## Consegna R1 — atlante delle classi, 0.4.1-preview.1

**Implementato:** atlante nativo da Borgo → … → Classi; 40 schede e otto percorsi di famiglia; abilità dai dati reali; otto illustrazioni base; chiusura touch/BOOT; nessun cambio classe dall'atlante. Le 32 evoluzioni dichiarano il ritratto di famiglia.

**Verifiche host superate:** 80 schede IT/EN, 208 riferimenti alle abilità locali, 16 percorsi; ritorno circolare, selezione di nodi con scorrimento, chiusura, hook PWR/ripresa e confronto del profilo con la simulazione temporale attesa. Sono state prodotte 17 catture native. Suite completa, ASan/UBSan dell'atlante, revisione indipendente di testi/scroll e compilazione ESP-IDF superate. Queste prove non certificano la latenza fisica.

**Distribuzione:** pacchetto locale, quattro documenti HTML, collegamenti, hash e ZIP verificati; nove test Python e cinque JavaScript superati. La pubblicazione ricostruisce tutto dal commit e verifica i download via HTTP. Il risultato di CI e distribuzione è consultabile nel [workflow](https://github.com/ShydanRem/pocketwayfarer/actions/workflows/pages.yml); versione e commit installabili sono identificati da [release.json](https://shydanrem.github.io/pocketwayfarer/firmware/release.json). [Log della consegna](evidence/README.md).

## Copertura, senza sommare categorie diverse

| Categoria | Completamento attuale |
|---|---|
| Regole delle classi classiche | 40/40 già presenti nel core |
| Illustrazioni base R1 | 8/8 importate, mostrate e verificate nell'atlante |
| Illustrazioni specifiche delle evoluzioni | **32/32**, tutte le avanzate e tutte le maestre |
| Nuovi avatar modulari classici | **40/40**, otto basi, sedici avanzate e sedici maestre |
| Fallback legacy delle evoluzioni | 0 classi valide |
| Assetti Lanternbound nel nuovo trattamento | 3/3; sistema separato |
| Luoghi classici ridisegnati | Borgo, cinque regioni e cinque dungeon, fino alla Cittadella Celeste |
| Mostri del Classico ridisegnati | **42/42** |
| Interfacce di servizio del Classico | Mappa, zaino, bottega, forgia e diario aggiornati |
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

## Consegna R4.1 — prime otto maestre, 0.7.0-preview.1

**Integrati su host:** Cavaliere del Drago (24), Signore della Guerra (25), Arcimago (26), Cronomante (27), Signore delle Bestie (28), Arco della Tempesta (29), Lama Notturna (30), Maestro di Lame (31). Copertura 32/40; le otto maestre 32–39 restano esplicite. Costume, copricapo default e illustrazione specifica; nessuna nuova regola, famiglia o struttura persistente.

**Accettazione:** 245.760 combinazioni estetiche, 361.485 composizioni, 24.320 controlli di continuità e 1.200 confronti sotto lo stesso elmo. La mutazione tier2→1 viene respinta con 16 errori attesi. Catture: 591 rig, 64 atlante, 104 promozioni, totale 759. Trentadue colonne Statistiche IT/EN ai livelli 45/99 sono dentro il cerchio, senza wrap o invasione del canvas avatar.

**Promozioni:** 64 cambi touch e salvataggi riletti, 96 rifiuti sotto soglia. Includono 16 percorsi completi base → avanzata → maestra IT/EN, soglie 44/40 e 45/39 respinte, salto diretto e ramo errato rifiutati; otto ID delle abilità ereditate confrontati anche in battaglia, rank precedenti conservati, reset job 1/xp 0 e termine della via a job 50.

**Correzioni necessarie:** la colonna Statistiche automatica entrava nel canale avatar con i nomi lunghi. Ora usa 260 px centrati a 290, con tutte le quattro righe dentro il cerchio. Le cappe lunghe di Drago/Lama Notturna evitano il taglio rettangolare del braccio con una posa coerente e prese degli oggetti solidali; le maschere articolate restano R7. [Prove R4.1](master-a-evidence/README.md). Animazione definitiva, R5 e collaudo fisico rimangono aperti.

## Consegna R4.2 — tutte le 40 classi

La preview `0.7.0-preview.2` completa Templare (32), Ierofante (33), Gran Maestro (34), Asceso (35), Forgiarune (36), Trasmutatore (37), Signore della Morte (38) ed Evocavuoto (39). Tutti gli ID validi usano costumi modulari e illustrazioni dedicate. Le 32 verifiche dei percorsi base → avanzata → maestra coprono tutti i rami in italiano e inglese, comprese soglie, abilità ereditate e salvataggi v4.

La revisione delle pose ha individuato e corretto strappi nelle vesti di Ierofante, Asceso, Signore della Morte ed Evocavuoto. I test coprono 307.200 combinazioni estetiche, 1.600 confronti sotto lo stesso elmo, 64 casi di Statistiche a livello 45 e 99 e 1.003 catture native. [Prove R4.2](master-b-evidence/README.md).

## Consegna R5.1 — interfaccia del personaggio

La preview `0.8.0-preview.1` porta l’archivio nel Classico. Creazione con indicatore delle otto tappe, scheda eroe leggibile con i nomi lunghi, abilità con gradi separati, carte di promozione e atlante nella stessa palette. Equipaggiamento con nomi degli slot sempre visibili e dettagli coerenti. I due flussi IT/EN attraversano creazione, spesa punti e rimozione/ripristino oggetto fino alla rilettura del salvataggio. [Prove R5.1](classic-ui-evidence/README.md)

## Consegna R5.2 — borgo e prima regione

La preview `0.8.0-preview.2` ridisegna il borgo del Classico e i Prati di Lumen. Le prime **8 specie su 42** usano nuove sagome. I test bilingui seguono il percorso dal borgo all’incontro, selezionano ogni nemico, completano anello e parata, ritirano le ricompense e rileggono il salvataggio. È verificato anche l’ingresso in Cripta dal piano 6. [Catture e prove R5.2](lumen-evidence/README.md)

## Consegna R5.3 — Cripta e navigazione

La mappa distingue il prossimo passo, la stanza attuale, le stanze visitate e quelle ancora da scoprire. I comandi **Esci** e **Scorte** sono sempre nella stessa posizione. Il Mimic e il Re delle Ossa sono integrati con un nuovo fondale. Se torni al borgo dopo aver battuto il Re Gelatino al piano 5, il record sblocca l’ingresso al piano 6. [Catture e prove R5.3](crypt-evidence/README.md)

## Consegna R5.4 — Riva di Vetro e Tempio Sommerso

Il nuovo campo usa ardesia al tramonto e il Tempio usa pietra verde sommersa. Otto specie sono integrate. Campo, battaglie e dungeon usano i controlli già introdotti a Lumen. Il confronto nativo ha portato a scegliere 233 × 233 per entrambi i fondali. Il percorso in IT/EN parte dalla Riva bloccata, attraversa i due boss e si chiude con il Bosco raggiungibile. [Catture, prove e budget](shore-evidence/README.md)

## Consegna R5.5 — Bosco Sussurrante e Radici Antiche

La radura color salvia rende leggibile il Lupo nero. Nelle Radici Antiche spiccano le ali della Fata oscura. Il Ragno tessitore ha una sagoma bassa e ramata, la Regina Ragno ha un addome alto e zampe color avorio. Le prove in italiano e inglese coprono tre bersagli, una trappola, il checkpoint e lo sblocco delle Caverne. [Catture, prove e budget](wood-evidence/README.md)

## Consegna R5.6 — Caverne di Brace e Fucina Infernale

Lava e bagliori restano fuori dalla fascia centrale di battaglia. Le forme del Golem, del Drago di cenere e del Signore dei Demoni risaltano sulla pietra fredda. Il percorso bilingue copre anello d’attacco, intento e parata con touch e BOOT, entrambi i boss e accesso alle Guglie dopo il riavvio. [Catture, prove e budget](ember-evidence/README.md)

## Consegna R5.7 — Guglie del Cielo e Cittadella Celeste

La terrazza lilla e il pavimento verde petrolio lasciano spazio ai gruppi di nemici. Le ultime otto specie comprendono il Custode Celeste e il Drago Stellare. Il giro bilingue verifica il checkpoint, il ritorno al borgo e il record del piano 10 dopo due riavvii. [Catture, prove e budget](sky-evidence/README.md)

## Consegna R5.8 — mappa e interfacce di servizio

Le cinque regioni e il borgo si riconoscono sulla mappa illustrata. Righe adattabili e ritorno coerente via touch e BOOT unificano zaino, bottega, forgia e diario. Le prove bilingui percorrono le cinque regioni, raggiungono l’ultimo oggetto con swipe reali e verificano acquisti, vendita, potenziamento e missione dopo un riavvio. [Catture, prove e budget](services-evidence/README.md)

## Prossimo lotto R6.1 — ritmo delle spedizioni

1. Fissare due seed e un profilo di riferimento. Cronometrare l’arrivo alla prima scelta, il tempo fra incontri, le soste, le sconfitte e il ritorno al borgo.
2. Ripetere le prove a parità di eroe e progressione. Usare il primo playtest per capire quali attese o scelte fanno interrompere la spedizione.
3. Scegliere un solo intervento in base alle osservazioni e verificare gli stessi punti dopo la modifica. R6.2 usa la base così misurata per una nuova scelta rischio e ricompensa.

Il collaudo fisico può procedere in parallelo e serve per la stabile. [Sequenza e criteri](../ROADMAP.md#r6-e-r7--profondità-ritmo-e-feedback)
