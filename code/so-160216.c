/*
 Scrivere una applicazione C/POSIX multithread costituita da due thread T 1 e T 2 . Il thread
T 1 legge il contenuto di un file il cui nome è passato sulla linea comando. Il file contiene un
numero finito e variabile di strutture del seguente tipo:

 struct record {
int key;
char name[32];
};

Il formato dei dati è nativo per l’architettura del calcolatore; in altre parole, la sequenza di
byte nel file corrisponde alla rappresentazione in RAM delle varie strutture record, una dopo
l’altra. I record entro il file non sono ordinati in alcun modo particolare.
Il thread T 1 legge dal file un record alla volta e lo inserisce in un buffer circolare. Il thread
T 2 legge i record dal buffer circolare e li inserisce in una lista ordinata in base al valore del
campo key.

 Al termine, il thread T 1 scrive in standard output i record ordinati nella lista con il seguente
formato: una riga di testo per ciascun differente valore di key contenente il valore di key e
tutte le stringhe name associate a quel valore.
Si assuma che il campo name di ogni record contenga sempre una stringa di lunghezza variabile
terminata da ’\0’.

 Si considerino gli eventuali problemi legati alle race condition ed alla sincronizzazione tra i
due thread.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

#define ringbuf_size 16

struct record {
    int key;
    char name[32];
};

struct ringbuf {
    int E, S;
    pthread_mutex_t mtx;
    pthread_cond_t no_full, no_empty;
    struct record rec[ringbuf_size];
};


struct list_t {
    struct record rec;
    struct list_t *next;
};

struct thread_data {
    struct ringbuf *r;
    struct list_t **lh;
};

void insert_after_node(struct list_t *new, struct list_t **pnext)
{
    new->next = *pnext;
    *pnext = new;
}

void insert_in_ordered_list(struct record *rec, struct list_t **phead)
{
    int key = rec->key;
    struct list_t *new = malloc(sizeof(struct list_t));
    if (new == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new->rec = *rec;
    new->next = NULL;
    while (*phead != NULL && key >= (*phead)->rec.key)
        phead = &(*phead)->next;
    insert_after_node(new, phead);
}


void wait_until_rb_not_empty(struct ringbug *rb)
{
    while (rb->E == rb->S)
        chk_pthread_err(pthread_cond_wait(&rb->no_empty, &rb->mtx));
}

void ringbuf_out(struct record *rec, struct ringbuf *rb)
{
    chk_pthread_err(pthread_mutex_lock(&rb->mtx));
    wait_until_rb_not_empty(rb);
    *rec = rb->rec[rb->S];
    rb->S = (rb->S + 1) % ringbuf_size;
    chk_pthread_err(pthread_mutex_unlock(&rb->mtx));
}

void * T2_work(void *arg)
{
    struct thread_data *td = (struct thread_data *) arg;
    struct record rec;
    for (;;) {
        ringbuf_out(&rec, td->r);
        insert_in_ordered_list(&rec, td->lh);
        chk_pthread_err(pthread_cond_signal(&rb->no_full));
    }
}

void print_sorted_list(struct list_t *h)
{
    while (h != NULL) {
        int key = h->rc.key;
        printf("%d: %s", key, h->rec.name);
        for (;;) {
            h = h->next;
            if (h == NULL || key != h->rec.key)
                break;
            printf(" %s", h->rec.name);
        }
        printf("\n");
    }
}


void wait_until_rb_empty(struct ringbuf *rb)
{
    chk_pthread_err(pthread_mutex_lock(&rb->mtx));
    while (rb->E != rb->S)
        chk_pthread_err(pthread_cond_wait(&rb->no_full, &rb->mtx));
    chk_pthread_err(pthread_mutex_unlock(&rb->mtx));
}

int wait_until_rb_not_full(struct ringbuf *rb)
{
    int nE = (rb->E + 1) % ringbuf_size;
    while (nE == rb->S)
        chk_pthread_err(pthread_cond_wait(&rb->no_full, &rb->mtx));
    return nE;
}

void ringbuf_in(struct record *rec, struct ringbuf *rb)
{
    int nE;
    chk_pthread_err(pthread_mutex_lock(&rb->mtx));
    ne = wait_until_rb_not_full(rb);
    rb->rec[rb->E] = *rec;
    rb->E = nE;
    chk_pthread_err(pthread_cond_signal(&rb->no_empty));
    chk_pthread_err(pthread_mutex_unlock(&rb->mtx));
}

int get_record(FILE *f, struct record *buf)
{
    size_t rc = fread(buf, sizeof(struct record), 1, f);
    return (rc == -1);
}

void read_file(const char *fname, struct ringbuf *rb)
{
    struct record rec;
    FILE *f = fopen(fname, "r");
    if (f == NULL) {
        perror(fname);
        exit(EXIT_FAILURE);
    }
    while (!feof(f)) {
        if (get_record(f, &rec))
            ringbuf_in(&rec, rb);
        if (ferror(f)) {
            perror(fname);
            exit(EXIT_FAILURE);
        }
    }
}

void spawn_thread(struct thread_data *ptd)
{
    pthread_t tid;
    chk_pthread_err(pthread_create(&tid, NULL, T2_work, ptd));
}

void chk_pthread_err(int rc)
{
    if (rc == 0)
        return;
    errno = rc;
    perror(NULL);
    exit(EXIT_FAILURE);
}

void initialize_ringbuf(struct ringbf *r)
{
    r->E = r->S = 0;
    chk_pthread_err(pthread_mutex_init(&r->mtx, NULL));
    chk_pthread_err(pthread_cond_init(&r->no_full, NULL));
    chk_pthread_err(pthread_cond_init(&r->no_empty, NULL));
}

int main(int argc, char *argv[])
{
    struct ringbuf rb;
    struct list_t *lh = NULL;
    struct thread_data td = { &rb, &lh };
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    initialize_ringbuf(&rb);
    spawn_thread(&td);
    read_file(argv[1], &rb);
    wait_until_rb_empty(&rb);
    print_sorted_list(lh);
    return EXIT_SUCCESS;
}
