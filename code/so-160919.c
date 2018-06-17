/*Scrivere una applicazione C/POSIX multithread costituita da 12 thread concorrenti (da T 1 a
T 12 ). L’applicazione legge un file di input tramite memory mapping con blocchi di dimensione
massima di 4096 byte. Il nome del file è specificato sulla linea comando, e si assume che esso
contenga valori numerici di 8 bit in formato nativo del calcolatore. Il thread T i (per i da 1 a
12) analizza le porzioni contigue di 2 i byte del file e scrive in standard output la somma dei
valori da 8 bit di ciascuna porzione. Quindi, per esempio, il thread T 1 scriverà in standard
output la somma del primo e secondo byte del file, poi la somma del terzo e quarto byte del
file, eccetera; il thread T 2 scriverà in standard output la somma dei primi 4 byte del file, poi la
somma dei successivi 4 byte, e così via. L’applicazione deve funzionare con file di qualunque
lunghezza, ma si possono trascurare i problemi di overflow quando si superano i limiti della
dimensione nativa del calcolatore. Curare gli aspetti di sincronizzazione tra i thread, ove
necessario.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/mman.h>

int g_fd;                       //descrittore del file di input
unsigned char * g_buf;          //indirizzo di un buffer contenente un blocco di dati
size_t g_len;                   //lunghezza
pthread_barrier_t g_barrier;    //consente ai vari thread di sincronizzarsi

#define BLOCKSIZE 4096

#define abort_on_error(cond, msg) do { \
    if (cond) { \
        fprintf(stderr, "%s (errno=%d [%m])\n ", msg, errno); \
    exit(EXIT_FAILURE); \
    } \
} while (0)

void init_barrier(pthread_barrier_t *pb)
{
    int rc = pthread_barrier_init(pb, NULL, 12);
    abort_on_error(rc!=0 && ((errno=rc)), "Canoot initialize barrier");
}

int wait_on_barrier(void)
{
    int rc = pthread_barrier_wait(&g_barrier);
    if (rc == 0 || rc == PTHREAD_BARRIER_SERIAL_THREAD)
        return rc;
    abort_on_error(((errno=rc)), "Error in pthread_barrier_wait()");
    return 0;
}


unsigned char * mmap_at_offset(int fd, off_t offset)
{
    void *p = mmap(NULL, BLOCKSIZE, PROT_READ, MAP_PRIVATE, fd, offset);
    abort_on_error(p==MAP_FAILED, "Error in mmap()");
    return  p;
}


void compute_sums(int size)
{
    unsigned long value;
    int p, q;

    for (p=0; p<BLOCKSIZE; p+=size) {
        value = 0;
        for (q = 0; q < size; ++q)
            value += *(g_buf + p + q);
        printf("%d-block sum: %lu\n", size, value);
    }
}


void *thread_worker(void *arg)
{
    int size = (int) arg, selected;
    size_t curoff = 0;

    for (;;) {
        compute_sums(size);
        selected = wait_on_barrier();
        curoff += BLOCKSIZE;
        if (curoff >= g_len)
            break;
        if (selected) {
            int rc = munmap(g_buf, BLOCKSIZE);
            abort_on_error(rc==-1, "Error in munmap()");
            g_buf = mmap_at_offset(g_fd, curoff);
        }
        wait_on_barrier();
    }
    pthread_exit(0);
}

void spawn_threads(void)
{
    int i, rc, size;
    pthread_t tid;

    for (i=1, size=2; i<=12; ++i, size*=2) {
        rc = pthread_create (&tid, NULL, thread_worker, (void *)size);
        abort_on_error(rc!=0 && ((errno=rc)), "Thread creation failed");
    }
}

int open_file(const char *name)
{
    int fd = open(name, O_RDONLY);
    abort_on_error(fd==-1, "Cannot open input file");
    return fd;
}

size_t get_file_lenght(int fd)
{
    off_t v = lseek(fd, 0, SEEK_END);
    abort_on_error(v==(off_t)-1, "Error in lseek()");
    return  (size_t) v;
}


int main(int argc, char *argv[])
{
    abort_on_error(argc<2, "Missing file name in command line");
    g_fd = open_file(argv[1]);
    g_len = get_file_lenght(g_fd);
    g_buf = mmap_at_offset(g_fd, 0);
    init_barrier(&g_barrier);
    spawn_threads();
    pthread_exit(0);
}



