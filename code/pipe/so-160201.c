/*Scrivere una applicazione C/POSIX costituita da due processi P 1 e P 2 collegati con una pipe.
Il processo P 1 legge dalla linea comando il nome di un file di testo contenente righe con il
seguente formato:

        <numero> <operazione> <numero> <operazione> <numero> ...

 ove <numero> è la rappresentazione decimale di un numero intero, e <operazione> è uno
dei caratteri “+”, “-”, “*”, “/”. Tra ogni elemento della linea (<numero> o <operazione>)
possono essere presenti zero, uno o più caratteri spazio bianco (“ ”).
Per ciascuna riga del file, il processo P 1 controlla la sua correttezza formale, e invia sulla pipe
l’espressione numerica corrispondente con il seguente formato:

        valore <operazione> valore <operazione> valore ... =

 ove valore è la codifica del numero corrispondente in formato nativo del calcolatore (int),
<operazione> ha la medesima interpretazione descritta sopra, ed il carattere “=” indica la
fine dell’espressione.
Il processo P 2 legge dalla pipe ciascuna espressione e scrive in standard output una linea di
testo con il risultato corrispondente.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MAXLINE 256
#define MAXNUMBERS (MAXLINE/2)
#define exit_on_error(cond, msg) do { \
    if (cond) { \
        fprintf(stderr," %s (%d)\n",msg, errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)



char get_operation(int fd)
{
    int rc;
    char v;
    rc = (int) read(fd, &v, 1);
    if (rc == 0) {
        fprintf(stderr, "Truncated data in pipe\n");
        exit(EXIT_FAILURE);
    }
    exit_on_error(rc != 1, "read");
    return v;
}

int get_number(int fd)
{
    int rc, v;
    rc = (int) read(fd, &v, sizeof(int));
    if (rc == 0)
        exit(EXIT_FAILURE);
    exit_on_error(rc != sizeof(int), "read");
    return v;
}


void child_task(int pfd)
{
    for(;;) {
        int res = 0;
        char o = '+';
        do {
            int v = get_number(pfd);
            switch (o) {
                case '+':
                    res += v;
                    break;
                case '-':
                    res -= v;
                    break;
                case '*':
                    res *= v;
                    break;
                case '/':
                    res /= v;
                    break;
                default:
                    fprintf(stderr, "Invalid operation (%c)\n",o);
                    exit(EXIT_FAILURE);
            }
            o =get_operation(pfd);
        } while (o != '=');
        printf("%d\n", res);
    }
}


void send_to_pipe(int n, int *val, char *ops, int pfd)
{
    int i, rc;
    for (i=0; i<n; ++i) {
        rc = (int) write(pfd, val + i, sizeof(int));
        exit_on_error(rc != sizeof(int), "write");
        rc = (int) write(pfd, ops + i, 1);
        exit_on_error(rc != 1, "write");
    }
}

const char *read_number(const char *buf, int *value)
{
    char *p;
    errno = 0;

    *value = (int) strtol(buf, &p, 10);
    if (errno != 0)
        return NULL;
    return p;
}

const char *skip_blanks(const char *buf)
{
    while (*buf == ' ')
        ++buf;
    return buf;
}

const char *read_operation(const char *buf, char *op)
{
    buf = skip_blanks(buf);
    switch (*buf) {
        case '+':
        case '-':
        case '*':
        case '/':
            *op = *buf;
            break;
        case '\n':
        case '\0':
            *op = '=';
            return buf;
        default:
            return NULL;
    }
    return buf+1;
}

void process_line(const char *line, int pipefd)
{
    int values[MAXNUMBERS];
    char opers[MAXNUMBERS];
    int ci = 0;
    const char *p = line;

    while (*p != '\n' && *p !='\0') {
        p = read_number(p, values+ci);  //values + ci incrementa indirizzo puntatore in base al numero di operandi
                                        //esempio: 3 operandi --> 1457366304 + 1457366308 + 1457366312
        if (p != NULL)
            p = read_operation(p, opers+ci);
        if (p == NULL) {
            fprintf(stderr, "Invalid format in line: %s\n", line);
            return;
        }
        ++ci;
    }
    send_to_pipe(ci, values, opers, pipefd);
}

void read_and_process_line(FILE *inf, int pfd)
{
    char linebuf[MAXLINE], *p;
    p = fgets(linebuf, MAXLINE, inf);
    if (p == NULL)
        exit_on_error(ferror(inf), "input file");
    else
        process_line(linebuf, pfd);
}

void read_file(const char *filename, int pipefd)
{
    FILE *inf = fopen(filename, "r");
    exit_on_error(inf == NULL, filename);
    while (!feof(inf))
        read_and_process_line(inf, pipefd);
}

void spawn_child(int fds[])
{
    int rc;
    pid_t p = fork();
    exit_on_error(p == -1, "fork error");
    if (p == 0) {
        rc = close(fds[1]);
        exit_on_error(rc == -1, "child: close pipe 1 error");
        child_task(fds[0]);
    }
    rc = close(fds[0]);
    exit_on_error(rc == -1, "father: close pipe 0 error");
}

void create_pipe(int fds[])
{
    int rc = pipe(fds);
    exit_on_error(rc == -1, "pipe error");
}

int main(int argc, char *argv[])
{
    int pfds[2];
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }
    create_pipe(pfds);
    spawn_child(pfds);
    read_file(argv[1], pfds[1]);
    return EXIT_SUCCESS;
}