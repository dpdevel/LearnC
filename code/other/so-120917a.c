/*Esercizio 1. In un file di testo è memorizzata una sequenza di lunghezza indefinita di numeri
interi di tipo int separati tra loro da spazi bianchi e/o caratteri di fine riga (\n). Scrivere un
programma C/POSIX che riceve il nome del file di testo da linea comando; esso deve leggere
i numeri dal file e memorizzarli in una lista, inserendo volta per volta l’elemento letto per
ultimo in cima alla lista. La lista deve essere realizzata come una struttura di dati dinamica
utilizzando i puntatori.
Dopo aver costruito la lista il programma deve effettuarne una scansione stampando i suoi
elementi in standard output. In pratica quindi il programma deve stampare i numeri in ordine
inverso rispetto al file.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    FILE *inf;
    struct  lista_t *head = NULL;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <nome file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    inf = open_file(argv[1]);
    fill_list(inf, &head);
    print_list(head);
    close_file(inf);
    /* dynamic memory is freed b OS on process exit */
    return EXIT_SUCCESS;
}


//Ciascun elemento della lista è costituito da una struttura di dati di tipo struct list_t:

struct  list_t {
    struct lista_t *next;
    int val;
};

//L'apertura e la chiusura del file di ingresso sono effettuate dalle funzioni
// open_file() e close_file:

FILE * open_file(const char *name)
{
    FILE *f;

    f = fopen(name, "r");
    if (f == NULL) {
        perror(name);
        exit(EXIT_FAILURE);
    }
}

/* La funaione fill_list() legge i numeri interni dal file di ingresso e li passa alla
 * funzione insert_h() perchè siano inseriti in cima alla lista. Si noti che la funzione
 * fscanf() assume che i numeri in ingresso siano separati da una sequenza di spazi, tab
 * e/o caratteri "\n": */

void fill_list(FILE *f, struct lista_t **phead)
{
    int v;

    for (;;) {
        if (fscanf(f, "%d", &v) == 1)
            insert_h(phead, v);
        else
            break;
    }
    if (!feof(f)) {
        fprintf(stderr, "Invalid number in input file\n");
        exit(EXIT_FAILURE);
    }
}

/* La funzione insert_h() inserisce un elemento v in cima alla lista il cui elemento
 * di testa è puntanto dalla variabile il cui indirizzo è in ph: */

void insert_h(struct lista_t **ph, int v)
{
    struct lista_t *p;
    p = malloc(sizeof(struct lista_t));
    if (p == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    p->val = v;
    p->next = *ph;
    *ph = p;
}

/*Infine la funzione print_list() stampa in ordine tutti gli elementi della lista il
 * cui primo elemento è passato in head */

void print_list(struct lista_t *head)
{
    while (head != NULL) {
        printf("%d ", head->vail);
        head = head->next;
    }
    putchar('\n');
}

/*Si noti che l’argomento head è assimilabile ad una variabile locale il cui con-
tenuto iniziale è impostato dalla funzione chiamante; perciò è possibile modifi-
carne il valore all’interno di print list( ) senza conseguenze per la funzione
chiamante.*/