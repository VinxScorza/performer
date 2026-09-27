# PER|FORMER — smoke test di aggiornamento 0.4.4 → 0.4.6-beta.1

Durata indicativa: 25–35 minuti, più la prova Launchpad facoltativa. Esito hardware: DA ESEGUIRE.
Partenza: firmware 0.4.4 e progetti salvati con quella versione. Il percorso è diretto alla 0.4.6-beta.1, senza installazione intermedia della 0.4.5. Il firmware installato mostra 0.4.6; il suffisso beta appartiene al tag e al pacchetto di release. La checklist include anche le novità e le correzioni della 0.4.5, che resta la versione stabile.

> **ATTENZIONE — compatibilità dei progetti:** la 0.4.6 beta apre i progetti di 0.4.4 e 0.4.5, ma quando li salva li aggiorna al formato `Version41`. La 0.4.5 e le versioni precedenti non possono riaprire il file aggiornato. Prima della prova fare una copia completa della SD oppure conservare separatamente una copia di ogni progetto originale.

## Riferimento iniziale sulla 0.4.4

- [ ] Confermare 0.4.4 in System. Salvare un progetto di riferimento con almeno Note, Curve, Arp e Stochastic, due pattern, un routing CV/CC e impostazioni di scala/root riconoscibili. Fare una copia della SD prima dell'aggiornamento.
- [ ] Annotare BPM, sorgente clock, divisori, First/Last, scale/root, mute e configurazione MIDI. Registrare o annotare una breve riproduzione delle tracce deterministiche. Sulle tracce casuali confrontare scala, registro e controlli, non pretendere la stessa successione di note.
- [ ] Verificare Play/Stop, cambio pattern, ingresso in Note/System e apertura della lista Load. Questo è il riferimento per riconoscere eventuali regressioni dopo l'aggiornamento.

## Installazione e avvio

- [ ] Copiare il file del pacchetto `v0.4.6-beta.1` nella root della SD con nome `UPDATE.DAT`.
- [ ] Accendere tenendo premuto l'encoder, oppure entrare in System → Update e tenere premuto l'encoder. Il bootloader deve riconoscere la versione 0.4.6 e verificare il file. Confermare YES e attendere il completamento senza togliere alimentazione.
- [ ] Verificare 0.4.6 in System, display e pulsanti funzionanti, nessun riavvio inatteso.
- [ ] Caricare la copia del progetto 0.4.4: pattern, tracce, routing, BPM, divisori e impostazioni MIDI devono essere presenti. Tutti i nuovi filtri MIDI devono inizialmente risultare abilitati. Avviare e fermare la riproduzione, controllando CV e gate e confrontando il riferimento; le correzioni di scala Arp/Stochastic vanno verificate nella sezione dedicata.

## Novità e correzioni introdotte dopo la 0.4.4, incluse nella 0.4.5

- [ ] Su Note aprire Acid → Eucl Phrase. Generare una frase, variare i parametri euclidei e confrontare originale/anteprima con A/B. Cancel deve ripristinare gli step originali; rientrare, generare e usare Apply deve conservare la nuova frase. Verificare anche dal Launchpad Generators Mode, se disponibile.
- [ ] Su una copia del pattern, aprire Chaos → Wreck Pattern, generare, provare A/B e Cancel. Ripetere con Apply, poi entrare in Note e System. Nessun riavvio, blocco o modifica rimasta dopo Cancel. Ripetere il ciclo almeno cinque volte per intercettare i problemi di stabilità più evidenti.
- [ ] Sul routing provare un parametro continuo (ad esempio tempo), uno discreto (pattern o divisore) e uno booleano (mute/fill): movimento continuo nel primo caso, scatti validi nel secondo, attivazione/disattivazione stabile nel terzo. Provare una destinazione Curve su una traccia Note: l'abbinamento deve essere rifiutato o ignorato, senza corruzione della traccia.
- [ ] Aprire e scorrere Load con la SD abituale, caricare due progetti e provare Save As. Nessun riavvio o blocco; il progetto originale 0.4.4 deve restare intatto.

