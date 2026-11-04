# Legami: protocollo tra dispositivi

Trasporto: **ESP-NOW** sul canale Wi-Fi 1, senza router né associazione.
La radio è accesa solo nella schermata *Legami* e durante un duello.
I beacon vanno in broadcast; tutto il resto in unicast (ESP-NOW aggiunge
ACK e ritrasmissioni a livello MAC, il protocollo aggiunge le proprie).

Codice: `firmware/core/link.c` (indipendente dalla radio, testato su PC con
un canale che perde, duplica e riordina i pacchetti).

## Pacchetto (52 byte, little endian)

| Offset | Campo |
|---|---|
| 0–2 | `'W' 'F'` versione 1 |
| 3 | tipo |
| 4–7 | sessione (nonce) |
| 8–19 | nome (12 byte) |
| 20–23 | forma, livello, elemento, flag (occupato/dorme/via) |
| 24–25 | vittorie in duello |
| 26–33 | PV, ATT, DIF, VEL (istantanea per il duello) |
| 34–37 | metà del seme del duello |
| 38–39 | oggetto, quantità (dono) |
| 40–43 | round, azione, qualità (turno del duello) |
| 51 | checksum |

## Scambi

* **BEACON** ogni 1,5 s; un vicino sparisce dopo 9 s di silenzio.
* **GREET → GREET_ACK**: entrambi ricevono +umore/+legame una volta al giorno
  per amico. Ripetuto ogni 250 ms fino a 16 volte; deduplicato per sessione.
* **GIFT → GIFT_ACK**: l'oggetto lascia lo zaino subito; se nessuna conferma
  arriva viene restituito.
* **Duello**
  1. A invia `DUEL_INVITE` (istantanea + metà seme) finché B risponde (30 s).
  2. B accetta con `DUEL_ACCEPT` (istantanea + metà seme) e inizia la lotta;
     A risponde `DUEL_START`.
  3. Ogni round entrambi inviano `DUEL_TURN(round, azione, qualità)` finché
     ricevono quello dell'altro. Con le due mosse ciascuno chiama
     `wf_duel_round()` sullo **stesso stato con lo stesso seme**: il risultato è
     identico sui due dispositivi senza un "server".
  4. Se un turno del round precedente arriva in ritardo, si risponde con la
     propria mossa di quel round (recupero dei pacchetti persi).
  5. `DUEL_LEAVE` per uscire; 20 s di silenzio = connessione persa.

Nel duello la qualità del timing (anello) vale per l'attacco; la difesa è la
scelta "Difendi". Vince chi manda a zero l'altro; dopo 30 round vince chi ha
più PV in proporzione.
