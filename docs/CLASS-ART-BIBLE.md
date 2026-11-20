# Classi — direzione artistica e contratto di produzione

Obiettivo: otto famiglie immediatamente riconoscibili, quaranta identità di classe coerenti. Fonte di riferimento: [revisione della leggibilità](ART-CLARITY.md) e nuova arena nativa. Le illustrazioni rappresentative nell'atlante non definiscono razza, genere o aspetto obbligatorio del giocatore.

## Tre regole visive

1. **La forma precede l'ornamento.** Spalle, cappa, torso e distribuzione degli accessori identificano la via. Almeno due segnali della classe restano visibili con un casco equipaggiato. Armi e mani non si fondono con il torso.
2. **Il colore distribuisce l'attenzione.** Ombre indaco, luci avorio e un accento caldo o verde acqua. Il fondale è più quieto dietro il personaggio. Niente bloom o contorno chiaro spesso usato per nascondere una sagoma illeggibile.
3. **Ogni pixel deve reggere la dimensione reale.** Gruppi di pixel netti, pochi materiali leggibili, niente microdecorazioni casuali o sfocatura. La verifica avviene nel gioco a 466×466, anche in grigio, con HUD e pulsanti reali.

## Famiglie e firme dei rami

| Famiglia | Base | Ramo A: avanzata → maestra | Ramo B: avanzata → maestra |
|---|---|---|---|
| Guerriero | Armatura compatta avorio/navy, cappa corta mattone | Cavaliere: masse protettive chiuse → Drago: lamelle e tagli obliqui sulla cappa | Berserker: braccia aperte e fasce asimmetriche → Guerra: mantello largo e cintura da comando |
| Mago | Veste prugna/verde acqua, profilo verticale | Elementalista: pannelli alternati e polsini a cristallo → Arcimago: colletto e manto a costellazione | Arcanista: veste geometrica e fascia del tomo → Cronomante: lembi doppi e pendoli compatti |
| Arciere | Giacca corta muschio/crema, faretra leggibile | Ramingo: cappa irregolare e motivi del branco → Bestie: pelliccia stilizzata e spallaccio aperto | Cecchino: linee strette e protezione dell'avambraccio → Tempesta: lembi e cuciture a saetta |
| Ladro | Giacca indaco, sciarpa verde acqua, cintura rame | Assassino: sagoma raccolta e lembi spezzati → Notturna: mantello tagliato e fasce sottili | Duellante: torso aperto e maniche affusolate → Lame: cappa corta a ventaglio e bracciali doppi |
| Chierico | Avorio/rosa, emblema della lanterna-solare | Paladino: piastre sovrapposte e tabarro → Templare: struttura solida con sigilli netti | Sacerdote: tessuti verticali e stola → Ierofante: manto largo e ricami a raggi grandi |
| Monaco | Zafferano, fasce alle mani, perle | Campione: fasce larghe e protezioni compatte → Gran Maestro: drappi sovrapposti e nodi grandi | Mistico: linee leggere e sciarpa lunga → Asceso: pannelli a petalo e pochi punti luminosi |
| Artefice | Grembiule ocra, armatura da lavoro verde acqua | Fabbro: guanti pesanti e grembiule ferrato → Forgiarune: placche con poche rune ampie | Alchimista: fiale grandi e tasche → Trasmutatore: doppi polsini e contenitori geometrici |
| Occultista | Cappuccio notte, fodera lavanda, tomo | Negromante: colletto a costole e fasce → Morte: manto corazzato e bordi angolari | Stregone: tessuto nero/violetto e spalle oblique → Vuoto: aperture nette e falde spezzate |

Queste sono firme di costume proposte per la produzione modulare. Le armi illustrate non autorizzano a sostituire quelle realmente equipaggiate. Nessun compagno animale, evocazione o effetto ornamentale introduce una nuova meccanica di combattimento per il solo fatto di apparire in un concept.

## Illustrazioni dell'atlante — R1

- Sorgente RGBA con quattro colonne e due righe; ogni cella contiene un rappresentante intero, rivolto a tre quarti verso destra. Gutter trasparente su tutti i bordi, nessun testo dipinto nell'asset.
- Runtime 88×88 a 2×, silhouette massima 76 pixel logici e piedi ancorati alla riga 84. Scala condivisa per tutte le otto figure, campionamento nearest-neighbor, alfa binario a soglia 128, massimo 96 colori per illustrazione, RGB565.
- Nel layout corrente il centro della figura è `(233,197)`: i pixel opachi devono stare sotto il pannello superiore e sopra il badge, nell'intervallo verticale 123–278. Le prove leggono l'alfa del buffer realmente espanso.
- Otto illustrazioni base; le 32 schede di evoluzione riportano **Ritratto della famiglia**, senza conteggiarlo come ritratto specifico. Non colorare una base e chiamarla maestra completata.
- Sorgenti/prompt/hash in [assets/classes](../assets/classes/README.md). Le immagini sono generate con ImageGen e importate meccanicamente; non dichiarate disegnate a mano.

