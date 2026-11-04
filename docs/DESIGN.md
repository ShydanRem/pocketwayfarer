# Progettazione di gioco

Obiettivo: un gioco che si apre per 30 secondi più volte al giorno (cura,
missioni, spedizioni) ma che regge anche sessioni da 15 minuti (esplorazione,
guardiani, duelli). Tutto con un pollice su uno schermo rotondo da 1,75".

## Ciclo di attenzione ("perché lo riapro?")

| Ritmo | Gancio |
|---|---|
| minuti | bisogni che calano, disordine da pulire, carezze con ricarica di 5 min |
| ore | spedizioni da 30 m / 2 h / 8 h, energia che torna dormendo |
| giorno | regalo con serie di giorni, 3 missioni nuove, saluto agli amici (1/giorno) |
| settimane | evoluzioni (lv 5 e 12) con forme diverse in base a cibo e cura, 5 guardiani |

Il compagno **non muore**: la trascuratezza produce "errori di cura" che
peggiorano l'umore, abbassano il legame e possono portare alla forma adulta
"selvatica" invece che a quella "luminosa". Punisce senza far smettere.

## Bisogni (core/game.c)

Valori 0..100000 (milli-percento), simulati a passi di 1 minuto con
recupero offline fino a 14 giorni.

| | sveglio | dorme | in spedizione |
|---|---|---|---|
| Fame | −12,5%/h | −6,25%/h | −6,25%/h |
| Energia | −8,3%/h | +25%/h | −4,2%/h |
| Umore | −10%/h (×2 se affamato/stanco, +50% per disordine) | fermo | fermo |

Energia a 0: si addormenta da solo (+1 errore). Fame o umore a 0 per un'ora:
+1 errore, −3% legame. Il disordine compare ogni 150 minuti da sveglio (max 3).

In lotta i bisogni contano: umore < 25% → Attacco −15%; fame < 20% → Difesa
−15%; energia < 20% → Velocità −15%; umore > 80% e legame > 60% → Attacco +10%.

## Evoluzione

* **Seme**: 24 tocchi per scaldarlo e farlo schiudere (gratificazione immediata).
* **Lumino** → al livello 5 forma elementale in base all'affinità: peperoncino,
  alga e germoglio (e le zone dove combatte) spingono verso Brace, Marea o Fronda.
* Al livello 12 forma adulta: **luminosa** con legame ≥ 50% e ≤ 6 errori,
  altrimenti **selvatica** (più attacco, meno difesa).

## Lotta

Motore deterministico a interi (core/battle.c) separato dall'input:
l'interfaccia trasforma il tocco in una qualità 0..3 (core/timing.c).

* **Anello** (attacco): periodo 0,9–1,7 s secondo il livello del nemico; arco
  "bene" largo 18–48° secondo Velocità e Umore, "ottimo" ~55%, "perfetto" ~22%.
  Moltiplicatori: 35% / 100% / 130% / 175%.
* **Parata** (difesa): l'onda arriva in 0,6–1,4 s; finestre ±45/95/170 ms
  allargate dalla Velocità. Danno subito 100% / 70% / 45% / 0% + contrattacco.
* **Focus** (max 3): +1 attaccando o difendendo; l'abilità ne costa 2.
  Brace incendia (8% PV × 2 turni), Marea cura il 40% del danno, Fronda radica
  (prossimo attacco nemico −40%). Elementi: ×1,5 / ×0,67.
* Danno = ATT × potenza × 50 / (50 + DIF) × qualità × variazione 92–108%.

Bilanciamento verificato nei test: un Lumino lv 1 batte una creatura lv 1 in
2–6 turni; il primo guardiano batte quasi sempre chi gioca senza timing al lv 5
e perde quasi sempre contro chi para e colpisce bene al lv 6.

## Mondo

| Isola | Livelli | Creature | Guardiano |
|---|---|---|---|
| Prati di Lumen | 1–4 | Gelatino, Fungotto, Scintillo | Re Gelatino lv 5 |
| Riva di Vetro | 4–8 | Granchietto, Medusina, Gelatino Blu | Granchio Corazzato lv 10 |
| Bosco Sussurrante | 8–13 | Radicotto, Pipistrello, Gufo Lunare | Antico Ceppo lv 15 |
| Caverne di Brace | 13–18 | Salamandra, Golem di Lava, Pipistrello Ardente | Drago di Cenere lv 20 |
| Guglie del Cielo | 18–26 | Tempestino, Cristallo Errante, Grifoncino | Custode Celeste lv 28 |

Le creature selvatiche si adattano al livello del compagno dentro la fascia
dell'isola. Il guardiano si sblocca dopo 6 vittorie e si affronta quando si vuole.

## Idee per le prossime versioni

* Raid cooperativo: un guardiano condiviso, il dispositivo che ospita somma i
  danni di tutto il party (il protocollo Legami è già pensato per estenderlo).
* Suoni con il codec ES8311 della scheda, vibrazione "virtuale" con flash.
* IMU QMI8658: scuoti per giocare / svegliare.
* Equipaggiamento (collari, talismani) e scambio tra amici.
* Simulatore nel browser (WebAssembly) costruito dalla stessa UI LVGL.
