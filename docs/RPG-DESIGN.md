# Pocket Wayfarer — RPG fantasy tascabile

Documento di progettazione (v0.2). È anche la specifica da cui viene scritto
`firmware/rpg/`. Il gioco precedente del cucciolo resta nel repository come build
alternativa (`firmware/pet/`, `-DWF_GAME=pet`).

Lingue: italiano e inglese (scelta alla creazione, modificabile dalle opzioni).

## 1. Il ciclo di gioco

| Ritmo | Cosa si fa |
|---|---|
| 30 secondi | raccogli la caccia automatica, mangia, ritira regalo e missioni |
| 5 minuti | qualche combattimento in un campo, un paio di stanze di dungeon, forgia |
| 20+ minuti | un dungeon completo con boss, cambio classe, duelli/party (M2) |
| in tasca | l'eroe caccia da solo nel campo scelto (gioco passivo) |

**Attivo**: combattimenti a turni con timing sul bordo rotondo (anello in
attacco, onda in difesa), scelta delle abilità e dei bersagli, scelta delle
stanze nei dungeon.
**Passivo**: caccia automatica (fino a 12 h) e pulsante AUTO in combattimento
(l'eroe usa attacchi e abilità da solo con timing "Bene"; giocando a mano si
rende di più).

## 2. L'eroe

Creazione: nome, razza, aspetto (2 corporature, 5 carnagioni, 6 acconciature,
8 colori di capelli), classe iniziale, lingua.

### Razze (estetica + piccoli bonus)

| Razza / Race | Bonus | Aspetto |
|---|---|---|
| Umano / Human | +5% PE | equilibrato |
| Elfo / Elf | +2 DES, +1 INT | orecchie a punta, più slanciato |
| Nano / Dwarf | +2 VIT, +1 FOR | più basso, barba (corporatura 1) |
| Ferino / Beastkin | +2 AGI, +1 FOR | orecchie e coda da felino |

### Caratteristiche (stile RO)

FOR, AGI, VIT, INT, DES, FOR(tuna) — `STR AGI VIT INT DEX LUK`.
3 punti + 1 ogni 5 livelli a ogni livello base. Il livello di classe è
separato e dà 1 punto abilità per livello.

| Derivata | Formula (semplificata) |
|---|---|
| ATK | arma + FOR×2 + DES/2 + liv (archi: DES×2 + FOR/2) |
| MATK | arma magica + INT×2 + liv |
| DEF / MDEF | armature + VIT / armature + INT/2 + VIT/2 |
| PV / PM | per classe e livello, ×(1 + VIT%) / ×(1 + INT%) |
| Critico | FOR(tuna)/3 + DES/10 % (+ ogni timing PERFETTO è critico) |
| Schivata | AGI/4 % (max 30%) contro gli attacchi nemici, prima della parata |
| Rapidità | AGI dà una probabilità di **doppia azione** nel turno |
| Precisione | DES allarga la finestra dell'anello |
| Fortuna | bottino migliore, più gemme d'anima |

Livello base massimo 70 (v0.2), livello classe: 30 / 40 / 50 per gradino.

### Bisogni (componente tamagotchi sull'eroe)

| Bisogno | Cala | Si recupera | Effetto |
|---|---|---|---|
| Fame | −6/ora | cibo | < 20: −20% danni, niente recupero stamina |
| Stamina | stanze (2), lotte nei campi (1), caccia (6/ora) | +10/ora riposando, ×3 alla locanda o dormendo | a 0 non si combatte |
| Morale | sconfitte, fame | vittorie, bottino raro, saluti, regalo | > 80: +10% PE e bottino; < 25: −20% PE |

Regalo giornaliero con serie di giorni, 3 missioni al giorno (stessa logica della v0.1).

## 3. Classi: 8 di base, 16 avanzate, 16 maestre (40)

Cambio classe dal Maestro di classe in città:
- gradino 2: liv. classe 30 e liv. base 20, si sceglie uno dei due rami;
- gradino 3: liv. classe 40 nel gradino 2 e liv. base 45.

Ogni classe ha 3 abilità (le maestre 2: una passiva e una suprema), livello
abilità 1–5. Le abilità dei gradini precedenti restano.

| Base | Arma | Avanzate (gradino 2) | Maestre (gradino 3) |
|---|---|---|---|
| Guerriero / Warrior | spada, scudo | Cavaliere / Knight · Berserker | Cavaliere del Drago / Dragon Knight · Signore della Guerra / Warlord |
| Mago / Mage | bastone | Elementalista / Elementalist · Arcanista / Arcanist | Arcimago / Archmage · Cronomante / Chronomancer |
| Arciere / Archer | arco | Ramingo / Ranger · Cecchino / Sharpshooter | Signore delle Bestie / Beastmaster · Arco della Tempesta / Stormbow |
| Ladro / Rogue | pugnali | Assassino / Assassin · Duellante / Duelist | Lama Notturna / Nightblade · Maestro di Lame / Blademaster |
| Chierico / Cleric | mazza, scudo | Paladino / Paladin · Sacerdote / Priest | Templare / Templar · Ierofante / Hierophant |
| Monaco / Monk | tirapugni | Campione / Champion · Mistico / Mystic | Gran Maestro / Grandmaster · Asceso / Ascendant |
| Artefice / Artificer | martello | Fabbro / Blacksmith · Alchimista / Alchemist | Forgiarune / Runesmith · Trasmutatore / Transmuter |
| Occultista / Occultist | falce, tomo | Negromante / Necromancer · Stregone / Warlock | Signore della Morte / Deathlord · Evocavuoto / Voidcaller |

