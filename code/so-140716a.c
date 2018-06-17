/*Scrivere una applicazione C/POSIX che simula il comportamento del sistema
dei Cinque Filosofi a Cena (E. W. Dijkstra, 1965). I cinque filosofi sono seduti a cena ad un
tavolo rotondo. Davanti a ciascun filosofo c’è un piatto di spaghetti, e tra un piatto e l’altro
c’è una bacchetta (quindi in totale esistono cinque bacchette). Ciascun filosofo pensa per
un tempo casuale, poi cerca di prendere le bacchette a destra e sinistra del proprio piatto,
mangia per un tempo casuale, lascia le due bacchette, e torna a pensare. Ovviamente se l’una
o l’altra delle due bacchette non è disponibile il filosofo dovrà aspettare prima di iniziare a
mangiare.

L’applicazione deve fare uso di cinque processi, ciascuno dei quali realizza il comportamento
di un filosofo. In ciascuna iterazione ogni processo scrive in standard output l’istante di tempo
in cui inizia a pensare, smette di pensare, ed inizia a mangiare. Le “bacchette” condivise tra
i processi devono essere realizzate con opportuni meccanismi di sincronizzazione, e si devono
evitare le situazioni di stallo dovute a “deadlock”.

 Nota 1: per misurare il tempo si può utilizzare la API:
#include <time.h>
time_t time(time_t *t);

 In pratica time(NULL) restituisce il numero di secondi trascorsi da una data prefissata (1 gen-
naio 1970).

 Nota 2: per ottenere un valore casuale si può utilizzare la API:
#include <stdlib.h>
long random(void);

 In pratica random() restituisce un numero pseudo-casuale compreso tra 0 ed il valore RAND MAX.*/

/*L’esercizio richiede di simulare il comportamento dei cinque filosofi a cena utiliz-
zando cinque processi. È facile rendersi conto che per realizzare il programma si
deve prevedere un meccanismo di sincronizzazione che consenta di tenere trac-
cia dello stato libero/occupato di ciascuna delle cinque bacchette a disposizione
dei filosofi. Facciamo la scelta di utilizzare come primitiva di sincronizzazione
un set di cinque semafori IPC, un semaforo per ciascuna bacchetta.
L’altro problema da risolvere è quale strategia adottare per evitare le situazioni
di stallo (deadlock). Ciascun filosofo deve utilizzare le bacchette poste alla
propria destra ed alla propria sinistra.

 Se ciascun filosofo prende le bacchette in un ordine relativo prefissato, ad esem-
pio prende sempre prima la bacchetta a sinistra e poi quella a destra, allora
è possibile che si verifichino situazione di stallo. D’altra parte, se ciascun fi-
losofo prende sempre prima la bacchetta con indice minore (ossia se rispetta
l’ordine globale indotto dagli indici delle bacchette), nessuno stallo si verificherà
mai. Pertanto l’ordine con cui i filosofi prenderanno le bacchette dovrà essere
il seguente:
Filosofo     Prima bacchetta         Seconda bacchetta
0                    0                       4
1                    0                       1
2                    1                       2
3                    2                       3
4                    3                       4

Memorizziamo l’ordine in cui i processi dovranno acquisire i semafori in due
vettori costanti:*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define  num 5
const int first_sem[num] = {0, 0, 1, 2, 3};
const int second_sem[num] = {4, 1, 2, 3, 4};

time_t base_time;

//Il compito più importante della funzione initialize( ) è creare un nuovo set
//di cinque semafori IPC:

void  initialize(int *psemid)
{
    union semun {
        int val;
        struct semid_ds *buf;
        unsigned  short *array;
        struct seminfo *__buf;   //linux_specific
    } su;
    int sid, i;

    // create a new semaphore set with num semaphores
    sid = semget(IPC_PRIVATE, num, 0666);
    if (sid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    *psemid = sid;

    // initialize the semaphores as free mutexes
    su.val = 1;
    for (i=0; i<num; ++i) {
        int rc = semctl(sid, i, SETVAL, su);
        if (rc == -1) {
            perror("semctl");
            exit(EXIT_FAILURE);
        }
    }
    base_time = time(NULL);
}

int main()
{
    int semid;
    initialize(&semid);
    create_phils(semid);
    return EXIT_SUCCESS;
}