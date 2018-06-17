/* Scrivere una applicazione C/POSIX multiprocesso costituita da 256 processi concorrenti. L’ap-
plicazione legge un file di input il cui nome è specificato sulla linea comando contenente valori
numerici interi a 8 bit. Ciascuno dei 256 processi è associato ad un distinto valore tra 0 e 255, e
conta il numero di occorrenze di tale valore all’interno del file. Al termine l’applicazione scrive
in standard output il valore numerico (o i valori numerici, in caso di parità) che appare con
maggior frequenza nel file. Curare gli aspetti di sincronizzazione tra i processi, ove necessario.*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/msg.h>


struct msgbuf {
    long mtype;
    int count;
    int value;
};

#define abort_on_error(cond, msg) do { \
    if (cond) { \
        fprintf(stderr, "%s (errno=%d [%m])\n ", msg, errno); \
    exit(EXIT_FAILURE); \
    } \
} while (0)


FILE * open_file(const char *name)
{
    FILE *f;
    f = fopen(name, "r");
    abort_on_error(f == NULL, "Cannot open input file");
    return f;
}


void print_max(const int *freqs, int max)
{
    printf("Max frequency id %d\nValue(s): ", max);
    for (int i=0; i<256; ++i)
        if (freqs[i] == max)
            printf("%hhu ", i);
    putchar('\n');
}


int count_values(FILE *f, int value)
{
    int c = 0, counter = 0;
    for (;;) {
        c = fgetc(f);
        if (c == EOF)
            break;
        if (c != value)
            continue;
        ++counter;
    }
    abort_on_error(c == EOF && !feof(f), "Error reading from input file");
    return counter;
}


void send_value(int msq, int freq, int value)
{
    struct msgbuf m;
    int rc;
    m.mtype = 1;
    m.count = freq;
    m.value = value;
    rc = msgsnd(msq, &m, sizeof(m)-sizeof(long), 0);
    abort_on_error(rc == -1, "Error writing to message queue");
}


void child_work(const char *filename, int value, int msq)
{
    FILE *f = open_file(filename);
    int freq = count_values(f, value);
    send_value(msq, freq, value);
    exit(EXIT_SUCCESS);
}

void fork_children(const char *filename, int msq)
{
    for (int i=1; i<256; ++i)
    {
        pid_t p = fork();
        abort_on_error(p == -1, "Error in fork()");
        if (p == 0)
            child_work(filename, i, msq);
    }
}


int collect_values(int *freqs, int msq)
{
    struct msgbuf m;

    int max = 0;
    for (int i=1; i<256; ++i) {
        ssize_t s = msgrcv(msq, &m, sizeof(m)-sizeof(long), 0, 0);
        abort_on_error(s == -1, "Error reading from message queue");
        abort_on_error(m.value<1 || m.value>255, "Invalid value in message");
        freqs[m.value] = m.count;
        if (m.count > max)
            max = m.count;
    }
    return max;
}


int main (int argc, char *argv[])
{
    FILE *f;
    int freqs[256];
    int v, msq;

    abort_on_error(argc != 2, "Specify a file name as argument");
    f = open_file(argv[1]);
    msq = msgget(IPC_PRIVATE, S_IRUSR|S_IWUSR);
    abort_on_error(msq == -1, "Message queue creation failure");
    fork_children(argv[1], msq);
    freqs[0] = count_values(f, 0);
    v = collect_values(freqs, msq);
    print_max(freqs, v);
    v = msgctl(msq, IPC_RMID, NULL);
    abort_on_error(v == -1, "Message queue destruction failure");
    return EXIT_SUCCESS;
}