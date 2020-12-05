# Progetto di esempio con BeRTOS e meson build system

Progetto prototipo che permette di utilizzare BeRTOS come libreria esterna per una applicazione firmware generica.

## TLDR

Per prima cosa è necessario creare un container docker, che può essere creato lanciando:
 - `./setup.sh`
 
Per compilare con meson:
 - `./compile.sh`
 
L'output di compilazione si troverà in `obj/meson`
 
La build funziona su linux e anche su mac.

## Descrizione generale

Questo repo contiene un'applicazione firmware prototipo che usa BeRTOS e un nuovo sistema di build (meson).

L'applicazione è un bootloader TFTP che deriva dal corrente bootloader usato sulle schede Comelz (che a sua volta deriva da quello usato sulle schede di ECM-Progressrail).

E' basato sui processori Cortex-M3 di ST (serie STM32F2) e utilizza il kernel, lo stack di rete lwip e vari driver hw delle CPU e generici di BeRTOS. L'ho scelto perché è un progetto mediamente complesso e utilizza più o meno tutti i servizi che BeRTOS mette a disposizione.

L'idea alla base era quella di estrarre completamente BeRTOS dall'applicazione e di renderlo una libreria esterna compilabile a parte. In questo modo l'applicazione utente può decidere di usare alcuni o tutti i servizi di BeRTOS, restandanone indipendente.

Nell'ottica di abbandonare l'uso del kernel di BeRTOS ma di poter comunque utilizzare eventuali driver e moduli utili, rendere BeRTOS un'entità a sé stante era d'obbligo.

## Meson build system
Attualmente BeRTOS e l'applicazione utente sono un tutt'uno che viene compilato tutto insieme dai makefiles forniti con BeRTOS. È l'applicazione a essere una parte di BeRTOS, quando invece vorremmo che fosse il contrario.

Oltre a questo, i makefiles e le regole di build utilizzate sono abbastanza ostiche da comprendere: fare modifiche è sempre un bagno di sangue. Avere build task specifiche che interessano solo l'applicazione (per esempio: creazione di file binari in formato speciale per i vari bootloader), significa modificare i makefiles di BeRTOS stesso, rendendo queste necessità non scorporabili dal sistema operativo.

Quando si hanno poi più applicazioni per uno stesso progetto firmware (per esempio più schede), i relativi fw sono compilati tutti insieme, in un'unica soluzione. Anche questo non è auspicabile.

Visto quindi l'obbiettivo di separare BeRTOS dall'applicazione, con l'occasione ho deciso di valutare altri build system.

Alla fine ho scelto **meson** perché tra tutti i sistemi valutati mi è sembrato quello migliore. Avevo escluso make e cmake perché entrambi ostici in modi diversi. Qmake era carino, ma troppo Qt-dipendente.

