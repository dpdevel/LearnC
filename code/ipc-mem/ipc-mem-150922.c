/*Scrivere una applicazione C/POSIX che legge da una regione di memoria condivisa IPC di
dimensione pari a 4 pagine e la cui chiave è ricavabile utilizzando il percorso "." ed il codice
’1’.

 La regione di memoria contiene un insieme di dati costituito da un numero variabile di
strutture del seguente tipo:

 struct record {
    unsigned int key;
    int length;
    char name[];
 };

Il formato dei dati è nativo per l’architettura del calcolatore; in altre parole, la regione
di memoria contiene solo la sequenza di byte corrispondente alla rappresentazione in RAM
delle varie strutture record, una dopo l’altra. La lunghezza del vettore di caratteri name è
memorizzata nel corrispondente campo length. I record non hanno un ordine particolare
nella regione di memoria. Il valore di key è sempre un numero maggiore di zero, tranne per
il fatto che l’ultimo record significativo nella regione è seguito da un record in cui key ha il
valore speciale 0.

L’applicazione deve inserire i record letti dalla regione in una lista dinamica ordinata in base
al valore del campo key. Diversi record nella regione di memoria possono avere la stessa
chiave key; tali record corrispondenti allo stesso valore di key debbono essere rappresentati
da un unico elemento nella lista; il campo name associato a tale elemento nella lista deve
essere la concatenazione delle stringhe nei campi name nei record originali.
Inoltre, prima di terminare l’esecuzione, l’applicazione deve stampare in standard output i
record ordinati nella lista (una riga di testo per ciascun elemento della lista).*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

struct record {
    unsigned int key;
    int lenght;
    char name[];
};


struct listel {
    struct listel *next;
    unsigned int key;
    int lenght;
    char *name;
};

void error_exit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

void print_list(struct listel *p)
{
    while (p != NULL) {
        printf("%d : %s\n", p->key, p->name);
        p = p->next;
    }
}

void insert_after_node(struct listel *new, struct listel **pnext)
{
    new->next = *pnext;
    *pnext = new;
}

void insert_sorted_list(struct listel *new, struct listel **pnext)
{
    struct listel *p;
    char *q;
    for (p=*pnext; p!=NULL; pnext=&p->next, p=p->next) {
        if (p->key == new->key) {                     // se stessa chiave concateno le stringhe
            q = malloc(p->lenght + new->lenght - 1);
            if (q == NULL)
                error_exit("malloc");
            strncpy(q, p->name, p->lenght);
            strncat(q, new->name, new->lenght+1);
            free(p->name);
            p->name = q;
            p->lenght = p->lenght + new->lenght - 1;
            free(new->name);
            free(new);
            return;
        }
        if (p->key > new->key) {            // se chiave diversa inserisco l'elemento
            insert_after_node(new, pnext);
            return;
        }
    }
    insert_after_node(new, pnext);
}

struct listel *alloc_node(size_t strl)  //strl lunghezza del vettore name
{
    struct listel *p;
    p = malloc(sizeof(struct listel));
    if (p == NULL)
        error_exit("malloc");
    p->name = malloc(strl+1);   //+1 for '\0'
    if (p->name == NULL)
        error_exit("malloc");
    p->lenght=strl+1;
    return p;
}


struct listel *get_record(struct record **preg)   //preg puntatore alla regione
{
    struct record *r = *preg;
    struct listel *p;
    if (r->key == 0)                // se la chiave è 0 return NULL
        return NULL;
    p = alloc_node(r->lenght);      // alloca nodo con lunghezza di char = r
    p->next = NULL;
    p->key = r->key;
    p->lenght = r->lenght;
    p->name = strndup(r->name, r->lenght);   // copia il vettore
    *(char *) preg += sizeof(*r) + r->lenght;  // aggiorna posizione del vettore nella regione
    return p;                                  // somma record + lunghezza del vettore
}


void build_list(struct record *reg, struct listel **plh)   // plh indirizzo testa lista
{                                                          // reg regione mem
    struct listel *lep;
    for (;;) {
        lep = get_record(&reg);             // prelevo della mem un elemento
        if (lep == NULL)                    // se NULL salto
            break;
        insert_sorted_list(lep, plh);       // inserisco il record nella lista
    }
}

struct record * open_shmem(void)
{
    key_t key;
    int shmid;
    struct record *reg;
    int page_size;
    page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1)
        error_exit("sysconf");
    key = ftok(".", '1');
    if (key == -1)
        error_exit("ftok");
    shmid = shmget(key, 4*page_size, 0);
    if (shmid == -1)
        error_exit("shmget");
    reg = shmat(shmid, NULL, 0);
    if (reg == -1)
        error_exit("shmat");
    return reg;
}

int main ()
{
    struct recond *shared_region;
    struct listel *list_head = NULL;
    shared_region = open_shmem();
    build_list(shared_region, &list_head);
    print_list(list_head);
    return EXIT_SUCCESS;
}



