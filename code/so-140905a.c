/*Scrivere una applicazione C/POSIX multithread che legge dallo standard input
linee con il seguente formato:
        numero decimale spazio stringa di testo

 Il numero decimale è considerato un ritardo in secondi. Per ciascuna linea con formato
corretto l’applicazione crea un thread che attende il numero di secondi indicato e poi scrive
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
la stampa ritardata delle stringhe (sono attività da svolgere contemporaneamente). Utilizzare
opportune primitive di sincronizzazione tra i thread per evitare race condition sullo standard
output.*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define MAX_LINE_LENGTH 128

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

struct thread_data {
    unsigned long delay;
    char message[MAX_LINE_LENGTH];
};


//Il thread appena creato inizia ad eseguire la funzione thread_job( ):
/*La prima operazione eseguita dal thread non è strettamente necessaria: il thread invoca
pthread detach() per fare in modo che, in uscita, tutte le risorse del thread vengano imme-
diatamente liberate (in caso contrario alcune risorse del thread rimarrebbero allocate in attesa
che qualche altro thread dell’applicazione esegua pthread join( ) sul thread terminato).
Poi il thread si auto-sospende per il numero di secondi richiesto. Si noti che l’uso della funzione
sleep( ) in una applicazione multithread è sicuro in Linux ma in generale poco consigliato
in altri sistemi POSIX.

Quando il thread si risveglia deve scrivere il messaggio in standard output. Per evitare che
più thread scrivano contemporaneamente sullo standard output viene utilizzato il mutex mtx,
allocato come variabile globale ed inizializzato staticamente.
Infine il thread termina dopo aver rilasciato la propria struttura di dati thread data.*/

void *thread_job(void *arg)
{
    struct thread_data *td = (struct thread_data *) arg;
    if (pthread_detach(pthread_self()) != 0) {
        fprintf(stderr, "Error in pthread_detach()\n");
        exit(EXIT_FAILURE);
    }
    sleep((unsigned int) td->delay); // safe on Linux
    if (pthread_mutex_lock(&mtx) != 0) {
        fprintf(stderr, "Error in pthread_mutex_lock()\n");
        exit(EXIT_FAILURE);
    }
    printf("> %s\n", td->message);
    if (pthread_mutex_unlock(&mtx) != 0) {
        fprintf(stderr, "Error in pthread_mutex_unlock()\n");
        exit(EXIT_FAILURE);
    }
    free(td);
    pthread_exit(0);
}

/*Per attivare l’allarme viene invocata la funzione activate alarm( ) passando come argomenti
delay e msg:*/

void activate_alarm(unsigned long delay, char *message)
{
    pthread_t tid;
    struct thread_data *p;

    p = malloc(sizeof(struct thread_data));
    if (!p) {
        fprintf(stderr, "Memory allocation error \n");
        exit(EXIT_FAILURE);
    }
    p->delay = delay;
    strncpy(p->message, message, MAX_LINE_LENGTH - 1);
    p->message[MAX_LINE_LENGTH - 1] = '\0';
    if (pthread_create(&tid, NULL, thread_job, p) != 0) {
        fprintf(stderr, "Error in pthread_create()\n");
        exit(EXIT_FAILURE);
    }
}


/*La funzione parse input line( ) legge la linea dallo standard input e la analizza. Il ritardo
in secondi viene memorizzato in delay, mentre msg punta entro buf al primo carattere della
stringa di testo. In caso di linea malformata parse input line( ) restituisce NULL.*/

char * parse_input_line(char *line, int maxsize, unsigned long *v)
{
    char *msg;

    if (fgets(line, maxsize, stdin) == NULL) {
        if (!feof(stdin))
            fprintf(stderr, "Error reading from standard input\n");
        return NULL;
    }

    errno = 0;
    *v = strtoul(line, &msg, 10);
    if (errno != 0) {
        fprintf(stderr, "Invalid number in standard input line\n");
        return NULL;
    }

    if (*msg++ != ' ') {
        fprintf(stderr, "Invalid separator in standard input line\n");
        return NULL;
    }

    // remove the trailing end line, if any
    if (msg[strlen(msg)-1] == '\n')
        msg[strlen(msg)-1] = '\0';

    return msg;
}

/*La funzione process_input_line() alloca spazio sullo stack per una linea di testo (vettore
buf). Assumiamo che la linea sia lunga al massimo MAX LINE LENGTH caratteri:*/

void process_input_line(void)
{
    char buf[MAX_LINE_LENGTH];
    char *msg;
    unsigned long delay;

    msg = parse_input_line(buf, MAX_LINE_LENGTH, &delay);
    if (msg != NULL)
        activate_alarm(delay, msg);
}


int main(int argc, char *argv[])
{
    if (argc != 1) {
        fprintf(stderr, "Usage: %s (no arg)\n", argv[0]);
        return EXIT_FAILURE;
    }
    while (!feof(stdin))
        process_input_line();
    pthread_exit(EXIT_SUCCESS);
}