## Editing e navigazione

- [ ] Su Note, creare quattro step riconoscibili, selezionarli e usare Copy. La selezione deve sparire; Paste senza destinazione deve mostrare `SELECT DEST` e lasciare invariato il pattern. Selezionare quattro destinazioni e incollare: devono corrispondere alla sorgente.
- [ ] Selezionare gli step 1–4 e usare Duplicate: copia su 5–8, nuova selezione su 5–8, Last Step esteso se necessario. Provare anche una selezione non contigua: la distanza fra gli step deve essere conservata.
- [ ] Selezionare solo l'ultimo step disponibile e duplicare: `NO ROOM`, nessuna modifica. Senza selezione, verificare la duplicazione classica del range First/Last.
- [ ] Ripetere copia/incolla e duplicazione almeno una volta su Curve, Logic, Stochastic e Arp.
- [ ] Dalla pagina Track, doppio clic su un tasto traccia: ingresso nell'editor della traccia corretta. Dalla pagina Project, singolo clic sulla traccia: ingresso in Steps.

## Fill temporaneo

- [ ] Impostare una Note con ritmo regolare e divisore noto. In Performer, tenere premuto S9 (Fill T1), girare l'encoder: cambia la velocità della traccia, non il BPM globale. Rilasciare S9: ritorna il divisore originale; verificare anche il valore nella pagina Sequence.
- [ ] Tenere S1 e girare l'encoder: cambia la quantità di Fill di T1, non il divisore e non il BPM.
- [ ] Provare il Fill globale e ripetere il cambio di velocità su Curve, Logic, Stochastic e Arp con sequenze indipendenti, senza track link. Nessun blocco o gate rimasto alto al termine della prova.
- [ ] Se disponibile, attivare il Fill di una traccia dal Launchpad e girare l'encoder del modulo; verificare variazione temporanea e ritorno al rilascio. Disconnettere il controller dopo il rilascio e verificare il normale uso dell'encoder.

## MIDI e scale

- [ ] Collegare una sorgente MIDI alla porta e al canale configurati. Con `MIDI Notes` abilitato, tenere una nota, disabilitare il filtro e rilasciare la nota: il Note Off deve essere accettato e la nota/gate deve chiudersi. Con il filtro disabilitato, verificare che nuovi Note On siano bloccati; riabilitarlo e riprovare.
- [ ] Con un CC assegnato a un routing riconoscibile, verificare che `MIDI CC = No` ne blocchi l'effetto e `Yes` lo ripristini. Provare allo stesso modo Program Change con integrazione abilitata, Pitch Bend e Aftertouch con destinazioni configurate.
- [ ] Salvare il progetto con alcuni filtri disabilitati, ricaricarlo e verificare che i valori siano conservati.
- [ ] Su Arp e Stochastic, provare la pentatonica minore di Do con posizioni cromatiche legacy/bypass: Mi bemolle deve restare Mi bemolle e Do diesis deve essere escluso dalla selezione delle note. Ascoltare o misurare l'intonazione; poi verificare anche la modalità Semitones.
- [ ] Assegnare CV o CC al Divisor, muoverlo lentamente e fermarsi vicino a un cambio di divisione. Le divisioni devono essere valide e non oscillare continuamente con piccole fluttuazioni del segnale.

## Chiusura

- [ ] Lasciare suonare per almeno due minuti cambiando pattern e usando Fill. Nessun crash, riavvio o arresto inatteso.
- [ ] Salvare con un nome nuovo, riavviare e ricaricare: note, routing e filtri MIDI devono essere conservati; il divisore temporaneo del Fill non deve alterare quello memorizzato nella sequenza.

Annotare per ogni errore: tipo di traccia, pattern, clock interno/esterno, controller collegati, sequenza di tasti e risultato osservato. Il superamento di questa checklist è uno smoke test, non una validazione completa di tutte le funzioni.