Meson mi è piaciuto subito per la semplicità di utilizzo, la linearità e la comprensibilità della sintassi. Ho visto che si trovano diversi utenti e domande su reddit e stackoverflow, segno che inizia a essere abbastanza diffuso. Supporta la cross-compilazione in varie forme; ho fatto una ricerca e ho trovato anche alcuni progetti embedded che lo usano (tipo [uno proprio per STM32](https://github.com/hwengineer/STM32F3Discovery-meson-example))

E' scritto in python ed è sviluppato correntemente su github (https://github.com/mesonbuild/meson), questo mi fa ben sperare che sia mantenuto nel tempo.
Di default usa [ninja](https://ninja-build.org) come backend che è estremamente veloce.

La sintassi è molto pythonosa e **non** è turing-completa. Questa è una scelta deliberata: da una parte rende alcune cose un po' macchinose, dall'altra evita di sviluppare sistemi di build troppo complessi. Il vantaggio che se ne ottiene è quello di avere dei meson file parsabili dalle varie IDE, e quindi di avere il progetto integrato nel proprio editor.

## Dettagli implementativi del build system

Per separare meglio le cose, ho scelto di avere ogni progetto firmware in un progetto meson a sé stante, singolo.

Meson è abbastanza rigido sulle sue convenzioni. Prevede che ogni progetto sia tutto completamente contenuto all'interno di una directory. Nel caso del progetto di esempio, la nostra applicazione si trova dentro la directory `boot`. Usa però anche dei files che sono a comune ad (eventuali) altre applicazioni, che sono nella directory `common`, al suo stesso livello.

Ho quindi fatto un soflink a `common` dentro `boot` per permettere di accederci.

Il file che descrive il progetto si deve chiamare `meson.build` ed è localizzato all'interno della directory dell'applicazione `boot`. Ogni file `meson.build` può eventualmente includere altri file `meson.build` che a loro volta si trovano in sottodirectory del progetto. Questa feature non è usata nell'applicazione, ma in BeRTOS sì.

Per effettuare la cross-compilazione, meson prevede di avere un file di configurazione apposito. Sempre dentro la directory `boot` è presente il file `cross.build` con le dovute opzioni. In questo modo, ogni progetto può avere la propria configurazione di cross-compilazione.

BeRTOS è trattato come un progetto meson a sé stante, e volendo può essere compilato separatamente ed indipendentemente: trovate il suo `meson.build` file dentro la directory `bertos`. L'output di compilazione è una libreria, `libbertos.a`, che poi può essere linkata dai progetti che vogliono usarla.

La nostra applicazione ha come dipendenza BeRTOS stesso, che nel dettaglio viene visto come un meson *subproject*.
Anche i subproject devono stare dentro la directory dell'applicazione, per la precisione all'interno della directory `subprojects`. Siccome la directory di `bertos` è allo stesso livello dell'applicazione, ho creato anche qui un softlink dentro la directory `boot/subprojects`.

### Dipendenza dell'applicazione da BeRTOS
Dentro il `meson.build` file dell'applicazione `boot` è possibile vedere che BeRTOS viene importato come subproject e viene dichiarato che la nostra app dipende da esso. Questo è necessario ogni qualvolta si voglia importare e usare BeRTOS. Questa dipendenza farà sì che vengano aggiunti i files, i flags e le librerie giuste per poter compilare e linkare `boot` con la `libbertos`. Questo avviene tutto automagicamente grazie alle dipendenze di meson.

### Configurazione di BeRTOS
BeRTOS può essere configurato tramite il `meson.build` file dell'applicazione. Quando si fa l'importazione del subproject è possibile vedere come viene configurato.
Ci sono 3 cose che è possibile specificare:
 - Moduli di BeRTOS (necessario)
 - Scelta CPU (necessario)
 - Frequenza CPU (opzionale)
 
#### Moduli di BeRTOS
Per attivare le funzionalità di BeRTOS ho creato il contetto di "modulo". Per modulo si intende una stringa che viene passata al progetto BeRTOS per permettere l'inclusione di una feature. In generale attivare un modulo di BeRTOS significa in pratica aggiungere i files/flags del modulo alla compilazione della libreria `libbertos`. 

Avrei potuto fare in modo che venisse sempre compilato tutto, ma i tempi di compilazione sarebbero aumentati di molto. Inoltre ci sono delle cose che vanno abilitate alternativamente e non sarebbe stato possibile avere sempre tutto attivo contemporaneamente.

Per adesso ho definito solo i moduli che mi servivano alla compilazione dell'applicazione, ma in generale aggiungerne altri è molto semplice e può essere fatto in maniera incrementale via via che si rende necessario. 

##### Il modulo `cpu_core`
Il modulo `cpu_core` è l'unico che è particolare rispetto agli altri. Includerlo significa che vogliamo usare le funzioni core di BeRTOS, ovvero:
 - I driver che dipendono da periferiche specifiche del micro (timer, ethernet, etc...)
 - Eventuali routine di startup assembler
 - Eventuali linker script specifici per questo micro
 - Il kernel di BeRTOS (e le routine di context switch specifiche in C/asm)

Va quindi incluso solo se vogliamo usare BeRTOS come vero RTOS della macchina.
Non è necessario includerlo per usare un modulo generico (come crc, fifo, hashing, etc...)

##### Dipendenza tra i moduli
Le dipendenze tra i moduli **non** sono risolte automaticamente. Se per esempio il modulo `tftp` dipende da `lwip` sarà necessario includerli entrambi esplicitamente. Se si aggiunge il modulo `timer` sarà necessario includere anche `cpu_core`, per via della sua dipendenza dalle periferiche della CPU, e così via. Sarebbe pensabile aggiungere anche un sistema di gestione automatica delle dipendenze, magari tramite uno script python esterno, ma renderebbe il progetto non parsabile dalle IDE e in definitia il guadagno sarebbe solo minimo.

#### Scelta CPU
Per usare BeRTOS è necessario specificare quale CPU stiamo utilizzando. Attualmente è supportata solo la CPU del progetto di esempio la `STM32F207IG`. Per aggiungere altre CPU è necessario modificare il file `bertos/cpu/meson.build` e, i file `meson.build` presenti nelle sottodirectory dei vari core (per esempio in `bertos/cpu/cortex-m3`).

#### Frequenza CPU
Specificare la frequenza CPU è necessario solo se si utilizzano moduli che richiedono di avere accesso l'hardware (per esempio timer, adc, ethernet, etc.). 


### Propagazione delle opzioni di compilazione
Essendo BeRTOS un subproject dell'applicazione, eredita da essa le configurazioni: il compilatore da utilizzare, il livello di ottimizzazione, l'attivazione della modalità debug, etc.
Allo stesso tempo, essendo un progetto separato, utilizza anche dei flag di compilazione specifici. Questo permette di tunare con estrema precisione quali flags vogliamo attivi a livello globale, quali solo nel nostro progetto e quali specificatamente per BeRTOS.


## Cosa manca
Prima di mettere tutto eventualmente in produzione, il sistema di configurazione (i file `_cfg.h`) e la modalità con cui si specifica la parte di basso livello dei driver (directory `hw` dell'applicazione), vanno completamente rivisti.

In particolare, per l'utilizzo della directory `hw`, il vecchio sistema di build di BeRTOS usa un "trucco": include la directory `hw` dell'applicazione tra i path di ricerca _prima_ dei path interni di BeRTOS. Durante la compilazione vengono così trovati i file hw specifici della app.

Passando a usare meson, essenso BeRTOS un sottoprogetto completamente separato, non è consentito passargli nessun file esplicitamente. Può eventualmente ricrearsi durante la build quello che necessita, ma non può utilizzare file esterni (perché altrimenti non sarebbe indipendente).

Per adesso ho bovinamente copiato le directory `hw` e `cfg` dall'applicazione quando si crea per la prima volta la directory di build. Questo funziona solo in parte, perché se si modifica qualcosa in `hw` o `cfg` è necessario riconfigurare completamente la build (e infatti durante la configurazione salta fuori un warning).

Andrebbe secondo me cambiata l'architettura di questa cosa. Per i file cfg potremmo stabilire un default e passare a BeRTOS solo le opzioni che sono diverse da esso tramite il meccanismo di opzioni di configurazione di meson.

Per la parte hw, trattandosi di macro che contengono codice, sinceramente non ho idee valide. Potremmo trasformarle in funzioni/callback da passare, ma perderemmo tantissimo in prestazioni. Sono aperto a nuove idee su questo.
