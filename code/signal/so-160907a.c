/*Esercizio 1
Scrivere una applicazione C/POSIX multiprocesso che legge da un file di input il cui nome è
specificato come primo argomento della linea comando. Il file è costituito da righe di testo
con il seguente formato:
            numero decimale
            spazio
            stringa di testo
Il numero in formato testo decimale è da considerare come un ritardo periodico in secondi.
Per ciascuna riga del file con il formato corretto l’applicazione crea un processo che attende
il numero di secondi indicato e poi esegue il comando (con eventuali argomenti) specificato
dalla stringa di testo. Dopo la conclusione del comando, il processo attende ancora il numero
di secondi indicato e poi torna ad eseguire il comando, e cosı̀ via.
Ad esempio, se il file contiene le seguenti due righe:
            100 ls -l /tmp
            60 date
dovranno essere creati due nuovi processi: un processo eseguirà ls, che elencherà in standard
output il contenuto della directory /tmp, ad intervalli di 100 secondi; l’altro processo eseguirà
date, che scriverà in standard output data e ora corrente, ad intervalli di 60 secondi.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE_LENGHT 4096
#define abort_on_error(cond, msg) do { \
    if (conf) { \
        fprintf(stderr, "%s (%d)\n", msg, errno); \
        exit(EXIT_FAILURE); \
    } \
} while(0)

int compute_argc(char *p)
{
    int argc = 1;
    if (p == NULL)
        return 0;
    for (;;) {
        p =strchr(p, ' ');
        if (p == NULL)
            return argc;
        while (*p == ' ')
            ++p;
        ++argc;
    }
}

char **convert_string_to_argv(char *cmdstr)
{
    char **argv, *p;
    int i, argc;
    argc = compute_argc(cmdstr);
    argv = malloc(sizeof(argv[0])*(argc+1));
    abort_on_error(argv == NULL, "Memory allocation error");

    for (i=0, p=cmdstr; i<argc; ++i) {
        argv[i] = p;
        p = strchr(p, ' ');
        for (; p != NULL && *p == ' '; *p++ = '\0')
            ;
    }
    argv[argc] = NULL;
    return argv;
}

pid_t spawn_command(char *cmdstr)
{
    char **argv;

    pid_t pid = fork();
    abort_on_error(pid == -1, "Error in fork()");
    if (pid != 0)
        return pid; // parent, go out

    argv = convert_string_to_argv(cmdstr);
    execvp(argv[0], argv);
    abort_on_error(1, "Cannot execute command");
    return 0;
}

void spawn_periodic_command(unsigned long delay, char *cmdstr)
{
    int rc, status;
    pid_t pid = fork();
    abort_on_error(pid == -1, "Error in fork()");

    if (pid != 0)
        return;

    sleep(delay);
    pid = spawn_command(cmdstr);

    for (;;) {
        rc = wait(&status);     // sospende il processo corrente finche' un figlio (child) termina o
                                // finche' il processo corrente riceve un segnale di terminazione,
                                // ritorna il process ID del child che termina

        abort_on_error(rc == -1, "Error in wait()");
        if (pid == rc && !WIFSIGNALED(status) && !WIFCONTINUED(status)) {
            sleep(delay);
            pid = spawn_command(cmdstr);
        }
    }
}
unsigned long read_value(char *line, char **nextch)
{
    unsigned long delay;
    char *p;

    errno = 0;
    delay = strtoul(line, &p, 10);  //&p indirizzo del puntatore successivo al primo char
    if (errno != 0 || *p != ' ') {
        fprintf(stderr, "Invalid line: %s\n", line);
        *nextch = NULL;
        return 0;
    }
    *nextch = ++p;
    return delay;
}

void process_line(char *line)
{
    unsigned long delay;
    char *cmdstr;
    line[strlen(line)-1]='\0';
    delay = read_value(line, &cmdstr);
    if (cmdstr)
        spawn_periodic_command(delay, cmdstr);
}

char *get_next_line(FILE *f, char *buf)
{
    char *p;
    p = fgets(buf, MAX_LINE_LENGHT, f);
    abort_on_error(p == NULL && errno(f), "Error reading from input file");
    return p;
}

void read_all_lines(FILE *f)
{
    char lbuf[MAX_LINE_LENGHT], *line;
    while (!feof(f))
    {
        line = get_next_line(f, lbuf);
        if (line != NULL)
            process_line(line);
    }
}

void read_input_file(const char *filename)
{
    FILE *f;
    int rc;
    f = fopen(filename, "r");
    abort_on_error(f == NULL, "Error in fopen");
    read_all_lines(f);
    rc = fclose(f);
    abort_on_error(rc != 0, "Error in fclose");
}

int main(int argc, char *argv[])
{
    abort_on_error(argc < 2, "Missing filename in command line");
    read_input_file(argv[1]);
    return EXIT_SUCCESS;
}
