/*
Scrivere una applicazione C/POSIX multithread che simula il comportamento
del sistema dei Cinque Filosofi a Cena (E. W. Dijkstra, 1965). I cinque filosofi sono seduti
a cena ad un tavolo rotondo. Davanti a ciascun filosofo c’è un piatto di spaghetti, e tra un
piatto e l’altro c’è una bacchetta (quindi in totale esistono cinque bacchette). Ciascun filosofo
pensa per un tempo casuale, poi cerca di prendere le bacchette a destra e sinistra del proprio
piatto, mangia per un tempo casuale, lascia le due bacchette, e torna a pensare, continuando
cosı̀ all’infinito. Ovviamente se l’una o l’altra delle due bacchette non è disponibile il filosofo
dovrà aspettare prima di iniziare a mangiare.
L’applicazione deve fare uso di cinque thread, ciascuno dei quali realizza il comportamento
di un filosofo. In ciascuna iterazione ogni thread scrive in standard output l’istante di tempo
in cui inizia a pensare, smette di pensare, ed inizia a mangiare. Le “bacchette” condivise tra
i thread devono essere realizzate con opportuni meccanismi di sincronizzazione, e si devono
evitare le situazioni di stallo dovute a “deadlock”.

 Nota1: per misurare il tempo si può utilizzare la API:
 #include <time.h>
 time_t time(time_t *t);

 In pratica time(NULL) restituisce il numero di secondi trascorsi da una data prefissata (1 gennaio 1970)

 Nota2: per ottenere un valore casuale si può utilizzare la API:
 #include <stlib.h>
 long random(void);

 Un pratica random() restituisce un numero pseaudo-casuale tra 0 ed il valore RAND_MAX.
 */


//Memorizziamo l'ordine in cui i processi dovranno acquisire i mutex in due vettori costanti:

#define num 5
const int first_mux[num] = { 0, 0, 1, 2, 3 };   //prima bacchetta 
const int second_mux[num] = { 4, 1, 2, 3, 4 };  //seconda bacchetta

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

time_t base_time;
pthread_mutex_t chopsticks[num]; //bacchette

struct job {
    int thread_index;
    pthread_mutex_t *first_chopstick;
    pthread_mutex_t *second_chopstick;
};

struct job job_descr[num];


void acquire_mux(pthread_mutex_t * mux)
{
    if (pthread_mutex_lock(mux) != 0) {
        fprintf(stderr, "Error while acquiring the mutex\n");
        exit(EXIT_FAILURE);
    }
}

void release_mutex (pthread_mutex_t * mux)
{
    if (pthread_mutex_unlock(mux) != 0) {
        fprintf(stderr, "Error while releasing the mutex\n");
        exit(EXIT_FAILURE);
    }
}

void random_sleep(void)
{
    const int max_sleep = 20; //seconds
    int delay = random() % max_sleep + 1;
    while (delay != 0)
        delay = sleep(delay);
}


void *play_phil(void *arg)
{
    struct job *job = (struct job *) arg;
    int phn = job->thread_index;
    for (;;) {
        printf("P%d: start thinking at %ld\n", phn, now());
        random_sleep();
        printf("P%d: stop thinking at %ld\n", phn, now());
        acquire_mux(job->first_chopstick);
        acquire_mux(job->second_chopstick);
        printf("P%d: start eating at %ld\n", phn, now());
        random_sleep();
        realease_mux(job->second_chopstick);
        realease_mux(job->first_chopstick);
    }
}


void create_phils(void)
{
    int i;
    pthread_t p;

    for (i = 0; i < num; i++) {
        job_descr[i].thread_index = i;
        job_descr[i].first_chopstick = chopsticks + first_mux[i];
        job_descr[i].second_chopstick = chopsticks + second_mux[i];
        if ( pthread_create(&p, NULL, play_phil, (void *) (job_descr + i)) != 0) {
            fprintf(stderr, "Error in POSIX thread creation \n");
            exit(EXIT_FAILURE);
        }
    }
}


time_t now(void)
{
    return time(NULL) - base_time;
}


void initialize(void)
{
    int i;
    for (i = 0; i < num; ++i)
        if (pthread_mutex_init(chopsticks + i, NULL) != 0) {
            fprintf( stderr, "Error in mutex initialization\n");
            exit(EXIT_FAILURE);
        }
    base_time = time(NULL); //numero di secondi trascorsi dal 1 gennaio 1970
    srandom(base_time); 
}


int main()
{
    initialize();
    create_phils();
    pthread_exit(0);
}