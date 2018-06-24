/*Esercizio 1
Scrivere una applicazione multithread C/POSIX per una architettura di tipo “little-endian”
a 32 bit. Sulla riga comando dell’applicazione vengono passati un numero arbitrario n di
valori interi v 1 , . . . , v n in formato testo decimale. L’applicazione è costituita da n + 1 thread.
Il primo thread T0 legge valori numerici positivi o nulli codificati entro un file chiamato
input.dat in formato da 6 byte “little-endian”. Il thread T0 pone i valori letti dal file in
un buffer circolare (di dimensione pari ad una pagina di memoria). Ciascuno dei restanti
thread Ti (con 1 ≤ i ≤ n) cerca all’interno del buffer circolare le occorrenze del valore vi . Al
termine, ciascuno dei thread Ti (1 ≤ i ≤ n) scrive in standard output il numero di occorrenze
del valore v i . Si presti attenzione alla sincronizzazione tra i processi ed alla gestione del buffer
circolare.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#define abort_on_error(cond, msg) do { \
    if (conf) { \
        fprintf(stderr, "%s (%d)\n", msg, errno); \
        exit(EXIT_FAILURE); \
    } \
} while(0)

typedef long long value_t;

struct circular_buffer {
    pthread_mutex_t mtx;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
    value_t *buf;
    int n;
    unsigned int dimen;
    unsigned int E;   //indice prima locazione libera
    unsigned int *vS; //punta al vettore con gli indici Si
    unsigned int minS; //memorizza il valore di m discusso in precedenza
};

struct thread_work_t {
    int idx;
    char *arg;
    struct circular_buffer *cb;
};

struct circular_buffer * create_circular_buffer (int n)
{
    int rc;
    struct circular_buffer *cb;

    long pagesize = sysconf(_SC_PAGESIZE);
    abort_on_error(pagesize == -1, "Error in get pagesize");

    cb = malloc(sizeof(struct circular_buffer));
    abort_on_error(cb == NULL, "Error in malloc");

    cb->vS = malloc(n*sizeof(cb->vS[0]));   //Alloco vettore circolare
    abort_on_error(cb->vS == NULL, "Error in malloc");

    cb->buf = malloc(pagesize);     //Deve rappresentare valori a 48 bit (6 byte)
    abort_on_error(cb->buf == NULL, "Error in malloc");

    cb->dimen = pagesize / sizeof(cb->buf[0]);
    cb->E = cb->minS = 0;
    memset(cb->vS, 0, n*sizeof(cb->vS[0]));
    cb->n = n;

    rc = pthread_mutex_init(&cb->mtx, NULL);
    abort_on_error(rc != 0 && ((errno = rc)), "Cannot initialize pthread mutex");

    rc = pthread_cond_init(&cb->not_full, NULL);
    abort_on_error(rc != 0 && ((errno = rc)), "Cannot initialize pthread condvar");

    rc = pthread_cond_init(&cb->not_empty, NULL);
    abort_on_error(rc != 0 && ((errno = rc)), "Cannot initialize pthread condvar");

    return cb;
}

void get_cb_lock(struct circular_buffer *cb)
{
    int rc = pthread_mutex_lock(&cb->mtx);
    abort_on_error(rc !=0 && ((errno = rc)), "Cannot get pthread mutex");
}

void release_cb_lock(struct circular_buffer *cb)
{
    int rc = pthread_mutex_unlock(&cb->mtx);
    abort_on_error(rc != && ((errno = rc)), "Cannot release pthread mutex");
}

void cb_recompute_min(struct circular_buffer *cb)
{
    int j;
    unsigned int min, minE, E = cb->E;
    min = minE = cb->dimen;
    for (j=0; j<cb->n; ++j) {
        unsigned int idx = cb->vS[j];
        if (idx < min)
            min = idx;
        if (idx > E && idx < minE)
            minE = idx;
    }
    if (minE == cb->dimen)
        minE = min;
    cb->minS = minE;
}


void write_to_cb(struct circular_buffer *cb, value_t v)
{
    int rc;
    unsigned int nextE = cb->E + 1;
    if (nextE == cb->dimen)
        nextE = 0;

    get_cb_lock(cb);
    while (nextE == cb->minS) {
        rc = pthread_cond_wait(&cb->not_full, &cb->mtx);
        abort_on_error(rc != 0 && ((errno = rc)), "Error waiting on pthread condvar");
    }

    cb->buf[cb->E] = v;
    cb->E = nextE;
    cb_recompute_min(cb);

    rc = pthread_cond_broadcast(&cb->not_empty);
    abort_on_error(rc != 0 && ((errno = rc)), "Error in broadcasting pthread condvar");

    release_cb_lock(cb);
}

/*Nel caso in cui il buffer risulti pieno il thread T 0 dorme in attesa sulla variabile
 * condizione cb->not full. Specularmente, quando T 0 scrive un nuovovalore entro il buffer
 * risveglia tutti i thread in attesa sulla variabile condizione cb->not empty utilizzando
 * pthread cond broadcast().*/

