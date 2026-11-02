# PER|FORMER — verifica del delta operativo 0.4.5 → 0.4.6-beta.2

Stato della verifica: **build e test locali superati; i due cambiamenti della beta.2 non sono ancora verificati sul modulo**. Il blocco all'avvio con una SD esistente segnalato in [#22](https://github.com/VinxScorza/performer/issues/22) resta da chiarire.

La baseline è il tag `0.4.5`. Per il firmware, il tag contiene gli stessi sorgenti del commit di release `cfc19cf9`; il commit successivo riguarda soltanto presentazione e licenza.

> **ATTENZIONE — prima di provare la beta fare un backup della SD.** La 0.4.6-beta.2 apre i progetti della 0.4.5, ma al primo salvataggio li converte da `Version40` a `Version41`. Da quel momento la 0.4.5 e le versioni precedenti non riescono più a rileggerli: il controllo del file fallisce e il progetto viene scartato. Conservare quindi una copia originale dei progetti 0.4.5 separata da quella usata con la beta.

## Delta funzionale

| Area | Differenza rispetto alla 0.4.5 | Verifica eseguita | Da verificare sul modulo |
|---|---|---|---|
| Progetti | Il formato passa a `Version41` per salvare i filtri MIDI. I progetti `Version40` si aprono con tutti i nuovi filtri attivi. | Percorso di lettura e inizializzazione controllato; build STM32 riuscita. | Apertura di un progetto reale 0.4.5, salvataggio con un altro nome e ricaricamento. |
| MIDI input | Nuovi filtri per Notes, CC, Program Change, Pitch Bend e Aftertouch. La beta.2 impedisce anche ai Note On filtrati di modificare gli step selezionati nell'editor Note. Note Off resta accettato; la mappatura Program Change resta fissa. | Test UI sulla modifica degli step e test automatici su filtri e Note On/Off; CC e Program Change confermati su hardware nella beta.1. | Ripetere il caso Note Trig/step selezionati su hardware con `MIDI Notes = No`; provare Bend, Aftertouch e persistenza dei filtri. |
| Copy/Paste/Duplicate | Comportamento uniforme nei cinque editor Note, Curve, Stochastic, Logic e Arp. Paste richiede una destinazione esplicita; Duplicate conserva gli spazi della selezione, estende Last Step e segnala `NO ROOM`. | Test C++; funzionamento generale confermato su hardware nella beta.1. | Eventuale passaggio aggiuntivo nei singoli editor non coperti dal resoconto hardware. |
| Navigazione | Il doppio clic su `T1..T8` entra direttamente nell'editor Steps quando la pagina non usa già quei tasti per un'altra funzione. | Test UI e conferma su hardware nella beta.1. | Nessuna nuova prova specifica per la beta.2. |
| Fill | Durante il Fill l'encoder cambia temporaneamente il divisore senza alterare quello salvato. Nella beta.2 funziona anche senza tenere premuto il tasto dopo aver agganciato Fill; con più Fill vale l'ultimo agganciato, o tutte le tracce Fill attive se l'ultimo è globale. | Test UI sulle priorità e sul ripristino; Fill momentaneo, agganciato e globale confermati su hardware nella beta.1. | Provare l'encoder con Fill agganciato, più tracce, Fill globale e ritorno al divisore salvato dopo lo sgancio. |
| Tracce collegate | Il comportamento resta quello della 0.4.5: una traccia collegata eredita timing e divisore dalla traccia principale. Il nuovo divisore Fill agisce sulle tracce indipendenti e non sostituisce quello ereditato. | Percorsi di playback controllati nel codice e comportamento documentato nel manuale. | Una prova con una traccia indipendente e una collegata. |
| Routing Divisor | Min, Max e modulazione percorrono divisori musicali validi; i valori vengono quantizzati e stabilizzati vicino alle soglie. | Test C++; confermato su hardware con CC da Octatrack nella beta.1. | Eventuale verifica aggiuntiva con CV vicino a una soglia. |
| Scale Stochastic/Arp | Corretto il ripristino dei pitch slot cromatici legacy quando il progetto usa una scala non cromatica; lo stesso percorso è allineato in Arp. | Test C++; comportamento Stochastic/Arp confermato su hardware nella beta.1. | Eventuale controllo di migrazione su un progetto reale 0.4.5. |
| Build Vagrant | Provisioning arrestato al primo errore, checksum SHA-256 dell'installer CMake verificato, dipendenze Python aggiornate. | Build pulita in VM già confermata dal contributore fino alla generazione di `UPDATE.DAT`. | Nessuna prova operativa sul modulo specifica. |

## Copertura già superata

- build desktop del simulatore;
- 53 test UI del simulatore;
- 13 eseguibili di test C++;
- build STM32 Release e generazione di `UPDATE.DAT`;
- build WebAssembly e caricamento della nuova build beta.2 nel browser, con display funzionante e senza errori in console;
- checksum del firmware, versione `0.4.6`, limiti della flash e vettori iniziali;
- controllo dei link del sito e della coerenza tra release stabile `v0.4.5` e beta `v0.4.6-beta.2`.

Questi controlli verificano logica, compilazione e integrazione, ma non sostituiscono la prova di CV, gate, MIDI, encoder, tasti e SD sul Performer fisico.

## Smoke test ridotto dalla 0.4.5

