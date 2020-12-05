# Progetto di esempio con BeRTOS e meson build system

Questo è un progetto prototipo che permette di utilizzare BeRTOS come libreria esterna per una applicazione firmware generica.

## Descrizione generale

Questo repo contiene un'applicazione firmware prototipo che usa BeRTOS e un nuovo sistema di build (meson).

L'applicazione è un bootloader TFTP che deriva dal corrente bootloader usato sulle schede Comelz (che a sua volta deriva da quello usato sulle schede di ECM-Progressrail).

E' basato sui processori Cortex-M3 di ST (serie STM32F2) e utilizza il kernel, lo stack di rete lwip e vari driver hw delle CPU e generici di BeRTOS. L'ho scelto perché è un progetto mediamente complesso e utilizza più o meno tutti i servizi che BeRTOS mette a disposizione.

L'idea alla base era quella di estrarre completamente BeRTOS dall'applicazione e di renderlo una libreria esterna compilabile a parte. In questo modo l'applicazione utente può decidere di usare alcuni o tutti i servizi di BeRTOS, restandanone indipendente.

Nell'ottica di abbandonare l'uso del kernel di BeRTOS ma di poter comunque utilizzare eventuali driver e moduli utili, rendere BeRTOS un'entità a sé stante era d'obbligo.

## Il build system
Attualmente BeRTOS e l'applicazione utente sono un tutt'uno che viene compilato tutto insieme dai makefiles forniti con BeRTOS. È è l'applicazione a essere una parte di BeRTOS, quando invece vorremmo che fosse l'opposto.

Oltre a questo, i makefiles e le regole di build utilizzate sono abbastanza ostiche da comprendere; fare modifiche è sempre un bagno di sangue. Avere cose specifiche che interessano solo l'applicazione (per esempio: creazione di file binari in formato speciale per i vari bootloader), significa modificare i makefiles di BeRTOS stesso, rendendo queste necessità non scorporabili dal sistema operativo.

Visto l'obbiettivo di separare fortemente BeRTOS dall'applicazione con l'occasione ho deciso di valutare altri build system.

Alla fine è stato scelto **meson** perché tra tutti i sistemi valutati mi è sembrato quello megliore. Avevo escluso make e cmake perché entrambi ostici in modi diversi. 
Meson mi è piaciuto subito per la semplicità di utilizzo, la linearità e la comprensibilità della sintassi. Ho visto che si trovano diversi utenti e domande su reddit e stackoverflow, segno che inizia a essere abbastanza diffuso. Supporta la cross-compilazione in varie forme; ho fatto una ricerca e ho trovato anche alcuni progetti embedded che lo usano (tipo [uno proprio per STM32](https://github.com/hwengineer/STM32F3Discovery-meson-example))

E' scritto in python ed è sviluppato correntemente su github (https://github.com/mesonbuild/meson), questo mi fa ben sperare che sia mantenuto nel tempo.
Di default usa [ninja](https://ninja-build.org) come backend che è estremamente veloce.

La sintassi è molto pythonosa e non è turing-completa. Questa è una scelta deliberata: da una parte rende alcune cose un po' macchinose, dall'altra evita di sviluppare sistemi di build troppo complessi. Il vantaggio che se ne ottiene è quello di avere dei meson file parsabili dalle varie IDE, e quindi di avere il progetto integrato nel proprio editor.

