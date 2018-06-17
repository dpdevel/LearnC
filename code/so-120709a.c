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

a) Realizzare un programma C/POSIX che riceva come argomento il percorso del file e scriva in standard
   output il prodotto delle somme degli elementi delle colonne della matrice.
   Ad esempio, per la matrice 2 × 2 precedente il risultato è (1.0 + 3.0) · (2.0 + 4.0) = 24.0.

b) Modificare il programma precedente in modo che il calcolo delle somme degli elementi delle colonne sia
   effettuato in parallelo con thread POSIX.
*/

#include <stdio.h>
#include <stdlib.h>


float  sumcol0(float *m, int N)
{
    int r;
    float sum = 0.0;

    for (r=0; r<N; ++r, m+=N)
        sum += *m;
    return sum;
}

//Il calcolo del valore richiesto è delegato alla funzione prodsumcol0( ):
float prodsumcol0(float *m, int N)
{
    int c;
    float prod = 1.0;

    for (c=0; c<N; ++c)
        prod *= sumcol0(m+c, N);
    return prod;
}

/*La funzione read_matrix( ) legge la dimensione della matrice dal file,
 alloca spazio per la matrice in memoria, e poi legge ad uno ad uno gli
 elementi della matrice: */
void read_matrix(FILE *f, float **M, int *N)
{
    int n;
    float *m;

    if (fscanf(f, "%d", &n) != 1) {
        perror("in fscanf");
        exit(EXIT_FAILURE);
    }
    *N = n;
    m = malloc(n*n*sizeof(float));
    if (m == NULL) {
        perror("in malloc");
        exit(EXIT_FAILURE);
    }
    *M = m;   //
    n *= n;   // n^2 righe*colonne
    for (; n > 0; --n) {
        float v;
        if (fscanf(f, "%f", &v) != 1) {
            perror("in fscanf");
            exit(EXIT_FAILURE);
        }
        *m++ = v;
    }
}

//Per aprire il file in lettura controllando gli eventuali errori
FILE *open_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        perror(path);
        exit (EXIT_FAILURE);
    }
    return f;
}

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
    val = prodsumcol0(M, N);
    printf ("Result (sequent): %f\n", val);
}




