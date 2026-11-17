# Controllo del brief con Game Maker AI

[Brief inglese](class-brief.en.md) e [risposte originali integrali](game-maker-results.json) conservano perimetro, proposte e limiti del plugin.

**Game brief score: 83/100.** È un controllo euristico dell'inglese, non un voto alla qualità, al codice o alla giocabilità. Le segnalazioni riguardano misurabilità, punteggio/progressione, schermate, viewport mobile e aggettivi.

Perimetro risolto nel piano operativo: 40 schede senza punteggio o condizioni di sconfitta, otto illustrazioni base, 32 riferimenti familiari espliciti; navigazione, chiusura, profilo invariato e catture native verificabili. L'atlante è consultazione, non un nuovo livello da vincere. I menu e i risultati di combattimento esistenti restano invariati.

Il parametro `mobile` del plugin è usato come proxy dei comandi touch, perché il tool non dispone di un target firmware. Il requisito reale è **466×466 circolare C/LVGL**. Le proposte automatiche di asset 64×64, sfondo 390×844, viewport 375px e suoni non sono adottate. Non viene generato un progetto browser o un link a un builder esterno: il lavoro prosegue nel repository nativo esistente.

`plan_game_balance` riceve soltanto i dati pertinenti già presenti: cinque stanze e boss da 176 PV. Restituisce `incomplete` perché è un modello a ondate/spari/salto. Non si inventano velocità, vite, cadenze di fuoco o punteggi da 100 per nemico; questi valori non entrano nel gioco. Non è richiesto un ribilanciamento per una consegna grafica in sola lettura. Le regole esistenti e le simulazioni native restano la fonte di verifica.

Il brief iniziale usa “Master/Maestro” come termine di design: nella UI effettiva il percorso è **Classico → Eroe → Classe**. L’implementazione e il manuale usano questa navigazione reale.
