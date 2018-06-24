/*Esercizio 2. Modificare il programma dell’esercizio precedente in modo tale esso utilizzi
8 thread POSIX, ciascuno dei quali esamina un diverso numero letto dal file. (Si noti che
in generale il file contiene più di 8 numeri, perciò ciascun thread, dopo aver processato un
numero, deve leggere il successivo numero da analizzare dal file.) Si raccomanda di curare gli
aspetti di sincronizzazione relativi alla lettura dal file ed alla scrittura sullo standard output.*/

/* Il programma dell'esercizio prcedente deve essere modificato per effettuare l'elaborazione
 * utilizzando 8 thread POSIX.
 * E' necessario includere l'header file per i thread POSIX, e definire una struttura di dati
 * che contenga le informazioni necessarie a ciascun thread:*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define num_threads 8

struct thread_data {
    pthread_t tid;
    FILE *inf;
    pthread_mutex_t *pmtx;
};

/* Nella struttura sono memorizzati l'identificatore del thread, il puntatore alla struttura FILE
 * per l'accesso al file di ingresso, e un puntatore al mutex necessario per la sincronizzare gli
 * accessi di ingresso e uscita dei thread.
 *
 * Nella funzione main() vengono allocati il vettore di struttura thread_data ed il mutex:*/

int main(int argc, char *argv[])
{
    FILE *inf;
    struct thread_data td[num_threads];
    pthread_mutex_t mtx;
    int i;


/* Dopo l'apertura del file di ingresso, realizzata dalla stessa funzione open_input_file()
 * dell'esercizio precedente, si inizializza il mutex:*/

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <inputfile>\n", argv[0]);
        return EXIT_FAILURE;
    }
    inf = open_input_file(argv[1]);
    if (pthread_mutex_init(&mtx, NULL) != 0) {
        fprintf(stderr, "Error in pthead_mutex_init()\n");
        return EXIT_FAILURE;
    }

/* Poi vengono inizializzati gli elementi del vettore di strtture thread_data:*/

    for (i=0; i<num_threads; ++i) {
        td[i].inf = inf;
        td[i].pmtx = &mtx;
    }

/* Si creano 7 thread POSIX (l'ottvo e costituito dal processo padre*/

    td[0].tid = pthread_self();
    create_threads(td+1, num_threads-1);

/* Subito dopo la loro creazione tutti i thread iniziano ad eseguire la funzione
 * process_values_in_file(). Termina la creazione dei thread anche il processo principale
 * passa ad eseguire funzione:*/

    process_values_in_file(td+0);

/* Quando il thread master ritorna dalla funzione process_values_in_file(), aspetta
 * la terminazione di tutti gli altri thread, poi conclude l'esecuzione del programma*/

    join_all_threads(td+1, num_threads-1);
    if (fclose(inf) != 0)
        perror(argv[1]);
    return EXIT_SUCCESS;
}

/* La creazione dei thread è affidata alla funzione create_threads();*/

void create_threads(struct thread_data *td, int num)
{
    int i;

    for(i=0; i<num; ++i)
        if (pthread_create(&td[i].tid, NULL, process_values_in_file, td+1) != 0) {
            fprintf(stderr, "Error returned by pthread_create()\n");
            exit(EXIT_FAILURE);
        }
}

/* La funzione process_values_in_file() deve essere modificata rispetto all'esercizio
 * precedente: devericevere come argomento l'indirizzo della struttura thread_data
 * contenente le informazioni necessarie al thread; inoltre deve proteggere le funzioni
 * di ingresso e uscita per mezzo del mutex.*/

void *process_values_in_file(void *arg)
{
    int rc;
    unsigned int v;
    struct thread_data *td = (struct thread_data *) arg;

    do {
        get_mutex(td->pmtx);
        rc = fscanf(td->inf, "%u", &v);
        put_mutex(td->pmtx);
        if (rc == 1) {
            if (analyze_value(v) == 1) {
                get_mutex(td->pmtx);
                printf("%u\n", v);
                put_mutex(td->pmtx);
            } else if (!feof(td->inf))
                fprintf(stderr, "Error reading from input file\n");
        }
        while (!feof(td->inf));
        return NULL;
    }
}


/*La funzione analyze value( ) è la stessa dell’esercizio precedente.
Per acquisire e rilasciare il mutex sono utilizzate due semplici funzioni:*/

void get_mutex ( pthread_mutex_t * pmtx )
{
    if (pthread_mutex_lock(pmtx) != 0) {
        fprintf ( stderr , " Error in pthread_mutex_ lok()\n" ) ;
        exit ( EXIT_FAILURE ) ;
    }
}

void put_mutex ( pthread_mutex_t * pmtx )
{
    if (pthread_mutex_unlock(pmtx) != 0) {
        fprintf ( stderr , " Error in pthread_mutex_lock()\n" ) ;
        exit ( EXIT_FAILURE ) ;
    }

}

/*Infine l’attesa per la terminazione dei thread è affidata alla funzione join all threads( ):*/

void join_all_threads ( struct thread_data * td , int num )
{
    int i;
    for ( i =0; i < num ; ++ i )
        if ( pthread_join ( td [ i ]. tid , NULL ) != 0) {
            fprintf ( stderr ,
                      "Error returned by pthread_join()\n" ) ;
            exit ( EXIT_FAILURE ) ;
        }
}