/* Esercizio 2. Scrivere un programma C/POSIX che continuamente legge righe di testo dallo
standard input e le scrive in un’area di memoria condivisa IPC. L’area di memoria IPC è già
esistente ed è identificabile tramite la chiave ottenibile con il percorso “/tmp” e il carattere
identificativo “!”. Si assuma che ciascuna riga di testo abbia una lunghezza massima di 256
caratteri. Dopo ciascuna scrittura sull’area di memoria condivisa il programma attende di
ricevere un segnale SIGUSR1 prima di leggere la successiva riga di testo dallo standard input.*/


/* 1) cattura del segnale USR1
 * 2) apertura della regione di memoria
 * 3) copia delle righe di testo dallo standard input alla regione di memoria*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>


/*Per aprire la regione di memoria condivisa IPC utilizziamo la funzione open_shared_memory():*/

char * open_shared_memory(void)
{
    int desc;
    key_t shm_id;
    void *addr;

    /* obtain the IPC ke*/
    shm_id = ftok(".", "!");
    if (shm_id == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }

    /* open the shared memory area*/
    desc = shmget(shm_id, 256, IPC_CREAT | 0666);
    if (desc == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    /* attach to the process address space */
    addr = shmat(desc, NULL, 0);
    if (addr == (void *) -1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    return  addr;
}

/* Infine la funzione copy_messages() che con un ciclo legge le righe di testo dallo std input
 * per mezzo di fgets() e le copia sulla regione di memoria condivisa:*/

void  copy_messages(char *shm)
{
    char buf[256];

    for(;;) {
        if (fgets(buf, 256, stdin) == NULL)
            break;
        strncpy(shm, buf, 256);
        pause();  /* waite for an cached signal*/
    }
    if (!feof(stdin)) {
        perror("standard input");
        exit(EXIT_FAILURE);
    }
}



/* Per catturare il segnale definiamo il gestore, sig_handler(), che in effetti non ha ncessità
 * di svolgere alcuna operazione. La modifica della gestione del segnale SIGUSR1 è effettuata dalla
 * funzione catch_signal().*/

void sig_handler(int sig)
{
    sig=sig; /* do nothing*/
}


void catch_signal(void)
{
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    if (sigemptyset(&sa.sa_mask) == -1) {
        fprintf(stderr, "Error handling the signal set\n");
        exit(EXIT_FAILURE);
    }
    sa.sa_flags = 0;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    return;
}


/* Si noti che la API pause() blocca il processo fino alla ricezione di un quelunque segnale
 * catturato o che provoca la terminazione del processo stesso. Poichè è stato catturato
 * esclusivamene il segnale SIGUSR1 il programma per semplicità non controlla il tipo di segnale
 * recevuto. Tale controllo è facilmente inseribile, ad esempio definendo una variabile globale
 * incrementata esclusivamente nel gestore del segnal sig_handler().*/

int main(int argc, char *argv[])
{
    char *shm_area;

    if (argc != 1) {
        fprintf(stderr, "Usage: %s (no arguments)\n", argv[0]);
        return EXIT_FAILURE;
    }

    catch_signal();
    shm_area = open_shared_memory();
    copy_messages(shm_area);
    return EXIT_SUCCESS;    /*never reached*/
}