Le abilità sono costruite da un vocabolario comune (danno singolo, multicolpo,
ad area, cura, potenziamento, indebolimento, risucchio, evocazione, passiva)
con elemento, potenza, costo in PM e ricarica in turni. Elenco completo in
`firmware/rpg/core/classes.c`.

Elementi: Neutro, Fuoco, Acqua, Terra, Vento, Sacro, Oscuro (tabella dei
vantaggi stile RO). Stati: Ustione, Veleno, Gelo, Stordimento, Sanguinamento,
Debolezza, Cecità.

## 4. Equipaggiamento e bottino

Slot: arma, mano secondaria, elmo, armatura, stivali, 2 accessori.
L'aspetto dell'eroe cambia con arma, mano secondaria ed elmo.

| Rarità | Affissi | Colore |
|---|---|---|
| Comune / Common | 0 | bianco |
| Non comune / Uncommon | 1 | verde |
| Raro / Rare | 2 | blu |
| Epico / Epic | 3 | viola |
| Leggendario / Legendary | 3 + effetto unico | arancio |

* Affissi: +caratteristiche, +ATK/MATK %, +DEF, +PV, critico, risucchio, danno elementale, recupero PM.
* **Forgia** +1…+10: probabilità di successo decrescente, da +6 un fallimento fa scendere di 1 (niente oggetti distrutti).
* **Gemme d'anima**: ogni specie di mostro può lasciarne una (raro); si incastonano negli oggetti con alveoli (0–2).
* Tipi di arma per famiglia di classe; armature leggere/medie/pesanti libere, con piccole penalità.

## 5. Mondo

**Città: Borgo Lanterna / Lantern Hollow** — bottega, forgia, locanda,
maestro di classe, bacheca missioni, uscite verso campi e dungeon.

| Campo (caccia attiva/automatica) | Livelli | Dungeon collegato | Piani |
|---|---|---|---|
| Prati di Lumen / Lumen Meadows | 1–10 | Cripta dei Lumi / Lantern Crypt | 10 |
| Riva di Vetro / Glass Shore | 10–20 | Tempio Sommerso / Sunken Temple | 10 |
| Bosco Sussurrante / Whispering Wood | 20–32 | Radici Antiche / Elder Roots | 10 |
| Caverne di Brace / Ember Caves | 32–48 | Fucina Infernale / Infernal Forge | 10 |
| Guglie del Cielo / Sky Spires | 48–70 | Cittadella Celeste / Celestial Citadel | 10 |

### Dungeon a stanze

Ogni piano è una piccola mappa a strati (4 righe di 2–3 stanze + scale), pensata
per lo schermo rotondo: si sceglie una stanza collegata a quella attuale.

| Stanza | |
|---|---|
| Mostri | 1–3 nemici |
| Élite | nemico potenziato, bottino migliore |
| Forziere | oggetto (a volte un mimic!) |
| Trappola | prova di AGI/DES: evitata o danno |
| Fontana | recupera PV/PM |
| Mercante | compra/vendi durante la discesa |
| Evento | scelta con rischio/ricompensa |
| Boss | ogni 5 piani |

PV, PM e stato restano tra le stanze. Si può uscire tra un piano e l'altro
tenendo il bottino; se si viene sconfitti si torna in città perdendo metà delle
monete raccolte nella discesa e un po' di morale.

### Combattimento

Fino a 3 nemici. Si tocca un nemico per bersagliarlo. Ogni turno: azione
dell'eroe (attacco con anello, abilità con anello o diretta, oggetto, guardia,
fuga), poi ogni nemico attacca: schivata automatica (AGI) e altrimenti onda di
parata. Parata perfetta = contrattacco. Pulsante AUTO per i combattimenti facili.

## 6. Multiplayer (ESP-NOW, M2)

* **Party** fino a 4: chi ospita genera il dungeon; ogni dispositivo gioca il
  proprio turno sullo stesso nemico, l'ospite tiene i PV dei nemici.
* **Duello** PvP lock-step (come la v0.1, con classi ed equip).
* **Scambio** di oggetti e monete, **saluti** giornalieri (bonus morale).

## 7. Tappe

| Tappa | Contenuto |
|---|---|
| **M1** | creazione eroe, 8+16+16 classi e abilità, caratteristiche, equip/bottino/forgia/gemme, città, campi e caccia automatica, dungeon a stanze, combattimento multi-nemico con timing e AUTO, bisogni, missioni giornaliere, IT/EN, salvataggi |
| M2 | Legami: party cooperativo, duelli, scambi, amici |
| M3 | suoni, IMU (scuoti), più dungeon e contenuti, simulatore web |
