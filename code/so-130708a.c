/* Scrivere un programma che continuamente legge dallo standard input righe di
testo terminate da caratteri di fine riga (\n). Ciascuna riga rappresenta un comando esterno
da eseguire, quindi contiene il nome del comando e gli eventuali argomenti:

 comando argomento1 argomento2 ...

Il programma deve eseguire il comando (lanciato con gli eventuali argomenti indicati) in modo
da salvare lo standard output del comando stesso su di un file chiamato “out.n ”, ove n è il
numero progressivo del comando eseguito.
Ad esempio, se il programa inizialmente legge dallo standard input le seguenti tre righe:

 ls -l -S /
 date
 cat /etc/passwd

allora il file “out.1” conterrà l’elenco dei file della directory radice ordinati per dimensione,
il file “out.2” conterrà la data e l’ora al momento dell’esecuzione, ed il file “out.3” sarà una
copia del file “/etc/passwd”.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[])
{
    int n;
    if (argc != 1) {
        fprintf(stderr, "Usage: %s (no arguments)\n", argv[0]);
        return EXIT_FAILURE;
    }
    n = 0;
    while (!feof(stdin))
        read_and_process_line(++n);
    return EXIT_SUCCESS;
}

/* La funzione read and process line( ) riceve come argomento il numero pro-
gressivo della riga da leggere. Essa deve effettivamente leggere la linea, analiz-
zare il suo contenuto, ed eseguire il comando corrispondente. */

#define max_line_lenght 256
void read_and_process_line(int n)
{
    char line[max_line_lenght];
    if (read_line(line))
        process_line(line, n);
}

/*È stata prefissata una dimensione massima per la linea pari al valore della
macro max line length. Il buffer line è destinato a contenere i dati letti dallo
standard input. La funzione read line( ) si occupa di leggere una linea dallo
standard input; essa restituisce il valore 0 in caso di EOF:*/

int read_line(char line[])
{
    if (fgets(line, max_line_lenght, stdin) == NULL) {
        if (!feof(stdin)) {
            perror("stdin");
            exit(EXIT_FAILURE);
        }
        return 0;
    }
    // remove the final '\n' character
    if (line[strlen(line) - 1] == '\n')
        line[strlen(line) - 1] = '\0';
    return 1;
}


/*Si noti che la funzione di libreria fgets( ) può memorizzare anche il carattere
di fine riga, che però va eliminato per evitare che sia considerato parte del nome
del comando da eseguire o di un argomento.
La funzione process line( ) riceve come argomenti il buffer contenente la linea
letta dallo standard input ed il numero progressivo del comando:*/

void process_line(char line[], int n)
{
    char *argv[max_line_lenght / 2];
    parse_line(line, argv);
    execute_command(argv[0], argv, n);
}

/*La funzione deve svolgere due compiti: (1) analizzare la linea letta dallo stan-
dard input per costruire un vettore di puntatori a stringa, ciascuno corrispon-
dente all’inizio del comando o di uno degli argomenti, e (2) eseguire il comando
stesso. Si noti che il vettore dei puntatori a stringa è allocato staticamente sulla
base della dimensione massima della linea (nel caso peggiore ogni comando o
argomento è costituito da un singolo carattere, ed è seguito/preceduto da un
carattere separatore).

La funzione parse line( ) riceve come argomenti il buffer con la linea e l’indi-
rizzo del vettore di puntatori a stringa: */

void parse_line(char *line, char *argv[])
{
    int len = strlen(line);
    int i = 0, k = 0;
    while (i < len) {
        i = skip_blanks(line, i);
        if (i >= len)
            break;
        argv[k++] = line + i;
        i = skip_non_blanks(line, i + 1);
    }
    argv[k] = NULL;
}

/* Le funzioni skip blanks( ) e skip non blanks( ) determinano la posizione del
successivo carattere separatore e non separatore, rispettivamente: */

int skip_blanks(char *linem int i)
{
    while (i < max_line_lenght && line[i] == ' ' || line[i] != '\t')
        ++i;
    return i;
}

/*Si noti come la funzione skip blanks( ) sostituisca tutti i caratteri separatore
con terminatori di stringa, cosı̀ che alla fine ciascun elemento nel vettore argv
punta ad una stringa “indipendente”.

Per eseguire il comando esterno si invoca la funzione execute command( ), che
riceve una stringa con il nome del comando, il vettore di puntatori di stringa
corrispondente agli argomenti, ed il numero progressivo del comando:*/

void execute_command(char *cmd, char *argv[], int n)
{
    pid_t pid;

    if (!cmd) //nothing to do (blank line)
        return;
    if ((pid = fork()) == -1) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    if (pid) //the parent returns
        return;
    redirect_output(n);
    execvp(cmd, argv);
    perror(cmd);
    exit(EXIT_FAILURE);
}

/*La funzione crea un processo: il padre ritorna subito, mentre il figlio prima
redirige lo standard output e poi esegue la funzione di libreria execvp( ) per
eseguire il comando.

La funzione redirect output( ) apre un file in scrittura con un nome dipen-
dente dal numero progressivo del comando e ridirige lo standard output su tale
file:*/

void redirect_output(int n)
{
    char filename[32];
    int fd;
    sprintf(filename, "out.%d", n);
    fd = open(filename, O_WRONLY | O_CREAT, 0644);
    if (fd == 1) {
        perror(filename);
        exit(EXIT_FAILURE);
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }
    if (close(fd) == -1)
        perror(filename);
}