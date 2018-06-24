/*Scrivere una applicazione C/POSIX costituita da 3 processi P1, P2 e P3 collegati
tra loro mediante 3 pipe A, B e C secondo lo schema:
                    A      B      C
    (stdin) −→ P 1 −→ P 2 −→ P 3 −→ P 1 −→ (stdout)

 • Il processo P1 continuamente legge linee di testo dallo standard input, ciascuna conte-
nente un numero intero senza segno in base 10. Per ciascun numero letto:
– converte il numero in formato binario a 32 bit e lo invia a P2 tramite la pipe A;
– attende una lista di coppie (primo, esponente) di numeri in formato binario a 32
    bit dalla pipe C e la scrive in standard output in formato testuale

 • Il processo P2 continuamente legge numeri in formato binario a 32 bit dalla pipe A; per
ciascuno di essi, fattorizza il numero inviando al processo P3 i fattori primi elementari
in formato binario a 32 bit per mezzo della pipe B.

 • Il processo P3 continuamente legge dalla pipe B una lista di primi (in ordine non decre-
scente) e ritrasmette al processo P1 la equivalente lista nel formato (primo,esponente).
Il formato binario è quello nativo del calcolatore. Le liste di numeri trasmesse sulle pipe B e
C sono terminate dal valore zero. Ad esempio:

 • P1 legge in standard input una linea con il numero “365904”
 • P1 invia sulla pipe A: 365904 (in formato binario a 32 bit)
 • P2 legge da A il numero inviato da P1 e scrive su B i numeri

            2 2 2 2 3 3 3 7 11 11 0  (in formato binario)

 • P3 legge da B la lista di numeri inviata da P2 e scrive su C la lista di numeri
            2 4 3 3 7 1 11 2 0  (in formato binario)

 • P1 legge dalla pipe C la lista di numeri inviata da P 3 e scrive sullo standard output la
            stringa di testo “2^4 * 3^3 * 7 * 11^2”
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int pipeA[2];   // data dir: from p1 to p2
int pipeB[2];   // data dir: from p2 to p3
int pipeC[2];   // data dir: from p3 to p1

// La funzione make pipes() crea le tre pipe A, B e C:

void make_pipes(int pipeA[2], int pipeB[2], int pipeC[2])
{
    if (pipe(pipeA) || pipe(pipeB) || pipe(pipeC)) {
        perror("Creating pipes");
        exit(EXIT_FAILURE);
    }
}

/*La funzione write u32 to pipe( ) scrive un numero binario su di una pipe,
facendo attenzione ad eventuali scritture terminate prematuramente:*/

void write_u32_to_pipe(unsigned int v, int fd)
{
    int len = sizeof(v);
    char *b = (char *) &v;
    do {
        int rc = (int) write(fd, b, (size_t) len);
        if (rc == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }

        len -= rc;
        b += rc;
    } while (len > 0);
}


/*La funzione read u32 from pipe( ) è analoga: legge un numero in formato
 nativo da una pipe:*/

unsigned int read_u32_from_pipe(int fd)
{
    unsigned int v;
    int len = sizeof(v);
    char *b = (char *) &v;
    do {
        int rc = (int) read(fd, b, (size_t) len);
        if (rc == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        len -= rc;
        b += rc;
    } while (0 < len);
    return v;
}


/*La funzione do P2 work( ) esegue il lavoro richiesto al processo P 2 : legge un
numero dalla pipe A e scrive sulla pipe B la lista dei suoi fattori primi elemen-
tari:*/

void do_P2_work(int piper, int pipew)
{
    unsigned int v, prime;
    for(;;) {
        v = read_u32_from_pipe(piper);
        if (v == 0) {
            write_u32_to_pipe(0, pipew);
            exit(EXIT_SUCCESS);   //end of work
        }
        // factorize v
        prime = 2;
        while ( v > 1) {
            if (v % prime == 0) {   //prime divides v
                write_u32_to_pipe(prime, pipew);
                v /= prime;
                // try again the same prime
            } else
                prime += 1 + (prime & 1);
            // skip even numbers
        }
        write_u32_to_pipe(0, pipew); // end of list
    }
}



/*Si noti che ricevere il numero 0 dalla pipe A viene interpretato come segnale di
terminazione per il processo. Il primo fattore provato è 2, poi vengono provati
tutti i numeri dispari (questo è ovviamente un algoritmo di fattorizzazione molto
inefficiente, ma adeguato per gli scopi di questo esercizio).
La funzione do P3 work( ) esegue il lavoro richiesto al processo P 3 : legge una
lista di fattori primi elementari dalla pipe B e scrive sulla pipe C una lista
equivalente nel formato primo, esponente, primo, esponente, . . . 0.*/

void do_P3_work(int piper, int pipew)
{
    unsigned int v, w ,exp;
    for (;;) {
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
        } while ( w!= 0);
        write_u32_to_pipe(0, pipew); //end of list
    }
}


