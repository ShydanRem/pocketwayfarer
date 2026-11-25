# R7.8 — progressione del Ladro

![Lo Zaino mostra l’attacco effettivo quando il Ladro torna al pugnale](../../img/hero_rig/confronto-pugnale.png)

La statistica principale del Ladro e delle sue quattro evoluzioni è l’Agilità. Nella preview.8 era già utile per schivare e ottenere azioni extra, mentre l’attacco del pugnale dipendeva soprattutto dalla Forza.

La parte di attacco ricavata dalle statistiche ora è **FOR + max(FOR, AGI) + DES/2** per i pugnali. La Forza vale sempre per la prima quota. Quando resta almeno pari all’Agilità, vale anche per la seconda. Fra spada e pugnale, la voce **Attacco eroe** nello Zaino indica il cambio effettivo, con bonus e gemme del personaggio.

## Campagna simulata

Dopo il confronto fra più coefficienti sui seed `120–143`, abbiamo ripetuto la campagna sui 40 seed `170–209` per ogni famiglia. Il simulatore parte dal livello 1 e usa punti, bottino e riposi ottenuti giocando. La strategia assegna 20 seed a ciascun ramo del Ladro.

| Timing | Finali del Ladro, preview.8 | Finali del Ladro, preview.9 |
|---|---:|---:|
| Bene costante | 19/40 | 32/40 |
| Attacco Bene, parata mista | 15/40 | 26/40 |
| Ottimo costante | 40/40 | 40/40 |

Con timing Bene, i finali passano da 8 a 17 su 20 per i profili destinati al ramo Assassino e da 11 a 15 su 20 per quello Duellante. I tentativi mediani nei campi passano da 80 a 64. Le tracce delle altre sette famiglie coincidono riga per riga in tutti e tre gli scenari.

```sh
cc -std=c11 -O2 -Ifirmware/rpg/core -Ifirmware/common \
  firmware/rpg/core/*.c firmware/common/rng.c firmware/common/timing.c \
  firmware/rpg/host/playtest_campaign.c -o /tmp/pocket-campaign
/tmp/pocket-campaign 170 40 good
/tmp/pocket-campaign 170 40 mixed
/tmp/pocket-campaign 170 40 great
```

Per riprodurre la preview.8 usa il core del commit `2e6d5a6f479c60d8657a7f0be11411461c972985`. [Simulatore e modalità di timing](../../../firmware/rpg/host/playtest_campaign.c)

## Verifiche

- Calcolo dell’attacco sopra, sotto e a parità di Forza. La proiezione include gli affissi dell’arma e coincide con le statistiche dopo lo scambio. [Prova del core](../../../firmware/rpg/host/test_rpg.c)
- Scambio spada/pugnale in entrambe le direzioni nello Zaino, nelle due lingue, con salvataggio e riavvio. [Prova LVGL](../../../firmware/rpg/host/test_device_ui.c)

Restano da osservare sul Pocket la scelta dell’arma durante una partita reale e la difficoltà con timing variabile.
