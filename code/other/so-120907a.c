/*Scrivere una applicazione C/POSIX che legge dallo standard input linee con il seguente formato:

                    numero decimale spazio stringa di testo

Il numero decimale è considerato un ritardo in secondi. Per ciascuna linea con formato
corretto l’applicazione crea un processo che attende il numero di secondi indicato e poi scrive
in standard output la stringa di testo.

 Ad esempio, passando sullo standard input le tre linee seguenti:
10 ritardo massimo
1 piccolo ritardo
5 ritardo medio

 verranno scritti sullo standard output:
> piccolo ritardo   (dopo circa 1 secondo)
> ritardo medio     (dopo circa 5 secondi)
> ritardo massimo   (dopo circa 10 secondi)

Si noti che l’applicazione deve continuare a leggere linee dallo standard input anche durante
la stampa ritardata delle stringhe (sono attività da svolgere contemporaneamente).

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    if (argc != 1) {
        fprintf(stderr, "Usage: %s (no arguments\n", argv[0]);
        return  EXIT_FAILURE;
    }

    while (!feof(stdin))
        process_input_line();

    return EXIT_SUCCESS;
}

//la funzione process_input_line() alloca spazione sull stack per una linea di test( vettore buf)
//Assumiamo che la linea sia lunga al massimo 128 caratteri.

void process_input_line(void)
{
    char buf[128];
    char *msg;
    unsigned long delay;

    msg = parse_input_line(buf, 128, &delay);
    if (msg != NULL)
        activate_alarm(delay, msg);
}

//La funzione parse_input_line() legge la linea dallo stdr input e la analizza. Il ritardo in secondi
//viene memorizzato in delay, mentro msg punta entro buf al primo caratter della stringa di testo.
//In caso di linea mailformata parse_input_line() resttuisce NULL.

char * parse_input_line(char *line, int maxsize, unsigned long *v)
{
    char *msg;

    /* read a line from standard input */
    if (fgets(line, maxsize, stdin) == NULL) {
        if (!feof(stdin))
            fprintf(stderr, "Error reading from standard input\n");
        return NULL;
    }

    /* parse the line*/
    errno = 0;
    *v = strtoul(lin, &msg, 10);
    if (errno != 0) {
        fprintf(stderr, "Invalid number in standard input line\n");
        return NULL;
    }

    if(*msg++ != ' ') {
        fprintf(stderr, "invalid separator in standard input line\n");
        return NULL;
    }

    /* remove the trailing neww line, if any*/
    if (msg[strlen(msg)-1] == '\n')
        msg[strlen(msg)-1] = '\0';
    return msg;
}

// per attivare l'allarme viene invocata la funzione activate_alarm() passando come
// argomenti delay e msg:

void activate_alarm(unsigned long delay, char *message)
{
    pid_t p = fork();
    if (p == -1) {
        fprintf(stderr, "Error in fork()\n");
        exit(EXIT_FAILURE);
    }
    if (p != 0)
        return; //the parent continues to ait for input
    sleep(delay);  //dont care about signals
    printf("> %s", message);
    exit(EXIT_SUCCESS);
}

//Il processo principale crea un figlio e termina la funzione (tornando quindi
//a leggere dallo standard input). Il figlio invoca sleep() per auto-sospendersi per
//il numero richiesto di secondi, poi stampa il messaggio e termina.
