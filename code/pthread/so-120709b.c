/*
Una matrice quadrata di numeri di tipo float è memorizzata in un file con il seguente formato:

• All’inizio del file è memorizzata la dimensione N (numero di righe e numero di colonne) della matrice come
 sequenza di cifre ASCII decimali
• Di seguito, separati da caratteri “spazio”, sono memorizzati gli N^2 elementi della matrice riga per riga
 come sequenza di cifre ASCII decimali e punti decimali.

ad esempio, la matrice
|1.0 2.0|
|3.0 4.0|

è memorizzata con la sequenza di caratteri nel file: 2 1.0 2.0 3.0 4.0

b) Modificare il programma precedente in modo che il calcolo delle somme degli elementi delle colonne sia
   effettuato in parallelo con thread POSIX. */

#include <stdio.h>
#include <stlib.h>
#include <unistd.h>
#include <pthread.h>

int main(int argc, char *argv[])
{
    FILE *f;
    float *M, val;
    int N;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    f = open_file(argv[1]);
    read_matrix(f, &M, &N);
    printf ("Result (sequent): %f\n", val);
    val = prodsumcol(M, N);
    printf ("Result (pthread): %f\n", val);
}

//Per tenera traccia del lavoro svolto da ciascun thread definiamo un struttura di dati:
struct thread_job {
    pthread_t tid;      //  Thread ID
    float *col;         //  Primo elemento della colonna
    int N;              //  Dimensione della colonna
    float  sum;         //  Somma degli elementi della colonna
};

/* La procedura prodsumcol( ) crea N thread ed aspetta che ciascuno di essi completi il pro
prio lavoro. Il risultato calcolato da ciascun thread, registrato entro la struttura di dati
thread job, è utilizzato per calcolare il prodotto finale. */

float prodsumcol(float *m, int N)
{
    int c;
    float prod = 1.0;
    struct thread_job *tv;

    //Allocazione del vettore di thread_job
    tv = malloc(N* sizeof(struct thread_job));
    if (tv == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    //Creazione degli N thread
    for (c=0; c<N; ++c) {
        tv[c].col = m+c;
        tv[c].N = N;
        if (pthread_create(&tv[c].tid, NULL, sumcol, tv+c) != 0) {
            fprintf(stderr, "Error in pthread_join()\n");
        }
        prod *= tv[c].sum;
    }
    return prod;
}

/*Procedura sumcol() seguita da ciascun degli N thread per calcolare la somma di una colonna.
 * I parametri su cui operare vengono letti dal puntatore alla struttura di dati thread job
 * il cui valore è ricavato, con un opportuno cast, dall’argomento passato alla procedura.*/

void *sumcol(void *p)
{
    struct thread_job *j = (struct thread_job *) p;
    float *m = j->col;
    int r;

    j->sum = 0.0;
    for (r=0; r < j->N; ++r, m += j->N)
        j->sum += *m;
    pthread_exit(NULL);   //si può omettere
}