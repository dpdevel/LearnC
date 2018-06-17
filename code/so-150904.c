/*Scrivere una applicazione C/POSIX multithread costituita da tre thread che implementano
le seguenti procedure:

 T1) Il primo thread inserisce in una lista dinamica ordinata per valori crescenti tutti i valori
interi x maggiori di 1, ossia 2, 3, 4, 5, ecc.

 T2) Contemporaneamente, il secondo thread rimuove dalla stessa lista dinamica i valori
interi che possono essere divisi esattamente da uno dei valori precedenti nella lista. Ad
esempio, il thread lascerà in lista i valori 2 e 3, ma rimuoverà il valore 4, perché è
divisibile per 2.

 T3) Contemporaneamente, il terzo thread accede alla stessa lista dinamica e scrive in stan-
dard output i valori che non sono stati (e non dovranno essere) cancellati dal secondo
thread.

 I thread debbono eseguire il più possibile in parallelo tra loro, ma si deve evitare ogni “race
condition” dovuta agli accessi alla lista dinamica condivisa. Si ponga attenzione alla sincro-
nizzazione tra i thread. Ad esempio, T3 non deve stampare valori in lista che dovrebbero
essere rimossi da T2.
 */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#define abort_on_error(cond, msg) do { \
    if (cond) { \
        fprintf(stderr, "%s (%d)\n", msg, errno); \
    } \
} while (0)

struct node_t {
    unsigned long long value;
    struct node_t *next;
};

struct node_t *head;
struct node_t *ptr_last;
struct node_t *ptr_safe;

pthread_mutex_t mtx_last;
pthread_cond_t  cond_last;
pthread_mutex_t mtx_safe;
pthread_cond_t  cond_safe;


void lock(pthread_mutex_t *mtx)
{
    if (pthread_mutex_lock(mtx))
        perror("error in pthread_mutex_lock()\n");
}

void unlock(pthread_mutex_t *mtx)
{
    if (pthread_mutex_unlock(mtx))
        perror("error in pthread_mutex_unlock()\n");
}

void awake(pthread_cond_t *cnd)
{
    if (pthread_cond_signal(cnd))
        perror("error in pthread_cond_signal()\n");
}

void wait(pthread_cond_t *cnd, pthread_mutex_t *mtx)
{
    if (pthread_cond_wait(cnd, mtx))
        perror("error in pthrad_cond_wait()\n");
}


void insert_value(unsigned long long value, struct node_t **pnode)
{
    struct node_t *new = malloc(sizeof(struct node_t));
    abort_on_error(new == NULL, "Memory allocation error\n");
    new->value = value;
    new->next = *pnode;
    *pnode = new;
}


int is_divisible(unsigned long long value, struct node_t * limit)
{
    struct node_t *p = head;
    if (limit == NULL)
        return 0;
    while (p != limit) {
        if (value % p->value == 0)
            return 1;
        p = p->next;
    }
    return 0;
}

void remove_next_node(struct node_t *prev)
{
    struct node_t *toberemoved = prev->next;
    prev->next = toberemoved->next;
    free(toberemoved);
}


void *T3_job(void *v)
{
    struct node_t *p = head;
    v = v; // v is unused
    for (;;) {
        lock(&mtx_safe);
        if (p == ptr_safe)
            wait(&cond_safe, &mtx_safe);
        unlock(&mtx_safe);
        printf("%llu\n", p->value);
        p = p->next;
    }
}

void *T2_job(void *v)
{
    struct node_t *p = head, *prev = NULL;
    v = v; //v is unused
    for (;;) {
        lock(&mtx_last);
        if ( p == ptr_last)
            wait(&cond_last, &mtx_last);
        unlock(&mtx_last);
        if (is_divisible(p->value, prev)) {
            p = p->next;
            remove_next_node(prev);
        } else {
            if (ptr_safe != p) {
                lock(&mtx_safe);
                ptr_safe = p;
                awake(&cond_safe);
                unlock(&mtx_safe);
            }
            prev = p;
            p = p->next;
        }
    }
}

void T1_job(void)
{
    unsigned long long value;
    struct node_t *p = head;
    for (value=3; value > 2; ++value) {
        insert_value(value, &p->next);
        lock(&mtx_last);
        ptr_last = p->next;
        awake(&cond_last);
        unlock(&mtx_last);
        p = p->next;
    }
}



void initialize(void)
{
    pthread_t tid1, tid2;
    if (pthread_mutex_init(&mtx_last, NULL) || pthread_mutex_init(&mtx_safe, NULL))
        perror("Error in pthread_mutex_init()\n");
    if (pthread_cond_init(&cond_last, NULL) || pthread_cond_init(&cond_safe, NULL))
        perror("Error in pthread_cond_init()\n");

    head = NULL;
    insert_value(2, &head);
    ptr_last = ptr_safe = head;
    if (pthread_create(&tid1, NULL, T2_job, NULL))
        perror("Error in creation second thread");
    if (pthread_create(&tid2, NULL, T3_job, NULL))
        perror("Error in creation third thread");
}

int main()
{
    initialize();
    T1_job();
    exit(EXIT_SUCCESS);
}