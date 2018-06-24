/*La firma di controllo di un file è lunga 32 bit, ed è costruita considerando il file suddiviso in
blocchi da 4096 byte. La firma di controllo a 32 bit di ciascun blocco è determinata così:

 • I 16 bit meno significativi sono i 16 bit meno significativi della somma
i singoli byte del blocco (0 ≤ i ≤ 4095). P ove bi sono

 • I 16 bit più significativi sono i 16 bit meno significativi della somma
ove b i sono i singoli byte del blocco (0 ≤ i ≤ 4095). P − i) · b i i b i
i (4096

 (Se l’ultimo blocco di un file è lungo meno di 4096 byte, considerare come se i byte mancanti
avessero valore zero.)

 La firma di controllo a 32 bit dell’intero file è lo XOR delle firme di controllo di ciascuno dei
suoi blocchi.

 Scrivere una applicazione C/POSIX multiprocesso e multithread che riceve sulla linea co-
mando un insieme arbitrario di nomi di file. L’applicazione crea un processo per ciascuno dei
file indicati che lavora in modo concorrente rispetto agli altri processi.
Ciascun processo calcola in parallelo la somma di controllo a 32 bit del file utilizzando un
thread per ciascun blocco di 4096 byte del file, e scrive in standard output il nome del file
seguito dal valore della somma di controllo in esadecimale.
Tenere presente gli aspetti relativi alla sincronizzazione tra i flussi di esecuzione, ove necessario.*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#define  BLOCKSIZE (4096)
#define abort_on_error(cond, msg) do { \
    if (cond) { \
        fprintf(stderr, "%s (errno=%d [%m])\n", msg, errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

struct pthread_param {
    pthread_t tid;
    unsigned char *block;
    int block_size;
    uint32_t bcrc;
};

void * block_checksum(void *arg)
{
    struct pthread_param *param = (struct pthread_param *) arg;
    unsigned char *p = param->block;
    int i, msum = 0, lsum = 0;

    for (i=0; i<param->block_size; ++i, ++p) {
        lsum += (*p);
        msum += (*p)*(BLOCKSIZE-i);
    }

    param->bcrc = lsum & 0xffff;   // lsum and 16bit
    param->bcrc |= (msum & 0xffff) << 16;  // or assign
    pthread_exit(NULL);
}

uint32_t compute_checksum_in_parallel(unsigned char *mmbuf, size_t size)
{
    uint32_t crc = 0;
    struct pthread_param *ptpars;
    int i, rc, nblocks, last_block_size;

    nblocks = (size+BLOCKSIZE-1) / BLOCKSIZE;       //numeri di blocchi del file, sommando un blocksize
    last_block_size = size - (nblocks-1)*BLOCKSIZE;     //posizione ultimo blocksize
    ptpars = malloc(nblocks*sizeof(struct pthread_param));
    abort_on_error(ptpars == NULL, "Memory allocation failure");
    for (i=0; i<nblocks; ++i) {
        ptpars[i].block = mmbuf + i * BLOCKSIZE;   //posizione file inizio
        ptpars[i].block_size = ( i < nblocks-1 ? BLOCKSIZE : last_block_size); //operatore ternario
        rc = pthread_create(&ptpars[i].tid, NULL, block_checksum, (void *)(ptpars+i));
        abort_on_error((errno=rc), "Thread creation failure");
    }
    for (i=0 ; i<nblocks; ++i) {
        rc = pthread_join(ptpars[i].tid, NULL);
        crc ^= ptpars[i].bcrc;
    }
    free(ptpars);
    return crc;
}

unsigned char *open_and_map_file(const char *name, size_t *plength)
{
    unsigned char *map;
    off_t flen;

    int fd = open(name, O_RDONLY);
    abort_on_error(fd == -1, "Can't open input file");
    flen = lseek(fd , 0, SEEK_END);
    abort_on_error(flen == (off_t) -1, "Can't seek to end of file");
    map = mmap(NULL, flen, PROT_READ, MAP_PRIVATE, fd, 0);
    abort_on_error(map == MAP_FAILED, "Can't map input file");
    *plength = flen;
    abort_on_error(close(fd) == -1, "Can't close input file");
    return map;
}

void compute_checksum(const char *filename)
{
    size_t filesize;
    unsigned char *mmbuf;
    uint32_t crc;

    mmbuf = open_and_map_file(filename, &filesize);
    crc = compute_checksum_in_parallel(mmbuf, filesize);
    printf("0x%08x %s\n", crc, filename);
}

void create_processes(int numfile, const char *filenames[])
{
    int i;
    for (i=0; i < numfile; ++i) {
        pid_t pid = fork();
        abort_on_error(pid == -1, "Cannot fork a process");
        if (pid == 0) {
            compute_checksum(filenames[i]);
            return;
        }
    }
}

int main(int argc, const char *argv[])
{
    abort_on_error(argc < 2, "Wrong number of command line arguments");
    create_processes(argc-1, argv+1);
    return EXIT_SUCCESS;
}