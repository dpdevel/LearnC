/*Esercizio 1. Scrivere una applicazione C/POSIX che avvia l’esecuzione di un program-
ma esterno con percorso “/tmp/producer” e legge i dati forniti in standard output da tale
programma.

I dati prodotti dal programma esterno sono costituiti da un numero finito e variabile di
strutture del seguente tipo:

struct record {
    int key;
    char name[32];
};

Il formato dei dati è nativo per l’architettura del calcolatore; in altre parole, la sequenza di
byte prodotta dal programma esterno corrisponde alla rappresentazione in RAM delle varie
strutture record, una dopo l’altra. I record forniti dal programma esterno non hanno un
ordine particolare.

L’applicazione deve inserire i record forniti dal programma esterno in una lista dinamica ordi-
nata in base al valore del campo key. Inoltre, prima di terminare l’esecuzione, l’applicazione
deve stampare in standard output i record ordinati nella lista con il seguente formato: una
riga di testo per ciascun differente valore di key contenente il valore di key e tutte le stringhe
name associate a quel valore.

Si assuma che il campo name di ogni record contenga sempre una stringa di lunghezza variabile
terminata da ’\0’.*/

#include <stdio.h>
#include <stdlib.h>

struct record {
    int key;
    char name[32];
};

struct listel {
    struct record rec;
    struct listel *next;
};

/*La struttura listel è costituita da una sotto-struttura di tipo record e da un puntatore
 * alla struttura listel successiva nella lista.*/


/* La variabile list_head memorizza il puntatore al primo elemento della lista dinamica
 * La funzione run_extern_program() invoca il programma esterno collagandolo tramite
 * una pipe: */

FILE *run_extern_program(void)
{
    const char *path = "/tmp/producer";
    FILE *f = popen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "Error executing popen(\"%s\")\n", path);
        exit(EXIT_FAILURE);
    }
    return f;
}

/*Nel momento in cui la posizione corratta, dipendente dal valore della chiave, è
 * stata determinata, l'elemento è inserito nella lista trmite la funzione
 * insert_after_node()*/

void insert_after_node(struct listel *new, struct listel **pnext)
{
    new->next = *pnext;
    *pnext = new;
}

/*Per cercare la posizione corretta di un nuovo elemento da inserire in lista, si
utilizza la funzione insert sorted list( ):*/

void insert_sorted_list(struct listel *new, struct listel **pnext)
{
    struct listel *p;
    for (p=*pnext; p!=NULL; pnext=&p->next, p=p->next)
        if (p->rec.key > new->rec.key) {
            //Inserisci new dopo pnext, prima di p
            insert_after_node(new, pnext);
            return;
        }
    //l'elemento va inserito in fondo alla lista
    insert_after_node(new, pnext);
}


/*Per allocare un nuovo elemento della lista viene utilizzata la funzione alloc_node()*/

struct listel *alloc_node(void)
{
    struct listel *p;
    p = malloc(sizeof(struct listel));
    if (p == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

/*La funzione get_record() legge un nuovo record dalla pipe. In caso di errore termina
 * l'applicazione, altrimenti restituisce l'indirizzo di un nuovo elemento della lista
 * (ancora da inserire)m ovvero NULL in cado di EOF sulla pipe:*/

struct listel *get_record(FILE *f)
{
    struct listel *p = alloc_node();
    p->next = NULL;
    if (fread(&p->rec, sizeof(struct record), 1, f) == 1)
        return p;
    if (feof(f))
        return NULL;
    fprintf(stderr, "Error reading from the pipe\n");
    exit(EXIT_FAILURE);
}


/*La funzione build_list() legge dalla pipe ed inserisce mana mano i record nella lista dinamica: */

void build_list(FILE *f, struct listel **plh)
{
    struct listel *lep;
    for (;;) {
        lep = get_record(f);
        if (lep == NULL)
            break;
        insert_sorted_list(lep, plh);
    }
}


/*Infine, per stampare prima del termine dell'esecuzione il contenuto della lista
 * dinamica, la funzione main() invoca print_list():*/

void print_list(struct listel *p)
{
    while (p != NULL) {
        int key = p->rec.key;
        printf("%d :", key);
        while (p != NULL && p->rec.key == key) {
            printf(" %s", p->rec.name);
            p = p->next;
        }
        putchar('\n');
    }
}

/*La funzione main() deve invocare il programma esterno facendo in modo di leggere il suo
 * standard output, costruire la lista dinamica, ed infine stampare in standard output il
 * contenuto della lista:*/

int main()
{
    FILE *fpipe;
    struct  listel *list_head = NULL;

    fpipe = run_extern_program();
    build_list(fpipe, &list_head);
    print_list(list_head);

    return EXIT_SUCCESS;
}