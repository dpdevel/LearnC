/*Un file contiene record di lunghezza variabile memorizzati con il seguente formato:
            n 1 record1 n 2 record2 n 3 record3 · · ·

ove n i rappresenta la dimensione in byte dell’i-esimo record nel formato nativo del calcolatore,
e recordi è la sequenza di byte corrispondente al contenuto dell’i-esimo record. Scrivere
una applicazione C/POSIX costituita da due processi che comunicano tramite una pipe. Il
primo processo legge il file ed trasmette sulla pipe al secondo processo un record alla volta
rovesciando l’ordine dei suoi byte. Il secondo processo deve scrivere sullo standard output il
contenuto dei record ricevuti, evidenziando chiaramente l’inizio e la fine di ciascun record.
*/


/*Risolviamo l’esercizio con un approccio top-down. Le operazioni principali
da svolgere nella funzione main( ) sono: creazione della pipe, creazione di un
processo figlio, apertura e processamento del file di ingresso.
Poiché il testo dell’esercizio non specifica la modalità con cui il file di input
viene selezionato, scegliamo di leggerne il nome dalla linea comando.*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void create_pipe(int pfd[2])
{
    int rc = pipe(pfd);
    if (rc == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
}

void spawn_child(int pfd[2])
{
    pid_t p = fork();
    if (p == -1) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    if (p != 0)
        return;
    print_records(pfd);
}

int open_file(const char *name)
{
    int fd = open(name, O_RDWR);
    if (fd == -1) {
        perror(name);
        exit(EXIT_FAILURE);
    }
    return fd;
}

void close_fd(int fd)
{
    if (close(fd) == -1) {
        perror("close");
        exit(EXIT_FAILURE);
    }
}

void print_records(int pfd[2])
{
    char *buf;
    close_fd(pfd[1]);
    for (;;) {
        int n = read_int(pfd[0]);
        if (n == 0)
            exit(EXIT_SUCCESS);
        buf = read_record(pfd[0], n);
        printf("--- START OF RECORD ---\n");
        if (fwrite(buf, n, 1, stdout) != 1u) {
            perror("stdout");
            exit(EXIT_FAILURE);
        }
        printf("\n--- END OF RECORD ---\n");
        free(buf);
    }
}

void write_buf(int fd, char *rec, int size)
{
    while (size > 0) {
        int rc = write(fd, rec, size);
        if (rc == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        rec += rc;
        size -= rc;
    }
}

void invert_bytes(char *rec, int len)
{
    char *last;
    for (last=rec+(len-1); rec < last; ++rec, --last) {
        char c = *rec;
        *rec = *last;
        *last = c;
    }
}

char * read_record(int fd, int size)
{
    char *p, *q;
    p = q = malloc(sizeof(char)*size);
    if (p == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    while (size > 0) {
        int rc = read(fd, q, size);
        if (rc == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        if (rc == 0) {
            fprintf(stderr, "Unexpected end of file\n");
            exit(EXIT_FAILURE);
        }
        q += rc;
        size -= rc;
    }
    return p;
}

int read_int(int fd)
{
    size_t len = sizeof(int);
    int rc, v;
    char *p = (char *) &v;
    do {
        rc = read(fd, p, len);
        if (rc == -1) {
            perror(NULL);
            exit(EXIT_FAILURE);
        }
        if (rc == 0) {
            if (len != sizeof(int))
                fprintf(stderr,"Unexpected end og file\n");
            return 0;
        }
        len -= rc;
        p += rc;
    } while (len > 0);
    return v;
}

void send_records(int ifd, int ofd)
{
    for (;;) {
        char *buf;
        int n = read_int(ifd);
        if (n == 0)
            return;
        buf = read_record(ifd, n);
        invert_bytes(buf, n);
        write_buf(ofd, (char *)&n, sizeof(n));
        write_buf(ofd, buf, n);
        free(buf);
    }
}

int main(int argc, char *argv[])
{
    int pfd[2], fd;
    if (argc != 2) {
        fprintf(stderr, "usage: %s <file>\n", argv[0]);
        return EXIT_FAILURE;
    }
    create_pipe(pfd);
    spawn_child(pfd);
    close_fd(pfd[0]);
    fd = open_file(argv[1]);
    send_records(fd, pfd[1]);
    close_fd(pfd[1]);
    close_fd(fd);
    return EXIT_SUCCESS;
}

