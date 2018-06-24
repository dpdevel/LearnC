/* Scrivere una applicazione C/POSIX multiprocesso costituita da 26 processi concorrenti. L'ap-
plicazione legge un le di input il cui nome è specicato sulla linea comando, e che contiene
un le di testo ASCII. Ciascuno dei 26 processi è associato ad una lettera dell'alfabeto, e
conta il numero di parole nel le che iniziano con la corrispondente lettera.
Una parola è denita come una sequenza di caratteri alfabetici consecutivi; numeri, spazi bianchi, segni di
punteggiatura, e tutti gli altri caratteri non alfabetici separano le parole tra loro ma non ne
fanno parte. Al termine l'applicazione scrive in standard output la lettera dell'alfabeto (o le
lettere dell'alfabeto, in caso di parità) che appare con maggior frequenza tra le iniziali delle
parole nel le. Curare gli aspetti di sincronizzazione tra i processi, ove necessario.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define abort_on_error(cond, msg) do { \
    if (cond) { \
        fprintf (stderr, "%s (errno=%d [%m])\n", msg, errno); \
        exit(EXIT_FAILURE); \
    } \
} while(0)


struct msgbuf {
    long mtype;
    int count;
    char letter;
};


int collect_values(int *freqs, int msq)
{
    struct msgbuf m;
    int max = 0;
    for (int i=1; i<26; ++i) {
        ssize_t  s = msgrcv(msq, &m, sizeof(m)-sizeof(long), 0, 0);
        abort_on_error(s == -1, "Error reading from message queue");
        abort_on_error(m.letter < 'a' || m.letter > 'z', "Invalid letter in msg");
        freqs[m.letter - 'a'] = m.count;
        if (m.count > max)
            max = m.count;
    }
    return max;
}


void print_max(int *freqs, int max)
{
    fprintf(stdout, "Max freq. is %d\nLetter(s): ", max);
    for (int i = 0; i < 26; ++i)
        if (freqs[i] == max) {
            fputc('a'+i, stdout);
            fputc(' ', stdout);
        }
    fputc('\n', stdout);

}

int is_alfa(char c)
{
    if (c >= 'a' && c <= 'z')
        return 1;
    if (c >= 'A' && c <= 'Z')
        return 1;
    return 0;
}

int count_words(FILE *f, char letter)
{
    int c = 0, prev_c, words = 0;
    char Letter = (char) ((letter - 'a') + 'A');

    for (;;) {
        prev_c = c;
        c = fgetc(f);
        if (c == EOF)
            break;
        if (c != letter && c != Letter)
            continue;
        if (is_alfa((char) prev_c))
            continue;
        ++words;
    }
    abort_on_error(c == EOF && !feof(f), "Error reading from input file");
    return words;
}


void send_value(int msq, int freq, char letter)
{
    struct msgbuf m;
    int rc;
    m.mtype = 1;   //qualsiasi valore tranne 0
    m.count = freq;
    m.letter = letter;
    rc = msgsnd(msq, &m, sizeof(m)-sizeof(long), 0);
    abort_on_error(rc == -1, "Error writing to message queue");
}


FILE * open_file(const char *name)
{
    FILE *f;
    f = fopen(name, "r+");
    abort_on_error(f == NULL, "Cannot open input file");
    return f;
}


void child_work(const char *filename, char letter, int msq)
{
    FILE *f = open_file(filename);
    int freq = count_words(f, letter);
    send_value(msq, freq, letter);
    exit(EXIT_SUCCESS);
}


void fork_children(const char *filename, int msq)
{
    for (int i=1; i<26; ++i) {
        pid_t p = fork();
        abort_on_error(p == -1, "Error in fork()");
        if (p == 0)
            child_work(filename, (char) ('a' + i), msq);
    }
}


int main(int argc, char *argv[])
{
    FILE *f;
    int v, msq, freqs[26];

    abort_on_error(argc != 2, "Specify a file name as argument");
    f = open_file(argv[1]);
    msq = msgget(IPC_PRIVATE, S_IRUSR|S_IWUSR);     //IPC_PRIVATE coda solo per processi figli
                                                    //S_IRUSR w S_IWUSR garantiscono r e w per l'utente
    abort_on_error(msq == -1, "Message queue creation failure");
    fork_children(argv[1], msq);
    freqs[0] = count_words(f, 'a');
    v = collect_values(freqs, msq);
    if (freqs[0] > v)
        v = freqs[0];
    print_max(freqs, v);
    v = msgctl(msq, IPC_RMID, NULL);
    abort_on_error(v == -1, "Message queue destruction failure");
    return EXIT_SUCCESS;
}




/*  fseek(f, 0, SEEK_END);
    fputs("questa è una nuova riga\n", f);
    fflush(f);

    fseek(f,0, SEEK_SET);
    for(;;) {
        code = fgets(line, 256, f);
        if (code == NULL)
            break;
        printf("questa è la riga: %s\n", line);*/


/*
f = open_file(argv[1]);
for(;;) {
    text = fgetc(f);
    one = (char) text;
    if (text == EOF)
        break;
    printf("questo è il carattere %c\n", one);
}
*/

/*
f = open_file(argv[1]);
for(;;) {
    code = fgets(line, 256, f);
    if (code == NULL)
        break;
    printf("questo è il carattere %s\n", line);
}
*/
