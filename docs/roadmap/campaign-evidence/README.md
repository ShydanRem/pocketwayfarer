# R7.5 — primo viaggio e campagna

![La guida del primo avvio sul display circolare](../../img/hero_rig/guida-avvio.png)

| Tappa | Percorso e correzione |
|---|---|
| Avvio | Scegli una classe e crea l’eroe. Al borgo puoi accettare la guida o saltarla. Un profilo esistente può attivarla dal Diario. |
| Prima lotta | Nei Prati di Lumen ti aspetta un Gelatino singolo. Impari i segnali di touch e BOOT e la prima vittoria indica la Cripta. |
| Prima stanza | I nodi dorati indicano le stanze raggiungibili. Il passo si chiude quando entri. Attacco e parata conservano il proprio stato di apprendimento. |
| Preparazione e ritirata | Servizi, equipaggiamento e classe restano al borgo. Dal punto di partenza di un piano puoi tornare gratis. Senza pergamena, la ritirata dall’interno lascia metà dell’oro della discesa. |
| Regioni e finale | Ogni boss finale mostra la prossima isola. Il quinto dungeon ha un riepilogo dedicato e la mappa conta i completamenti. |

## Prove su PC

- La stessa partita di prova attraversa **5 regioni, 50 piani, 103 combattimenti e 10 boss**. Usa un Guerriero preparato al livello di ogni regione, timing perfetto simulato, costi di stanza normali, riposo a pagamento e riavvio fra le regioni. [Test della campagna](../../../firmware/rpg/host/test_rpg.c)
- Il primo giro effettivo della UI LVGL verifica in italiano e inglese creazione, guida, attacco via touch, parata via BOOT, prima stanza, ritirata senza pergamena e riavvio. Una prova dedicata verifica il riepilogo del quinto dungeon e la mappa a 5/5. [Test del percorso](../../../firmware/rpg/host/test_device_ui.c) · [Test del finale](../../../firmware/rpg/host/preview_hero_rig.c)
- Una prova del primo incontro con 128 seed per ciascuna delle otto classi ha rilevato 18 incontri a tre nemici ogni 128 nella versione precedente. Il percorso guidato usa un incontro singolo. La simulazione con uso automatico di abilità e oggetti ha vinto 1.024 su 1.024 incontri guidati anche con il timing fallito. Il dato vale per questa simulazione.
- Il salvataggio resta v5. Si conservano i progressi nello stesso blob e si verifica la lettura dei dati precedenti, la penalità di ritirata applicata una sola volta e il morale dopo la sconfitta. [Regressioni](../../../firmware/rpg/host/test_reliability.c)

Restano da misurare sul Pocket il timing fisico, i tempi di crescita e la comprensione della guida al primo uso. [Roadmap](../../ROADMAP.md)
