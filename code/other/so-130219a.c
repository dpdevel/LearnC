/*Esercizio 1. In un file di testo è memorizzata una sequenza di lunghezza indefinita di numeri
interi di tipo unsigned int separati tra loro da spazi bianchi e/o caratteri di fine riga (\n).
Scrivere un programma C/POSIX che riceve il nome del file di testo da linea comando; esso
deve leggere i numeri dal file e riscrivere in standard output solo i numeri che sono divisibili
esattamente per
            2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47.

Ad esempio, se nel file di testo appare il numero 109134 esso dovrà essere scritto sullo standard
output (infatti 109134 = 2 · 3^3 · 43 · 47). Al contrario, se nel file di testo appare il numero
109130, esso non dovrà apparire in standard ouput (infatti 109130 = 2 · 5 · 7 · 1559).*/


#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    FILE *inf;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <inputfile>\n", argv[0]);
        return  EXIT_FAILURE;
    }
    inf = open_input_file(argv[1]);
    process_values_in_file(inf);
    if (fclose(inf) != 0)
        perror(argv[1]);
    return EXIT_SUCCESS;
}

//La funzione open_input_file() apre il file di ingresso:

FILE * open_input_file(const char *filename)
{
    FILE *f;
    f = fopen(filename, "r");
    if (f == NULL) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    return f;
}

//Il cuore del programma è realizzato dalla funzione process_values_in_file():

void process_values_in_file(FILE *inf)
{
    int rc;
    unsigned  int v;

    do {
        rc = fscanf(inf, "%u", &v);
        if (rc == 1) {
            if (analze_value(v))
                printf("%u\n", v);
        } else
            if (!feof(inf))
                fprintf(stderr, "Error reading from input file\n");
    } while (!feof(inf));
}

/*La funzione analyze_value() deve restituire 1 se e solo se il valore passato come argomento è
 esattamente divisibile per i divisori indicati nel testo. Un semplire algoritmo per realizzare
 tale funzione consiste nel dividere ripetutamente per i vari divisori, passando da un
 divisore al successivo quando si scopre che il resto della divisione è diverso da zero.
 Se il quoziente ad un certo punto si riduce ad 1, allora il valore iniziale era divisibile e
 la funzione restituisce 1. Al contrario, se i divisori finiscono ed il quoziente non è ridotto
 ad uno, allora il valore iniziale non era divisibile e la funzione restituisce 0*/

#define num_divisors 15
static const unsigned int divisors[num_divisors] = {2, 3, 5, 7, 11, 13, 17, 19, .....};

int analyze_value(unsigned int v)
{
    unsigned int i, d, q;

    if (v < 2)
        return 0;
    for (i=0; i<num_divisors; ++i) {
        d = divisors[i];
        for (;;) {
            if (v == 1)
                return 1;
            q = v / d;
            if (d * q != v)
                break;
            v = q;
        }
    }
    return 0;
}