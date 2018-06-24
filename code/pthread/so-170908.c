/*Scrivere una applicazione C/POSIX multithread che riceve sulla linea comando un numero
arbitrario n di stringhe s 1 , . . . , s n .

 L’applicazione è costituita da n + 1 thread concorrenti.
Il primo thread T 0 dell’applicazione legge dallo standard input righe di testo (terminate dal
carattere “\n”) ed inserisce i puntatori a tali righe di testo in un buffer circolare avente come
capacità massima 100 puntatori.

 Ciascuno degli altri thread T i (1 ≤ i ≤ n) esamina tutte le righe di testo inserite nel buffer
circolare per ricercare la stringa s i . Se la stringa s i è presente nella riga, il thread T i stampa
l’intera riga sullo standard output.

Le righe di testo devono essere memorizzate fino al momento in cui sono state esaminate
da tutti i thread T 1 , . . . T n ; successivamente devono essere eliminate, in modo da liberare le
posizioni nel buffer circolare e recuperare la memoria occupata.

 Tutti i thread devono lavorare in modo concorrente, ma si devono curare gli aspetti di sincronizzazione. Ad esempio, si deve
gestire la situazione in cui il buffer circolare è pieno (thread T 0 bloccato), oppure vuoto (thread
T 1 , . . . , T n bloccati), ed evitare le race condition sulle strutture di dati condivise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define CIRC_BUF_SIZE (100+1)

struct {
    char *lines[CIRC_BUF_SIZE];
    unsigned int counts[CIRC_BUF_SIZE]; //contatore analisi threads
    int write_idx;
    pthread_mutex_t mtx;
    pthread_cond_t newentry;
    pthread_cond_t getfree;
} cb ;

#define abort_on_error(cond, msg) do { \
    if (cond) { \
        fprintf(stderr, "%s (errno=%d [%m])\n", msg, errno);  \
        exit(EXIT_FAILURE); \
    } \
} while (0)

#define abort_on_pthread_error(cond, msg) \
    abort_on_error((errno=cond), msg)


void insert_line_in_cb(char *line, int n)
{
    int rc, widx;
    rc = pthread_mutex_lock(&cb.mtx);
    abort_on_pthread_error(rc, "Cannot lock the mutex");
    widx = cb.write_idx;
    while (cb.counts[widx] > 0) {       // legge la successiva posizione del vettore, se il velore è positivo, non è libera.
        rc = pthread_cond_wait(&cb.getfree, &cb.mtx); // resta in attesa che la condizione cb.getfree segnali la liberazione
        abort_on_pthread_error(rc, "Cannot wait for condition");
    }

    cb.lines[widx] = line;
    cb.counts[widx] = n;
    rc = pthread_cond_broadcast(&cb.newentry); // segnala a tutti gli altri thread che una nuova posizione è libera.
    abort_on_pthread_error(rc, "Cannot broadcast the condition");

    ++widx;
    if (widx == CIRC_BUF_SIZE)
        widx = 0;
    while (cb.counts[widx] > 0) {
        rc = pthread_cond_wait(&cb.getfree, &cb.mtx);
        abort_on_pthread_error(rc, "Cannot wait for condition");
    }

    cb.write_idx = widx;
    rc = pthread_mutex_unlock(&cb.mtx);
    abort_on_pthread_error(rc, "Cannot unlock the mutex");
}


int master_thread_finished = 0;

void *thread_search(void *arg)
{
    char *r, *line, *word = (char * ) arg;
    int rc, read_idx = 0;
    rc = pthread_mutex_lock(&cb.mtx);
    abort_on_pthread_error(rc, "Cannot lock the mutex");
    for (;;) {                              // per verificare se ci sta qualcosa da leggere
        while (cb.write_idx == read_idx) { //confronta l'indice di scrittura globlale con quello di lettura locale
            if (master_thread_finished) {  // Controllare il valore della variabile prima di bloccare
                                         // permette di far terminare tutti i thread dell’applicazione in modo ordinato dopo
                                        // che ciascuno di essi ha finito di esaminare tutte le righe rimaste nel buffe circolare.

                rc = pthread_mutex_unlock(&cb.mtx);
                abort_on_pthread_error(rc, "Cannot unlock the mutex");
                return NULL;
            }
            rc = pthread_cond_wait(&cb.newentry, &cb.mtx);
            abort_on_pthread_error(rc , "Cannot wait for condition");
        }
        rc = pthread_mutex_unlock(&cb.mtx);
        abort_on_pthread_error(rc, "Cannot unlock the mutex");

        line = cb.lines[read_idx];
        r = strstr(line, word);   // controlla la presenza della parola in word
        if (r != NULL)         // restituisce l’indirizzo iniziale della sottostringa, se essa è presente, oppure il valore NUL
            printf("[%s]: %s", word, line);
        rc = pthread_mutex_lock(&cb.mtx);
        abort_on_pthread_error(rc, "Cannot lock the mutex");
        --cb.counts[read_idx]; // Se il contatore si azzera, la riga è stata esaminata da tutti i thread
        if (cb.counts[read_idx] == 0) {
            free(line);
            cb.lines[read_idx] = NULL;
            rc = pthread_cond_signal(&cb.getfree);
            abort_on_pthread_error(rc, "Cannot signal the condition");
        }
        read_idx++;
        if (read_idx == CIRC_BUF_SIZE)
            read_idx = 0;
    }
}

 /*La funzione imposta ad uno master_thread_finished e sblocca ogni thread
eventualmente in attesa sulla variabile condizione cb.newentry. Infine il thread
T 0 termina.*/

void finish(void)
{
    master_thread_finished = 1; // impostato a 1 dal thread T0 subito prima che esso termini
    abort_on_pthread_error(pthread_cond_broadcast(&cb.newentry), "Cannot broadcast the condition");
    pthread_exit(NULL);
}


void read_lines_from_stdin(int n)
{
    char *p, locbuf[1024];
    for (;;) {
        p = fgets(locbuf, 1024, stdin);
        if (p == NULL) {
            abort_on_error(!feof(stdin), "Error reading from stdin");
            return;
        }
        p = strdup(locbuf);
        insert_line_in_cb(p, n);
    }
}

void create_threads(int n, char *argv[])
{
    int i, rc;
    for (i=0; i<n; ++i) {
        pthread_t tid;
        char *p = strdup(argv[i]);
        abort_on_error(!p, "String duplication failure");
        rc = pthread_create(&tid, NULL, thread_search, p);
        abort_on_pthread_error(rc, "Thread creation failure");
    }
}

void initialize_circ_buf(void)
{
    memset(&cb, 0, sizeof(cb));
    abort_on_pthread_error(pthread_mutex_init(&cb.mtx, NULL), "Cannot initialize the mutex");
    abort_on_pthread_error(pthread_cond_init(&cb.newentry, NULL), "Cannot initialize the condition");
    abort_on_pthread_error(pthread_cond_init(&cb.getfree, NULL), "Cannot initialize the condition");
}

int main(int argc, char *argv[])
{
    abort_on_error(argc < 2, "Wrong number of command line arguments");
    initialize_circ_buf();
    create_threads(argc-1, argv+1);
    read_lines_from_stdin(argc-1);
    finish();
}