unsigned int convert_number(char *str)
{
    unsigned int v;
    char *p;
    errno = 0;
    v = (unsigned int) strtoul(str, &p, 10);
    if (errno != 0 || (*p != '\0' && *p != '\n')) {
        fprintf(stderr, "invalid number in line: %s (%d)\n", str, *p);
        exit(EXIT_FAILURE);
    }
    return v;
}


/*La funzione read_number_from_stdin() legge una linea di testo dallo standard
input ed interpreta il contenuto come un numero in base 10 utilizzando la
funzione convert number():*/

unsigned int read_number_from_stdin(void)
{
    const int max_line = 1024;
    char line[max_line];
    unsigned int v;
    if (fgets(line, max_line, stdin) == NULL) {
        if (feof(stdin))
            return 0;
        printf(stderr, "Error reading from stdin\n");
        exit(EXIT_FAILURE);
    }
    v = convert_number(line);
    return v;
}


/* Le funzioni do_P1_work(), do_P2_work() e do_P3_work() sono specifiche per
ciascuno dei tre processi.
do_P1_work( ) continuamente legge righe di testo dallo standard input. Per
ciascuna linea, interpreta il suo contenuto come un numero senza segno in base
10, lo converte in formato nativo (binario) a 32 bit, e lo trasmette sulla pipe A
al processo P 2 . Infine legge una lista di numeri dalla pipe C, e li stampa nel
formato opportuno in standard output: */

void do_P1_work(int piper, int pipew)
{
    unsigned int v, w, flag;
    for (;;) {
        v = read_number_from_stdin();
        write_u32_to_pipe(v, pipew);
        if (v == 0) // EOF in stdin
            return;
        for (flag=0; ;flag=1) {
            v = read_u32_from_pipe(piper);
            if (v == 0)
                break; // end of list
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


// La funzione close_fds() chiude quattro descrittori di file passati in argomento:

void close_fds(int fd1, int fd2, int fd3, int fd4)
{
    if (close(fd1) || close(fd2) || close(fd3) || close(fd4)) {
        perror("close");
        exit(EXIT_FAILURE);
    }
}


// La funzione spawn_children() crea ed avvia l'esecuzione dei processi figli:

void spawn_children(void)
{
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork(P2)");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        close_fds(pipeA[1], pipeB[0], pipeC[0], pipeC[1]);
        do_P2_work(pipeA[0], pipeB[1]);
    }
    pid = fork();
    if (pid == -1) {
        perror("fork(P3)");
        exit(EXIT_FAILURE);
    }
    if (pid == 0) {
        close_fds(pipeB[1], pipeC[0], pipeA[0], pipeA[1]);
        do_P3_work(pipeB[0], pipeC[1]);
    }
}



int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s (no arguments)\n", argv[0]);
        return EXIT_FAILURE;
    }
    make_pipes(pipeA, pipeB, pipeC);
    spawn_children();
    close_fds(pipeC[1], pipeA[0], pipeB[0], pipeB[1]);
    do_P1_work(pipeC[0], pipeA[1]);
    return EXIT_SUCCESS;
}