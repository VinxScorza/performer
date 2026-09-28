# PER|FORMER — verifica del delta operativo 0.4.5 → 0.4.6-beta.1

Stato della verifica: **controlli automatici superati; prova finale sul modulo da eseguire**.

La baseline è il tag `0.4.5`. Per il firmware, il tag contiene gli stessi sorgenti del commit di release `cfc19cf9`; il commit successivo riguarda soltanto presentazione e licenza.

> **ATTENZIONE — prima di provare la beta fare un backup della SD.** La 0.4.6-beta.1 apre i progetti della 0.4.5, ma al primo salvataggio li converte da `Version40` a `Version41`. Da quel momento la 0.4.5 e le versioni precedenti non riescono più a rileggerli: il controllo del file fallisce e il progetto viene scartato. Conservare quindi una copia originale dei progetti 0.4.5 separata da quella usata con la beta.

## Delta funzionale

| Area | Differenza rispetto alla 0.4.5 | Verifica eseguita | Da verificare sul modulo |
|---|---|---|---|
| Progetti | Il formato passa a `Version41` per salvare i filtri MIDI. I progetti `Version40` si aprono con tutti i nuovi filtri attivi. | Percorso di lettura e inizializzazione controllato; build STM32 riuscita. | Apertura di un progetto reale 0.4.5, salvataggio con un altro nome e ricaricamento. |
| MIDI input | Nuovi filtri per Notes, CC, Program Change, Pitch Bend e Aftertouch. Note Off resta sempre accettato, anche quando Notes è disabilitato. La mappatura Program Change resta quella fissa della 0.4.5. | Test automatici su valori predefiniti, blocco CC e coppia Note On/Note Off. | Porte MIDI e USB MIDI reali; Program Change, Bend, Aftertouch e persistenza dopo il salvataggio. |
| Copy/Paste/Duplicate | Comportamento uniforme nei cinque editor Note, Curve, Stochastic, Logic e Arp. Paste richiede una destinazione esplicita; Duplicate conserva gli spazi della selezione, estende Last Step e segnala `NO ROOM`. | Test C++ su selezioni continue, selezioni con spazi, assenza di spazio e Paste senza destinazione; compilazione di tutti i cinque editor. | Un passaggio manuale rapido in ciascun editor. |
| Navigazione | Il doppio clic su `T1..T8` entra direttamente nell'editor Steps quando la pagina non usa già quei tasti per un'altra funzione. | Test UI su doppio clic e sui salti dalle pagine di progetto. | Sensibilità del doppio clic sui tasti fisici. |
| Fill | Durante il Fill l'encoder può cambiare temporaneamente il divisore della traccia, senza cambiare il divisore salvato né il BPM. Funziona dal pannello e dal Launchpad. | Test UI sui comandi diretti, globali, modali e Launchpad; controllo dell'azzeramento al rilascio e alla disconnessione. | Effettiva variazione ritmica su Note, Curve, Logic, Stochastic e Arp. |
| Tracce collegate | Il comportamento resta quello della 0.4.5: una traccia collegata eredita timing e divisore dalla traccia principale. Il nuovo divisore Fill agisce sulle tracce indipendenti e non sostituisce quello ereditato. | Percorsi di playback controllati nel codice e comportamento documentato nel manuale. | Una prova con una traccia indipendente e una collegata. |
| Routing Divisor | Min, Max e modulazione percorrono divisori musicali validi; i valori vengono quantizzati e stabilizzati vicino alle soglie. | Test C++ su Min/Max, quantizzazione, scrittura nella sequenza e isteresi. | Movimento lento di CV o CC vicino a una soglia, per escludere oscillazioni percepibili. |
| Scale Stochastic/Arp | Corretto il ripristino dei pitch slot cromatici legacy quando il progetto usa una scala non cromatica; lo stesso percorso è allineato in Arp. | Test C++ su pentatonica minore, note interne/esterne alla scala, Semitones, trasposizione e scale Voltage personalizzate. | Controllo di intonazione su un progetto 0.4.5 ricaricato. |
| Build Vagrant | Provisioning arrestato al primo errore, checksum SHA-256 dell'installer CMake verificato, dipendenze Python aggiornate. | Build pulita in VM già confermata dal contributore fino alla generazione di `UPDATE.DAT`. | Nessuna prova operativa sul modulo specifica. |

## Copertura già superata

- build desktop del simulatore;
- 47 test UI del simulatore;
- 13 eseguibili di test C++;
- build STM32 Release e generazione di `UPDATE.DAT`;
- build WebAssembly e caricamento reale del simulatore web nel browser;
- checksum del firmware, versione `0.4.6`, limiti della flash e vettori iniziali;
- controllo del sito e della coerenza tra release stabile `v0.4.5` e beta `v0.4.6-beta.1`.

