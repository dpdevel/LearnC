/* Esercizio 1. Scrivere una applicazione C/POSIX multithread costituita da tre thread che
implementano le seguenti procedure:

 T1) Il primo thread inserisce in un buffer circolare B1 i valori interi x-(x*x*x)%8+1 per x
uguale a 0, 1, 2, ecc. Il buffer B1 può contenere al massimo 1024 numeri interi, quindi
se necessario il thread T1 deve bloccare in attesa che venga liberato spazio in esso.

 T2) Il secondo thread inserisce in un buffer circolare B2 i valori interi x+(x*x)%7 per x
uguale a 0, 1, 2, ecc. Il buffer B2 può contenere al massimo 1024 numeri interi, quindi
se necessario il thread T2 deve bloccare in attesa che venga liberato spazio in esso.

 T3) Il terzo thread scandisce e confronta gli elementi contenuti nei due buffer B1 e B2 in
modo sincrono (il primo elemento di B1 con il primo elemento di B2, il secondo con il
secondo, ecc.). Se necessario T3 deve bloccare in attesa che venga inserito un elemento
in uno dei buffer. Se per un certo valore di x il valore risultante nei due buffer è identico,
T3 stampa tale valore in standard output. Dopo ogni confronto T3 rimuove gli elementi
corrispondenti dai due buffer.

 I thread debbono poter eseguire il più possibile in parallelo tra loro, ma si deve evitare ogni
possibile “race condition” dovuta agli accessi ai buffer circolari condivisi.

 A titolo di esempio, i primi valori stampati dal thread T3 dovrebbero essere
        7, 9, 21, 23, 35, 37, 49, 49, 51, 63, 65, 77, 79, 91, 93, . . .

 corrispondenti ai valori di x
        6, 8, 20, 22, 34, 36, 48, 49, 50, 62, 64, 76, 78, 90, 92, . . . */


/* Le strutture di dati principali del programma sono due buffer circolari, ciascu-
no dei quali è acceduto esclusivamente da due thread, uno in lettura ed uno in
scrittura. Un buffer circolare acceduto unicamente da un lettore e da uno scrit-
tore non presenta particolari problemi di “race condition”. Pertanto, è possibile
realizzare una soluzione che non fa uso di alcuna primitiva di sincronizzazione.
Svolgiamo l’esercizio seguendo un approccio “top-down”. Iniziamo con la defi-
nizione della struttura di dati necessaria per implementare i due buffer circolari: */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define cbuf_size 1025
struct double_circular_buffer {
    int E1;
    int E2;
    int S;
    unsigned int buf1[cbuf_size];
    unsigned int buf2[cbuf_size];
};

struct double_circular_buffer db;

/*Si noti che la dimensione dei buffer è pari ad un elemento in più di quanto
richiesto; infatti, per semplificare la logica di controllo e distinguere facilmente
i casi “buffer pieno” e “buffer vuoto” dovrà sempre esserci almeno una posizione
del buffer libera.

 I campi E1 e E2 rappresentano la fine dei dati rispettivamente nel primo e nel
secondo buffer. Il campo S rappresenta l’inizio dei dati in entrambi i buffer:
infatti le letture sono realizzate dal thread T 3 e coinvolgono sempre la stessa
posizione relativa nei due buffer.

 Un’altra struttura di dati necessaria per il programma è il descrittore del lavoro
che deve essere compiuto dai thread T 1 e T 2 : */

struct job_t {
    unsigned int *buf;
    int *pE, *pS;
    unsigned int (*expr)(unsigned int);
};
struct job_t job1, job2;


/*La struttura job t contiene puntatori al buffer circolare su cui dovrà operare
il thread; inoltre contiene il puntatore expr ad una funzione che implementa la
diversa espressione matematica utilizzata dai thread T 1 e T 2 .*/



//le funzioni che implementano le espressioni matematiche:

unsigned int expr1(unsigned int x)
{
    return x-(x*x*x)%8+1;
}
unsigned int expr2(unsigned int x)
{
    return x+(x*x)%7;
}


/*La funzione fill_buffer() è comune ad entrambi i thread creati
 * entro main(). Ha il compito di riempire un buffer circolare con i
 * dati calcolati per mezzo dell'espressione matematica:*/

void *fill_buffer(void *p)
{
    struct job_t *job = (struct job_t *) p;
    unsigned int *buf = job->buf;
    int *pE = job->pE;
    volatile int *pS = job->pS;
    unsigned int x, v;

    for (x=0; ;++x) {
        int nE = (*pE + 1) % cbuf_size;
        v = job->expr(x);
        //controllare se buffer pieno
        while (nE == *pS)
            ; // busy wait
        buf[*pE] = v;
        *pE = nE;
    }
    //NOT REACHED
}


/*Nel caso in cui il buffer circolare sia pieno (ossia vi sia una sola posizione libera),
la funzione entra in un ciclo “busy wait” in cui attende che il thread di lettura
renda libera una nuova posizione del buffer incrementando la variabile puntata
da job->pS. A tale riguardo si noti come questo puntatore venga assegnato
ad una variabile locale di tipo volatile int *. La parola chiave volatile
informa il compilatore che il contenuto della cella di memoria referenziata dal
puntatore può essere modificato da un altro flusso di esecuzione esterno alla
funzione. In assenza di volatile, il compilatore potrebbe generare codice ot-
timizzato che non rileva accessi alla cella e quindi evita di rileggere ad ogni
ciclo il valore nella cella di memoria, con conseguente funzionamento erroneo
del programma.*/


/*Infine la funzione scan buffers( ) implementa il lavoro affidato al thread
T 3 : legge gli elementi di entrambi i buffer, controllando i valori coincidenti,
e contemporaneamente libera lo spazio occupato:*/

void scan_buffers(struct double_circular_buffer *db)
{
    volatile int *pE1 = &db->E1;
    volatile int *pE2 = &db->E2;
    for (;;) {
        int S = db->S;
        while (S == *pE1 || S == *pE2)
            ; //busy wait: a circular buffer is empty
        if (db->buf1[S] == db->buf2[S])
            printf("%u\n", db->buf1[S]);
        //remove the elements from the buffer
        db->S = (S+1) % cbuf_size;
    }
}


/*Anche in questo caso si invoca un “busy wait” nel caso in cui uno od entrambi
i buffer circolari sia vuoto, e si utilizza la parola chiave volatile per accede-
re ai campi che sono aggiornati in modo asincrono dagli altri due thread del
programma.*/


/* La funzione main( ) deve creare i due thread T 1 e T 2 , poi prosegue eseguendo
il lavoro affidato al thread T 3 :*/

int main(void)
{
    int s;
    pthread_t t;

    db.S = db.E1 = db.E2 = 0;

    // Thread T1 acts on buffer b1 and expressione expr1
    job1.buf = db.buf1;
    job1.pE = &db.E1;
    job1.pS = &db.S;
    job1.expr = expr1;
    s = pthread_create(&t, NULL, fill_buffer, &job1);
    if (s != 0) {
        fprintf(stderr, "Error in pthread_create\n");
        exit(EXIT_FAILURE);
    }

    // Thread T2 acts on buffer b2 and expression expr2
    job2.buf = db.buf2;
    job2.pE = &db.E2;
    job2.pS = &db.S;
    job2.expr = expr2;
    s = pthread_create(&t, NULL, fill_buffer, &job1);
    if (s != 0) {
        fprintf(stderr, "Error in pthread_create\n");
        exit(EXIT_FAILURE);
    }

    // The initial thread plays as T3
    scan_buffer(&db);
    return 0; //never reached
}