Durata indicativa: 15–20 minuti, più una breve prova Launchpad se disponibile.

### 1. Preparazione

- [ ] Con la 0.4.5, aprire un progetto che contenga almeno Note, Curve, Stochastic o Arp, un routing Divisor e impostazioni MIDI riconoscibili. Preparare anche una SD di prova separata: in #22 è stato segnalato un blocco all'avvio con una SD già usata.
- [ ] Fare un backup completo della SD o copiare separatamente il progetto originale. Non usare l'unica copia per il test.
- [ ] Annotare BPM, divisori, scala/root, pattern attivo e comportamento essenziale di CV/gate.

### 2. Aggiornamento e compatibilità

- [ ] Installare `UPDATE.DAT` della `v0.4.6-beta.2` e verificare che il modulo mostri `0.4.6`.
- [ ] Verificare l'avvio prima con la SD di prova, poi con una copia della SD abituale. Se si blocca su `LOADING`, non alterare l'originale: annotare la differenza e riprovare sulla copia rinominando `LAST.DAT`.
- [ ] Aprire la copia del progetto 0.4.5. Verificare tracce, pattern, BPM, routing, scale e uscite.
- [ ] Aprire Project e controllare che MIDI Notes, CC, Pgm, Bend e After risultino tutti `Yes`.

### 3. Funzioni modificate

- [ ] Su Note copiare alcuni step. Paste senza selezionare la destinazione deve mostrare `SELECT DEST`; dopo aver selezionato gli step di arrivo deve incollare correttamente.
- [ ] Duplicare una selezione continua e una con spazi. La nuova selezione deve spostarsi sulla copia; sull'ultimo step deve comparire `NO ROOM` senza modifiche. Ripetere rapidamente su Curve, Logic, Stochastic e Arp.
- [ ] Dalla pagina Track fare doppio clic su un tasto traccia e verificare l'ingresso nell'editor Steps corretto.
- [ ] In Performer attivare il Fill di una traccia indipendente e girare l'encoder. Deve cambiare soltanto il divisore temporaneo; al rilascio devono tornare ritmo e divisore originali. Il controllo Fill Amount deve continuare a modificare la quantità di Fill.
- [ ] Agganciare Fill con `SHIFT + S9`, poi girare l'encoder senza tenere premuto il tasto: cambia il divisore temporaneo di T1. Agganciare un'altra traccia: l'encoder segue l'ultima. Agganciare Fill globale con `SHIFT + F4`: l'encoder agisce sulle tracce Fill attive. Sganciare e verificare il ritorno ai divisori salvati.
- [ ] Ripetere su una traccia collegata: deve continuare a seguire timing e divisore della traccia principale.
- [ ] Muovere lentamente il routing Divisor tra due divisioni. Devono comparire soltanto divisioni valide, senza continui salti avanti e indietro vicino alla soglia.
- [ ] Ricaricare un progetto con Stochastic o Arp su una scala non cromatica e verificare che le note restino nella scala e nel registro attesi.

### 4. MIDI e salvataggio

- [ ] Tenere una nota MIDI, impostare MIDI Notes su `No` e rilasciare la nota: il Note Off deve chiudere correttamente il gate. Nuovi Note On devono essere bloccati.
- [ ] Aprire l'editor Note, selezionare gli step con doppio `SHIFT` e inviare Note Trig MIDI. Con `MIDI Notes = No`, pitch e gate selezionati devono restare invariati; con `Yes`, il normale inserimento da MIDI deve tornare disponibile.
- [ ] Provare almeno un CC assegnato a un routing; se disponibili, provare anche Program Change, Pitch Bend e Aftertouch, alternando `Yes` e `No`.
- [ ] Salvare **la copia di prova con un nome nuovo**, riavviare e ricaricarla. Filtri MIDI e modifiche devono essere conservati; il divisore temporaneo del Fill non deve essere stato salvato.

### 5. Controllo finale

- [ ] Lasciare il progetto in riproduzione per almeno due minuti, cambiando pattern e usando Fill. Non devono verificarsi blocchi, riavvii o gate sospesi.
- [ ] Conservare intatta la copia originale 0.4.5. Se si ritorna alla versione stabile, ripartire da quella copia e non dal file salvato dalla beta.

## Memoria rispetto alla 0.4.5

Le due versioni sono state ricompilate con la stessa toolchain GNU ARM 6.3.1.

| Regione | Libera 0.4.5 | Libera 0.4.6-beta.2 | Delta |
|---|---:|---:|---:|
| Flash applicazione | 242216 B | 223004 B | −19212 B |
| SRAM dopo dati statici | 16244 B | 16228 B | **−16 B** |
| CCMRAM dopo dati statici | 14768 B | 14756 B | −12 B |

La beta lascia quindi **16 byte di SRAM statica in meno** rispetto alla 0.4.5. Rispetto alla beta.1, la SRAM è invariata, mentre la CCMRAM libera diminuisce di 12 byte e la flash libera di 2012 byte. Questi numeri non misurano il picco runtime di stack e heap, che richiede una misura sul modulo.

## Esito

La beta.2 è in preparazione. I due cambiamenti nuovi superano i controlli locali, ma mancano la prova sul Performer e la diagnosi dell'avvio con la SD segnalato in #22. Non considerare questa build una release stabile; mantenere intatta la copia dei progetti precedente al salvataggio in `Version41`.