Questi controlli verificano logica, compilazione e integrazione, ma non sostituiscono la prova di CV, gate, MIDI, encoder, tasti e SD sul Performer fisico.

## Smoke test ridotto dalla 0.4.5

Durata indicativa: 15–20 minuti, più una breve prova Launchpad se disponibile.

### 1. Preparazione

- [ ] Con la 0.4.5, aprire un progetto che contenga almeno Note, Curve, Stochastic o Arp, un routing Divisor e impostazioni MIDI riconoscibili.
- [ ] Fare un backup completo della SD o copiare separatamente il progetto originale. Non usare l'unica copia per il test.
- [ ] Annotare BPM, divisori, scala/root, pattern attivo e comportamento essenziale di CV/gate.

### 2. Aggiornamento e compatibilità

- [ ] Installare `UPDATE.DAT` della `v0.4.6-beta.1` e verificare che il modulo mostri `0.4.6`.
- [ ] Aprire la copia del progetto 0.4.5. Verificare tracce, pattern, BPM, routing, scale e uscite.
- [ ] Aprire Project e controllare che MIDI Notes, CC, Pgm, Bend e After risultino tutti `Yes`.

### 3. Funzioni modificate

- [ ] Su Note copiare alcuni step. Paste senza selezionare la destinazione deve mostrare `SELECT DEST`; dopo aver selezionato gli step di arrivo deve incollare correttamente.
- [ ] Duplicare una selezione continua e una con spazi. La nuova selezione deve spostarsi sulla copia; sull'ultimo step deve comparire `NO ROOM` senza modifiche. Ripetere rapidamente su Curve, Logic, Stochastic e Arp.
- [ ] Dalla pagina Track fare doppio clic su un tasto traccia e verificare l'ingresso nell'editor Steps corretto.
- [ ] In Performer attivare il Fill di una traccia indipendente e girare l'encoder. Deve cambiare soltanto il divisore temporaneo; al rilascio devono tornare ritmo e divisore originali. Il controllo Fill Amount deve continuare a modificare la quantità di Fill.
- [ ] Ripetere su una traccia collegata: deve continuare a seguire timing e divisore della traccia principale.
- [ ] Muovere lentamente il routing Divisor tra due divisioni. Devono comparire soltanto divisioni valide, senza continui salti avanti e indietro vicino alla soglia.
- [ ] Ricaricare un progetto con Stochastic o Arp su una scala non cromatica e verificare che le note restino nella scala e nel registro attesi.

### 4. MIDI e salvataggio

- [ ] Tenere una nota MIDI, impostare MIDI Notes su `No` e rilasciare la nota: il Note Off deve chiudere correttamente il gate. Nuovi Note On devono essere bloccati.
- [ ] Provare almeno un CC assegnato a un routing; se disponibili, provare anche Program Change, Pitch Bend e Aftertouch, alternando `Yes` e `No`.
- [ ] Salvare **la copia di prova con un nome nuovo**, riavviare e ricaricarla. Filtri MIDI e modifiche devono essere conservati; il divisore temporaneo del Fill non deve essere stato salvato.

### 5. Controllo finale

- [ ] Lasciare il progetto in riproduzione per almeno due minuti, cambiando pattern e usando Fill. Non devono verificarsi blocchi, riavvii o gate sospesi.
- [ ] Conservare intatta la copia originale 0.4.5. Se si ritorna alla versione stabile, ripartire da quella copia e non dal file salvato dalla beta.

## Memoria rispetto alla 0.4.5

Le due versioni sono state ricompilate con la stessa toolchain GNU ARM 6.3.1.

| Regione | Libera 0.4.5 | Libera 0.4.6-beta.1 | Delta |
|---|---:|---:|---:|
| Flash applicazione | 242216 B | 225016 B | −17200 B |
| SRAM dopo dati statici | 16244 B | 16228 B | **−16 B** |
| CCMRAM dopo dati statici | 14768 B | 14768 B | 0 B |

La beta lascia quindi **16 byte di SRAM statica in meno** rispetto alla 0.4.5. Restano 16228 B di SRAM e 14768 B di CCMRAM nelle rispettive regioni. Questi numeri non misurano il picco runtime di stack e heap, che richiede una misura sul modulo.

## Esito

Il delta software rispetto alla 0.4.5 è coerente con changelog e manuale e non mostra incompatibilità operative inattese oltre alla conversione irreversibile del progetto dopo il salvataggio in `Version41`. La beta è pubblicata per lo smoke test sul Performer; la validazione hardware resta ancora da completare e il feedback dei beta tester è particolarmente utile.
