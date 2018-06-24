/*Scrivere una applicazione C/POSIX multithread che legge un file di input il cui nome è speci-
ficato come primo argomento della linea comando. Il file di input deve essere letto utilizzando
la chiamata di sistema a basso livello read(). Si assuma che il file contenga valori numerici di
8 bit in formato nativo del calcolatore. L’applicazione è costituita da 256 thread concorrenti
(da T 0 a T 255 ). Il thread T i (per ogni i da 0 a 255) dovrà scrivere in standard output il numero
di occorrenze di byte con valore i all’interno del file di input. L’applicazione deve funzionare
con file di qualunque lunghezza, ma si possono trascurare i problemi di overflow quando si
superano i limiti della dimensione nativa del calcolatore. Curare gli aspetti di sincronizzazione
tra i thread, ove necessario.*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>

#define BLOCKSIZE 4096

#define abort_on_error(cond, msg) do { \
    if (cond) { \
        fprintf(stderr, "%s (errno=%d [%m])\n ", msg, errno); \
    exit(EXIT_FAILURE); \
    } \
} while (0)

unsigned char g_buf[BLOCKSIZE];     //buffer contenente un blocco di  4096
int g_fd;                           //descrittore file di input
size_t g_len;                       //quantità di dati presenti in g_buf
pthread_barrier_t g_barrier;        //consete ai vari thread di sincronizzarsi


int open_file(const char *name)
{
    int fd = open(name, O_RDONLY);
    abort_on_error(fd==-1, "Non e possibile aprire il file");
    return fd;
}

size_t read_next_block(int fd, unsigned char *buf)
{
    size_t toread = BLOCKSIZE;
    while (toread > 0) {
        ssize_t rc = read(fd, buf, toread);
        abort_on_error(rc==-1, "Error in read()");
        if (!rc)
            break;
        buf += rc;
        toread -= rc;
    }
    return BLOCKSIZE-toread;
}

void init_barrier(pthread_barrier_t *pb)
{
    int rc = pthread_barrier_init(pb, NULL, 256);
    abort_on_error(rc!=0 && ((errno=rc)), "Errore init barrier");
}

int wait_on_barrier(void)
{
    int rc = pthread_barrier_wait(&g_barrier);
    if (rc == 0 || rc == PTHREAD_BARRIER_SERIAL_THREAD)
        return rc;
    abort_on_error(((errno=rc)), "Error in pthread_barrier_wait()");
    return 0;
}

unsigned long count_values(unsigned char target, int bsize)
{
    int i;
    unsigned long t=0;

    for (i=0; i<bsize; ++i)
        if (g_buf[i] == target)
            ++t;
    return t;
}

void *thread_worker(void *arg)
{
    unsigned char target = (int) arg;
    int selected;
    unsigned long sum = 0;

    while (g_len > 0) {
        sum += count_values(target, g_len);
        selected =  wait_on_barrier();
        if (selected)
            g_len = read_next_block(g_fd, g_buf);
        wait_on_barrier();
    }
    printf("%d-values: %lu\n", target, sum);
    pthread_exit(0);
}

void spawn_threads(void)
{
    int i, rc;
    pthread_t tid;

    for (i=0; i<256; ++i) {
        rc = pthread_create(&tid, NULL, thread_worker,(void *) i);
        abort_on_error(rc!=0 && ((errno=rc)), "Errore spwan thread");
    }
}

int main(int argc,char *argv[])
{
    abort_on_error(argc<2, "Nessun file di input passato al programma");
    g_fd = open_file(argv[1]);
    g_len = read_next_block(g_fd, g_buf);
    init_barrier(&g_barrier);
    spawn_threads();
    pthread_exit(0);
}

