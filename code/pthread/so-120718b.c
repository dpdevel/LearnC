/*
b)  Modificare il programma precedente in modo che il calcolo della matrice prodotto sia
    effettuato in parallelo con thread POSIX.
*/

//Basta modificare la funzione matrix_product()

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <pthread.h>        //aggiunto


int main(int argc, char *argv[])
{
    int fdA, fdB;           //file descriptors
    int rA, rB, cA, cB;     //dimensions of input metrices
    int *mA, *mB, *mC;

    if (argc != 4) {
        fprintf(stderr, "Usage: %s <filename> <filename> <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    fdA = open_file(argv[1]);
    map_matrix(fdA, &mA, &rA, &cA);
    if (close(fdA) == -1)
        perror(argv[1]);       // non fatal error

    fdB = open_file(argv[2]);
    map_matrix(fdB, &mB, &rB, &cB);
    if (close(fdB) == -1)
        perror(argv[2]);       // non fatal error

    if (cA != rB) {
        fprintf(stderr, "Input metrices have wrong dim, aborting\n");
        exit(EXIT_FAILURE);
    }

    mC = create_output_matrix(argv[3], rA, cB);
    matrix_product(mC, mA, mB, rA, cA, cB);
    return EXIT_SUCCESS;
}

int open_file(const char *path)
{
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror(path);
        exit(EXIT_FAILURE);
    }
    return fd;
}


//definiamo una struttura per tenere traccia del lavoro svolto dai thread, ogni thread esegue
//un prodotto righe per colonna.
struct thread_job {
    pthread_t tid;
    int *row;
    int *col;
    int colAsize;
    int colBsize;
    int *dest;
};


//legge le dim della matrice del file e mappa la matrice in mem:
void map_matrix(int fd, int **matr, int *row, int *col)
{
    int r, c;
    int *m;

    //read dim matrix
    r = read_int(fd);
    c = read_int(fd);

    if (r <= 0 || c <= 0) {
        fprintf(stderr, "Non positive dimension in input file, aborting\n");
        exit(EXIT_FAILURE);
    }

    *row = r;
    *col = c;

    //create a memory mapping of the matrix
    m = mmap(NULL, (2+r*c)*sizeof(int), PROT_READ, MAP_PRIVATE, fd, 0);
    if (m == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    *matr = m+2;  //sckip the matrix dim
}


//poichè gli interi sono memorizzati nei file nel formato nativo dell'architettura,
//è sufficiete copiare i byte che costituiscono l'intero in una variabile di tipo int,
//mantenendo esattamente l'ordine.

int read_int(int fd)
{
    int v, c;

    c = read(fd, &v, sizeof(v));
    if (c == sizeof(v))
        return v;

    if (c == -1 && errno != EINTR) {
        perror("Read from input file");
        exit(EXIT_FAILURE);
    }

    if (c == 0) {
        fprint(stderr, "Unexpected end of file\n");
        exit(EXIT_FAILURE);
    }

    /* a short read: either a signal, or a partial read from a FIFO*/
    fprint(stderr, "Read operation interruptedm aborting\n");
    exit(EXIT_FAILURE);
}


//Per creare il file di uscita e mapparlo in meme l'unica complicazione è che è necessario
// estendere il file fino alla sua dimensione finale per poterlo mappare in memoria.
// Le dim. della matrice sono note.

int * create_output_matrix(const char *path, int row, int col)
{
    int *m;
    int fd, size = (row*col+2)*sizeof(int);

    //create the output
    fd = open(path, O_RDWR|O_CREAT, 0644);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    //write the dimensions
    write_int(fd, row);
    write_int(fd, col);

    //extend the file size
    if (lseek(fd, size-sizeof(int), SEEK_SET) == -1) {
        perror("lseek");
        exit(EXIT_FAILURE);
    }

    write_int(fd, fd); //actual data does not matter

    m = mmap(NULL, size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (m == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    if (close(fd) == -1)
        perror("close"); //non fatal error

    return  m+2;
}

//Per scrivere un intrel nel file:
void write_int(int fd, int v)
{
    int c;

    c = write(fd, &v, sizeof(v));
    if (c == sizeof(v))
        return;

    if (c == -1 && errno != EINTR) {
        perror("Write to output file");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "Write operation interrupted, aborting\n");
    exit(EXIT_FAILURE);
}


//ogni threade esegue prod_rowcol() per calcolare il prodotto figa-colonna
void *prod_rowcol(void *p)
{
    int k, a, b, v;
    struct thread_job *job = (struct thread_job *) p;

    for (k=v=0; k < job->colAsize; ++k) {
        a = *(job->row + k);
        b = *(job->col + (k*job->colBsize));
        v += a*b;
    }
    *job->dest = v;
    return NULL;
}


//Per calcolare la matrice non possiamo usare la classica notazione m[i][j] in quanto non è
//costante il numero di colonne delle matrice:

void matrix_product(int *C, int *A, int *B, int rA, int cA, int cB)
{
    int i, j;
    struct thread_job *jobs;

    jobs = malloc(rA*cB* sizeof(struct thread_job));
    if (jobs == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (i=0; i<rA; ++i) {
        for (j=0; j<cB; ++j) {
            jobs->row = A + i*cA;
            jobs->col = B+j;
            jobs->colAsize = cA;
            jobs->colBsize = cB;
            jobs->dest = C+(i*cB+j);
            if (pthread_create(&jobs->tid, NULL, prod_rowcol, jobs) != 0) {
                fprintf(stderr, "Error in pthread_create\n");
                exit(EXIT_FAILURE);
            }
            ++jobs;
        }
    }
}