## Avatar del giocatore — R2 e successivi

Il rig deve avere una chiave di classe esatta, un piano dei piedi condiviso e livelli registrati. Razza, pelle, occhi, capelli e variante del volto restano dati dell'eroe. Posa ed espressione sono assi distinti; un blink non sostituisce un'animazione di attacco.

Definire e provare l'ordine: cappa/oggetti posteriori → corpo → volto → abito → capelli/orecchie/barba → copricapo posseduto → arma/mano secondaria. Chiarire l'eventuale introduzione di torso e stivali posseduti prima di generare quei livelli. Il copricapo di classe è una scelta di default, non un oggetto da sovrapporre al casco dell'utente.

Non estendere semplicemente il canvas 48→88 mantenendo scale e coordinate attuali. Prima una scena completa con due famiglie, poi il lotto da otto. Nessun descrittore di immagine mostrato deve essere sovrascritto da una cache. Le versioni legacy si rimuovono solo quando il percorso sostitutivo copre i chiamanti e supera le regressioni.

**Contratto R2 implementato:** canvas 88×88, preset mondo 176, scheda 88, mappa 44 pixel. Costume esatto per ID base; testa/volto, capelli, pelle e occhi indipendenti. Copricapo posseduto sostituisce quello della classe. Arma/mano secondaria/testa sono i tre canali visibili; torso, piedi e accessori non introducono livelli grafici nuovi. La falce viene composta davanti all'abito ma dietro testa/capelli per conservare il volto in attacco. Quiver solidale al torso. Respiro con overlap al giunto della vita; piedi fermi nelle cinque pose parametriche. Le animazioni finali con maschere specifiche degli arti restano R7. [Sorgenti e trasformazioni](../assets/hero-rig/README.md).

## R3.1 — otto avanzate ammesse

| ID | Classe | Due firme sotto un elmo identico |
|---|---|---|
| 8 | Cavaliere | Spalle chiuse a piastre sovrapposte; tabarro lungo rettangolare |
| 9 | Berserker | Braccia aperte e spalla in pelliccia; fasce diagonali e falde irregolari |
| 10 | Elementalista | Polsini larghi a cristallo; pannelli triangolari svasati |
| 11 | Arcanista | Lapelli geometrici e imbracatura diagonale; doppie falde dritte e strette |
| 12 | Ramingo | Cappa frastagliata asimmetrica; stivali e gambali robusti |
| 13 | Cecchino | Cappotto lineare stretto; lungo bracciale e stivali aderenti |
| 14 | Assassino | Cappa corta spezzata a tre punte; torso chiuso e polpacci fasciati |
| 15 | Duellante | Giacca chiara aperta; mezza cappa e singola falda lunga |

Costumi e illustrazioni specifici sono separati. I 400 confronti dei fratelli con lo stesso elmo conservano almeno 143 pixel di sagoma distinti sotto la testa; la revisione nativa in grigio verifica le firme sopra. Alla consegna R3.1 le altre otto avanzate e tutte le maestre erano ancora nei lotti successivi. Le pose parametriche condivise non vengono conteggiate come animazioni finali.

## R3.2 — sedici avanzate completate

| ID | Classe | Due firme sotto lo stesso elmo |
|---|---|---|
| 16 | Paladino | Piastre protettive sovrapposte; tabarro largo e rettangolare |
| 17 | Sacerdote | Stole lunghe e verticali; maniche morbide e pendenti |
| 18 | Campione | Fasce/guanti compatti e protetti; placche larghe su fianchi e gambe |
| 19 | Mistico | Sciarpa lunga a due falde; veste leggera e asimmetrica |
| 20 | Fabbro | Grembiule lungo con pannello ferrato; guanti e stivali massicci |
| 21 | Alchimista | Cintura con grandi ampolle; giacca corta aperta e polsini morbidi |
| 22 | Negromante | Colletto ampio a costole; fronte chiuso con grandi fasce orizzontali |
| 23 | Stregone | Spalle oblique; falde laterali aperte e spezzate |

Alla consegna R3.2 erano ammessi gli ID 0–23: otto basi e tutte le sedici avanzate. I colori delle decorazioni non seguono la pelle: il layout registra la regione fissa della cintura dell'Alchimista e la palette distingue RGB565 e ruolo anche quando condividono la stessa tinta. Le ampolle sono parte del costume, non quantità di consumabili.

## R4.1 — prime otto maestre

