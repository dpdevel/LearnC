/*Una matrice rettangolare di numeri di tipo int è memorizzata in un file con il
seguente formato binario:

 • All’inizio del file sono memorizzate le dimensioni N (numero di righe) e M (numero
di colonne) della matrice come sequenza di byte corrispondenti al formato nativo del
calcolatore.

 • Di seguito sono memorizzati riga per riga gli N × M elementi della matrice, ciascuno
come sequenza di byte nel formato nativo del calcolatore.

 Ad esempio, se il calcolatore è little-endian e con interi int di 32 bit, la matrice

 |100 200|
 |300 400|
 |500 600|

 è memorizzata con la sequenza di byte nel file:
        3 0 0 0 2 0 0 0 100 0 0 0 200 0 0 0 44 1 0 0 144 1 0 0 244 1 0 0 88 2 0 0

Realizzare una applicazione C/POSIX multiprocesso che riceva come argomento tre percorsi
di file. I primi due file sono di ingresso e contengono ciascuno una matrice rettangolare
con la codifica appena descritta, con dimensioni (non prefissate) N × M e M × P . Il terzo
file è di uscita: alla fine dell’esecuzione dovrà memorizzare con la stessa codifica la matrice
corrispondente al prodotto “riga-colonna” delle due matrici di ingresso.

 Se le dimensioni delle matrici rettangolari di ingresso non consentono di effettuare il prodotto
(ossia il numero di colonne della prima matrice non coincide con il numero di righe della
seconda matrice) l’applicazione deve terminare segnalando un errore. L’applicazione deve
utilizzare N × P processi, ciascuno dei quali svolgerà il calcolo di un elemento della matrice
d’uscita. Si tengano in considerazione gli eventuali problemi dovuti all’esecuzione concorrente
dei vari processi. */


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int open_file(const char *path)
{
    int fd;
    fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Error in open file\n");
        exit(EXIT_FAILURE);
    }
    return fd;
}

int read_int(int fd)
{
    int v, c;

    c = (int) read(fd, &v, sizeof(v));
    if (c == sizeof(v))
        return v;
    if (c == -1 && errno != EINTR) {
        perror("Read from input file");
        exit(EXIT_FAILURE);
    }

    if ( c == 0) {
        fprintf(stderr, "Unexpected end of file\n");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Read operation interrupted\n");
    exit(EXIT_FAILURE);
}


void map_matrix(int fd, int **matr, int *row, int *col)
{
    int r, c;
    int *m;

    r = read_int(fd);
    c = read_int(fd);

    if (r <= 0 || c <=0) {
        fprintf(stderr, "Non positive dim, aborting\n");
        exit(EXIT_FAILURE);
    }

    *row = r;
    *col = c;

    m = mmap(NULL,(2+r+c)*sizeof(int), PROT_READ, MAP_PRIVATE, fd, 0);
    if (m == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    *matr = m+2;
}


void prod_rowcol(int i, int j, int *C, int *A, int *B, int cA, int cB)
{
    int k, v;
    int *row, *col;

    row = A + (i*cA);
    col = B + j;
    for (k = v = 0; k < cA; ++k, col += cB)
        v += (*row) * (*col);
    *(C+(i*cB)+j) = v;
}


void matrix_product(int *C, int *A, int *B, int rA, int cA, int cB)
{
    int i, j;
    pid_t pid;

    for (i=0; i<rA; ++i)
        for (j=0; j<cB; j++) {
            pid = fork();
            if (pid == -1) {
                fprintf(stderr, "Cannot fork a process\n");
                exit(EXIT_FAILURE);
            }
            if (pid == 0) {
                prod_rowcol(i, j, C, A, B, cA, cB);
                return;
            }
        }
}


void write_int(int fd, int v)
{
    int c;

    c = (int) write(fd, &v, sizeof(v));
    if ( c == sizeof(v))
        return;

    if ( c == -1 && errno != EINTR) {
        perror("Write to output file");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Write operation interrupted, aborting\n");
    exit(EXIT_FAILURE);
}


int * create_output_matrix(const char *path, int row, int col)
{
    int *m;
    int fd, size = (row*col+2)*sizeof(int);

    fd = open(path, O_RDWR|O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    write_int(fd, row);
    write_int(fd, col);

    if (lseek(fd, size - sizeof(int), SEEK_SET) == -1) {
        perror("lseek");
        exit(EXIT_FAILURE);
    }

    write_int(fd, fd);

    m = mmap(NULL, (size_t) size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1)
        perror("close");

    return m+2;
}


int main(int argc, char *argv[])
{
    int fdA, fdB; // file descriptors
    int rA, rB, cA, cB; // dimensions of input matrics
    int *mA, *mB, *mC;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <file> <file> <file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fdA = open_file(argv[1]);
    map_matrix(fdA, &mA, &rA, &cA);
    if (close(fdA == -1))
            perror(argv[1]);  //no fatal error

    fdB = open_file(argv[2]);
    map_matrix(fdB, &mB, &rB, &cB);
    if (close(fdB == -1))
        perror(argv[2]);  //no fatal error

    if (cA != rB) {
        fprintf(stderr, "Input matrices have wrong dim, aborting\n");
        exit(EXIT_FAILURE);
    }

    mC = create_output_matrix(argv[3], rA, cB);
    matrix_product(mC, mA, mB, rA, cA, cB);
    return EXIT_SUCCESS;
}




















