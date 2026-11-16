# La guida di Pocket Wayfarer

Pocket Wayfarer è un RPG fantasy per **Waveshare ESP32-S3-Touch-AMOLED-1.75**. Lanternbound è la nuova esperienza HD-2D: scegli un eroe, affronta cinque stanze e torna al borgo con un progetto per il viaggio successivo.

![Borgo, lotta e reliquia: schermate native](img/lantern/panoramica.png)

## Da dove cominciare

| Vuoi… | Apri |
|---|---|
| Mettere il gioco sul Pocket, senza compilare | [Installazione dal browser](INSTALLAZIONE.md) |
| Capire controlli, eroi e reliquie | [Manuale illustrato](GUIDA.md) |
| Aggiornare conservando il profilo, fare un backup o ripristinarlo | [Salvataggi e aggiornamenti](SALVATAGGI.md) |
| Risolvere problemi di USB, schermo o avvio | [Risoluzione dei problemi](RISOLUZIONE-PROBLEMI.md) |
| Giocare su PC, compilare o modificare il progetto | [Sviluppo e build](SVILUPPO.md) |
| Pubblicare sito e firmware su GitHub Pages | [Pubblicazione e rilascio](PUBBLICAZIONE.md) |

La versione `0.3.0-preview.1` è una **preview**. Il core e la UI nativa sono verificati su PC, e il firmware compila per ESP32-S3; il collaudo fisico non è ancora completato. L’installer mostra lo stesso stato, senza presentare questa build come una release già collaudata sul dispositivo.

## Per approfondire

- [Regole e prove del redesign Lanternbound](LANTERNBOUND.md).
- [Guida contestuale e feedback del combattimento](ONBOARDING-FEEDBACK.md).
- [Hardware, partizioni, alimentazione e checklist fisica](FIRMWARE.md).
- [RPG classico: documento di design](RPG-DESIGN.md).
- [Log di verifica correnti](lantern-evidence/README.md).
- [Provenienza della grafica e font](../assets/lantern/README.md), [componenti e licenze](../THIRD_PARTY.md).

I documenti sotto `audit/` descrivono la baseline precedente e le decisioni di redesign; non sostituiscono i manuali correnti. Il gioco del compagno in `firmware/pet/` è una build alternativa per sviluppatori, non un’opzione dell’installer Lanternbound.