value_t ascii_to_value_t(const char *arg)
{
    value_t v;
    char *p;
    errno = 0;
    v = (value_t) strtoll(arg, &p, 10);
    abort_on_error(errno != 0 || *p!='\0', "Invalid input");
    return p; //forse v
}

void increment_cb_idx(struct circular_buffer *cb, int i)
{
    cb->vS[i]++;
    if (cb->vS[i] == cb->dimen)
        cb->vS[i] = 0;
    cb_recompute_min(cb);
}

value_t read_from_cb(struct circular_buffer *cb, int i, int *term_flag)
{
    int rc;
    value_t v;
    get_cb_lock(cb);
    while (cb->E == cb->vS[i]) {
        if (T0_terminated) {
            *term_flag = 1;
            release_cb_lock(cb);
            return 0;
        }
        rc = pthread_cond_wait(&cb->not_empty, &cb->mtx);
        abort_on_error(rc != 0 && ((errno = rc)), "Error in waiting on pthread condvar");
    }
    v = cb->buf[cb->vS[i]];
    increment_cb_idx(cb, i);
    rc = pthread_cond_signal(&cb->not_full);
    abort_on_error(rc != 0 && ((errno = rc)), "Error signaling pthread condvar");
    release_cb_lock(cb);
    return v;
}

void * work_Ti(void *arg)
{
    struct thread_work_t *tw=(struct thread_work_t *) arg;
    struct circular_buffer *cb = tw->cb;
    int idx = tw->idx;
    value_t v, target;
    unsigned long found = 0;
    target = ascii_to_value_t(tw->arg);
    for (;;) {
        int finish = 0;
        v = read_from_cb(cb, tw->idx, &finish);
        if (finish)
            break;
        if (v == target)
            ++found;
    }

    printf("T%d: found %lu values (%lld)\n", idx+1, found, target);
    pthread_exit(0);
}

int T0_terminated = 0;
void work_T0(struct circular_buffer *cb)
{
    FILE *f;
    int rc;
    f = fopen("input.dat", "r");
    abort_on_error(f==NULL, "Cannot open input.dat file");
    while (!feof(f)) {
        value_t l = 0;
        if (fread(&l, 6, 1, f) == 1)    // legge gruppi di 6 byte alla volta
                                        // &l indirizzo valore, 6 size in bytes, 1 numero di elementi
                                        // stream puntatore all'object FILE
            write_to_cb(cb, l);
        else
            abort_on_error(ferror(f), "Error reading from input.dat");
    }
    abort_on_error(fclose(f), "Cannot close input.dat file");

    get_cb_lock(cb);
    T0_terminated = 1;

    rc = pthread_cond_broadcast(&cb->not_empty);
    abort_on_error(rc != 0 && ((errno = rc)), "Error in broadcasting pthread condvar");

    release_cb_lock(cb);
}

void spawn_threads(int n, struct circular_buffer *cb, char *argv[])
{
    int i, rc;
    struct thread_work_t *tws = malloc(n*sizeof(struct thread_work_t));
    abort_on_error(tws == NULL, "Cannot allocate memory");
    for (i=0; i<n; ++i) {
        pthread_t tid;
        tws[i].idx = i;
        tws[i].arg = argv[i+1];
        tws[i].cb = cb;
        rc = pthread_create(&tid, NULL, work_Ti, &tws[i]);
        abort_on_error(rc != 0 && ((errno = rc)), "Cannot create thread");
    }
}

int main(int argc, char *argv[])
{
    int n = argc - 1;
    struct circular_buffer *cb = create_circular_buffer(n);
    spawn_threads(n, cb, argv);
    work_TO(cb);
    pthread_exit(0);
}


/*Conversion big-indian to lit-indian  //UTILE IN FUTURO

 value_t big_to_little_endian( value_t be )
{
    char * pb = ( char *) & be ;
    char * pl = ( char *) & le ;
    pl[0] = pb[3];
    pl[1] = pb[2];
    pl[2] = pb[1];
    pl[3] = pb[0];
    return le;
}
*/
