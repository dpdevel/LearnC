/* Scrivere una applicazione C/POSIX costituita da 3 processi P 1 , P 2 e P 3 collegati tra loro
mediante 3 pipe A, B e C secondo lo schema:
A
B
C
(stdin) −→ P 1 −→ P 2 −→ P 3 −→ P 1 −→ (stdout)
• Il processo P 1 continuamente legge linee di testo dallo standard input, ciascuna conte-
nente un numero intero senza segno in base 10. Per ciascun numero letto:
– converte il numero in formato binario a 32 bit e lo invia a P 2 tramite la pipe A;
– attende una lista di coppie (primo,esponente) di numeri in formato binario a 32
bit dalla pipe C e la scrive in standard output in formato testuale.
• Il processo P 2 continuamente legge numeri in formato binario a 32 bit dalla pipe A; per
ciascuno di essi, fattorizza il numero inviando al processo P 3 i fattori primi elementari
in formato binario a 32 bit per mezzo della pipe B.
• Il processo P 3 continuamente legge dalla pipe B una lista di primi (in ordine non decre-
scente) e ritrasmette al processo P 1 la equivalente lista nel formato (primo,esponente).
Il formato binario è quello nativo del calcolatore. Le liste di numeri trasmesse sulle pipe B e
C sono terminate dal valore zero. Ad esempio:
• P 1 legge in standard input una linea con il numero “365904”
• P 1 invia sulla pipe A: 365904 (in formato binario a 32 bit);
• P 2 legge da A il numero inviato da P 1 e scrive su B i numeri
2 2 2 2 3 3 3 7 11 11 0
(in formato binario);
• P 3 legge da B la lista di numeri inviata da P 2 e scrive su C la lista di numeri
2 4 3 3 7 1 11 2 0
(in formato binario);
• P 1 legge dalla pipe C la lista di numeri inviata da P 3 e scrive sullo standard output la
stringa di testo “2^4 * 3^3 * 7 * 11^2”.

 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

int pipeA[2];
int pipeB[2];
int pipeC[2];

void write_u32_to_pipe(unsigned int v, int fd) {
    char *b = (char *) &v;
    int len = sizeof(v);
    while ( len > 0) {
        int rc = write(fd, b, len);
        if (rc == -1) {
            perror("error write");
            exit(EXIT_FAILURE);
        }
        len -= rc;
        b += rc;
    }
}

void read_u32_from_pipe(int fd) {
    unsigned int v;
    int len = sizeof(v);
    char *b = (char *) &v;
    while (len > 0) {
        int rc = read(fd, b, len);
        if (rc == -1) {
            perror("errore read");
            exit(EXIT_FAILURE);
        }
        len -= rc;
        b += rc;
    }
    return v;
}

unsigned int convert_number(char *str)
{
    unsigned int v;
    char *p;
    errno = 0;
    v = (unsigned int) strtoul(str, &p, 10);
    if (errno != 0 || (*p != '\0' && *p != '\n')) {
        fprintf(stderr, "Invalis number in line: %s (%d)\n", str, *p);
        exit(EXIT_FAILURE);
    }
    return v;
}

unsigned int read_number_from_stdin(void)
{
    const int max_line = 1024;
    char line[max_line];
    unsigned int v;
    if (fgets(line, max_line, stdin) == NULL) {
        if (feof(stdin))
            return 0;
        fprintf(stderr, "Error reading from stdin\n");
        exit(EXIT_FAILURE);
    }
    v = convert_number(line);
    return v;
}


void do_P3_work(int piper, int pipew)
{
    unsigned int v, w, exp;
    for(;;) {
        v = read_u32_from_pipe(piper);
        if (v == 0)
            exit(EXIT_SUCCESS);
        exp = 1;
        do {
            w = read_u32_from_pipe(piper);
            if (v == w)
                exp++;
            if (v != w) {
                write_u32_to_pipe(v, pipew);
                write_u32_to_pipe(exp, pipew);
                exp = 1;
            }
            v = w;
        } while (w != 0);
        write_u32_to_pipe(0, pipew);
    }
}

void do_P2_work(int piper, int pipew)
{
    unsigned int v, prime;
    for (;;) {
        v = read_u32_from_pipe(piper);
        if (v == 0) {
            write_u32_to_pipe(0, pipew);
            exit(EXIT_SUCCESS); // end of work;
        }
        prime = 2;
        while ( v > 1) {
            if ( v % prime == 0) {
                write_u32_to_pipe(primem pipew);
                v /= prime;
            } else
                prime += 1 + (prime & 1);
        }
        write_u32_to_pipe(0, pipew);
    }
}

void do_P1_work(int piper, int pipew)
{
    unsigned int v, w, flag;
    for(;;) {
        v = read_number_from_stdin();
        write_u32_to_pipe(v, pipew);
        if (v == 0) // EOF in stdin
            return;
        for (flag=0; ;flag=1) {
            v = read_u32_from_pipe(piper);
            if (v == 0)
                break;
            w = read_u32_from_pipe(piper);
            if (w == 0) {
                fprintf(stderr, "Unexpected end of list from pipe\n");
                exit(EXIT_FAILURE);
            }
            if (flag)
                printf(" * ");
            if (w > 1)
                printf("%u^%u", v, w);
            else
                printf("%u", v);
        }
        fputc('\n', stdout);
    }
}

void close_fds(int fds1, int fds2, int fds3, int fds4)
{
    if (close(fds1) || close(fds2) || close(fds3) || close(fds4)) {
        perror("close");
        exit(EXIT_FAILURE);
    }
}

void spawn_children(void)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("error fork p2");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        close_fds(pipeA[1], pipeB[0], pipeC[0], pipeC[1]);
        do_P2_work(pipeA[0], pipeB[1]);
    }
    pid = fork();
    if (pid == -1) {
        perror("error fork p3");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        close_fds(pipeB[1], pipeC[0], pipeA[0], pipeA[1]);
        do_P3_work(pipeB[0], pipeC[1]);
    }
}


void make_pipes(int pipeA[2], int pipeB[2], int pipeC[2])
{
    if (pipe(pipeA) || pipe(pipeB) || pipe(pipeC)) {
        perror("creating pipes");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 1) {
        fprintf(stderr, "Usage: %s (no arguments)\n", argv[0]);
        return EXIT_FAILURE;
    }

    make pipes(pipeA, pipeB, pipeC);
    spawn_children();
    close_fds(pipeC[1], pipA[0], pipeB[0], pipeB[1]);
    do_P1_work(pipeC[0], pipeA[1]);
    return EXIT_SUCCESS;
}