| ID | Classe | Due firme sotto lo stesso elmo |
|---|---|---|
| 24 | Cavaliere del Drago | Lamelle oblique su torso/fianchi; cappa stretta a due lunghe punte oblique |
| 25 | Signore della Guerra | Mantello corto largo e squadrato; cintura massiccia e pannelli corti ferrati |
| 26 | Arcimago | Manto basso largo a stella; gonna triangolare ampia e continua |
| 27 | Cronomante | Due code strette e rettangolari separate; piastra circolare al fianco e tracolla obliqua |
| 28 | Signore delle Bestie | Collo in pelliccia a grandi ciuffi su un lato; giacca corta smerlata e ginocchiere pesanti |
| 29 | Arco della Tempesta | Due lunghe falde spezzate a saetta; grande bracciale angolare su un avambraccio |
| 30 | Lama Notturna | Cappa lunga con unico taglio diagonale; torso e polpacci fasciati stretti |
| 31 | Maestro di Lame | Cappa corta larga a ventaglio; due bracciali sovrapposti e giacca chiara aperta |

Sono ammessi gli ID 0–31, con metadata maestri famiglia 0–3/grado UI 2. Le altre otto maestre restano in fallback R4.2. Le firme si verificano anche senza arma e mano secondaria: 1.200 confronti dei fratelli, quattro razze, cinque fasce elmo e cinque pose. La soglia minima di 119 pixel alfa diversi deriva dalla matrice complessiva, non da un voto estetico. La revisione nativa colore/grigio completa il controllo automatico.

Le illustrazioni restano rappresentanti separati dal giocatore. La cresta del Drago, la pelliccia delle Bestie e i dettagli dell'orologio sono decorazioni di costume, senza nuove creature, evocazioni o regole. Le pose restano trasformazioni parametriche preliminari; R7 è distinto.

**Correzione delle cappe lunghe:** il rettangolo del braccio tagliava la cappa in attacco sugli ID 24/30. Per questi due costumi il layout registra braccia rigide: attacco/guardia/colpito traslano l'abito connesso e le prese degli oggetti insieme, mantenendo le suole ferme. Non si ricostruiscono parti del PNG né si dichiara completata l'animazione. Le maschere separate per arti e cappa restano R7; una posa semplice ma continua precede il movimento articolato.

## R4.2 — ultime otto maestre

| ID | Classe | Firme del costume con lo stesso elmo |
|---|---|---|
| 32 | Templare | Piastre squadrate a incastro e tabarro stretto con sigillo |
| 33 | Ierofante | Mantello semicircolare basso e gonna continua a raggi |
| 34 | Gran Maestro | Drappi diagonali sovrapposti e grande nodo squadrato |
| 35 | Asceso | Pannelli a petalo aperti e anelli larghi ai polsi |
| 36 | Forgiarune | Grembiule a piastre sovrapposte e grandi protezioni quadrate |
| 37 | Trasmutatore | Contenitori esagonali e polsini doppi su giacca corta |
| 38 | Signore della Morte | Spalle angolari corazzate e fronte chiuso con barre orizzontali |
| 39 | Evocavuoto | Spalla obliqua e falde spezzate con apertura centrale |

I 40 ID validi hanno grafica dedicata. Le pose preliminari per le vesti continue degli ID 33, 35, 38 e 39 usano la stessa soluzione adottata per le cappe degli ID 24 e 30. Le prese seguono la traslazione della veste e le suole restano ferme dopo la trasformazione della razza. [Verifiche native](roadmap/master-b-evidence/README.md).

## R7.2a — prime pose di combattimento del Classico

Il Paladino (16) raccoglie l’arma e porta lo scudo verso il busto. Il Cavaliere del Drago (24) ha un affondo più aperto e una cappa che segue il busto. Le pose partono dai costumi registrati. La deformazione del costume e gli oggetti usano le stesse posizioni delle mani, mentre capo, capelli e copricapo condividono la posizione del collo. Il campionamento mantiene la trama a pixel e conserva le righe dei piedi. [Sorgenti e catture R7.2a](roadmap/classic-combat-evidence/README.md)

## R7.2b — arco e faretra, bastone e falce

Arciere separa le mani in carica e raccoglie il gesto al contatto. La faretra segue il busto. Il Mago alza il bastone in carica e lo porta avanti nel colpo. La falce dell’Occultista cambia angolo intorno alla presa e passa dietro volto e capelli. La cappa conserva il contorno mentre cambia l’appoggio del busto. I tre profili coprono tutti i rami delle rispettive famiglie. [Pose a dimensione reale](roadmap/classic-weapons-evidence/README.md)

## Checklist di produzione

- Sorgenti, prompt, identità del generatore e trasformazioni registrati.
- Contorni delle celle puliti, niente armi tagliate o fondi residui.
- Sagome distinte a scala reale e in grigio; due rami non dipendono soltanto dalla tinta o dal cappello.
- Occhi, mani e arma restano leggibili senza rumore di pixel aggiunto per simulare dettaglio.
- Piedi non nascosti dai controlli, pose senza variazioni arbitrarie di scala.
- Nomi e testi IT/EN contenuti, chiusura e ritorno sempre raggiungibili sul cerchio.
- Memoria/flash misurate, rendering incrementale coerente col completo, nessuna mutazione del profilo causata dalla consultazione.
- Catture native revisionate; il collaudo fisico è registrato separatamente.
