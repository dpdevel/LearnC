/*
Scrivere una applicazione multiprocesso C/POSIX il cui scopo è determinare le approssima-
zioni del numero π basate sulla seguente formula (algoritmo di Gauss-Legendre):

L’applicazione è costituita da tre processi P 1 , P 2 e P 3 che comunicano tramite code di mes-
saggi. Il processo P 1 calcola i valori successivi a i e b i , e li invia al processo P 2 . Il processo P 2
calcola i valori successivi di t i e p i , ed invia tutti i valori necessari a P 3 . Il processo P 3 calcola
le approssimazioni successive di π e le scrive in standard output.
Tutte le variabili utilizzate debbono essere in virgola mobile con precisione doppia. Si trascuri-
no i problemi legati alla precisione delle variabili, ma si presti attenzione alla sincronizzazione
tra i processi. Per calcolare la radice quadrata si utilizzi la funzione sqrt( ).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#define abort_on_error(cond, msg) do { \
    if (cond) { \
        fprintf(stderr, "%s (%d)\n", msg, errno); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

int pipe12[2];
int pipe23[2];


/*La scrittura è realizzata dalla generica funzione send value( ), che si pren-
de cura di scrivere tutti i byte del numero in virgola mobile gestendo anche
eventuali terminazioni premature della chiamata di sistema write( ):*/

     //send_value(pipe12[1], new_a)
void send_value(int fd, double v)
{
    int rc, size = sizeof(v);
    unsigned char *p = (unsigned char *) &v;
    while (size > 0) {
        rc = (int) write(fd, p, (size_t) size);
        abort_on_error(rc == -1, "Error writing to pipe");
        size -= rc;
        p += rc;
    }
}

//La funzione recv value( ) è naturalmente l’analoga di send value( ), e serve
//a leggere un valore di tipo double da una pipe:

     //recv_value(pipe12[0], &b);
void recv_value(int fd, double *v)
{
    int rc, size = sizeof(*v);
    unsigned char *p = (unsigned char *) v;
    while (size > 0) {
        rc = (int) read(fd, p, (size_t) size);
        abort_on_error(rc == -1, "Error reading pipe");
        size -= rc;
        p += rc;
    }
}

/*Le funzioni do P1 work( ), do P2 work( ) e do P3 work( ) implementano gli al-
goritmi specifici di ciascun processo. Ciascuna di queste funzioni esegue un ci-
clo senza fine calcolando sempre nuove approssimazioni delle variabili utilizzate
dall’algoritmo di Gauss-Legendre.*/

//La funzione do P1 work() esegue il calcolo delle variabili di tipo ai e bi :
/*Inizialmente la funzione chiude i descrittori delle pipe non utilizzati dal pro-
cesso P 1 . In ogni iterazione del ciclo senza fine vengono calcolati i nuovi valori
delle variabili a i e b i , che vengono poi scritti sulla pipe pipe12.*/

void do_P1_work(void)
{
    double a = 1.0, b = 1.0/sqrt(2);
    int rc = close(pipe23[0]);
    abort_on_error(rc != 0, "Error closing pipe23 r-fd");
    rc = close(pipe23[1]);
    abort_on_error(rc != 0, "Error closing pipe23 w-fd");
    rc = close(pipe12[0]);
    abort_on_error(rc != 0, "Error closing pipe12 r-fd");
    for (;;) {
        double new_a = ( a+b )/2.0;
        double new_b = sqrt( a*b );
        send_value(pipe12[1], new_a);
        send_value(pipe12[1], new_b);
        a = new_a;
        b = new_b;
    }
}


//La funzione do P2 work( ) implementa in modo analogo l’algoritmo seguito dal
//processo P 2 :

void do_P2_work(void)
{
    double prev_a = 1.0;
    double t = 0.25;
    double p = 1.0;
    double a, b;
    int rc = close(pipe23[0]);
    abort_on_error(rc != 0, "Error closing pipe23 r-fd");
    rc = close(pipe12[1]);
    abort_on_error(rc != 0, "Error closing pip12 w-fd");
    for (;;) {
        double x;
        recv_value(pipe12[0], &a);
        recv_value(pipe12[0], &b);
        x = prev_a - a;
        t -= p * (x * x);
        p *= 2.0;
        send_value(pipe23[1], a);
        send_value(pipe23[1], b);
        send_value(pipe23[1], t);
        prev_a = a;
    }
}


//Infine la funzione do P3 work() implementa l’algoritmo del processo P 3 :

void do_P3_work(void)
{
    double a, b, t;
    int rc = close(pipe12[0]);
    abort_on_error(rc != 0, "Error closing pipe12 r-fd");
    rc = close(pipe12[1]);
    abort_on_error(rc != 0, "Error closing pipe12 w-fd");
    rc = close(pipe23[1]);
    abort_on_error(rc != 0, "Error closing pipe23 w-fd");
    for (;;) {
        double x, pi;
        recv_value(pipe23[0], &a);
        recv_value(pipe23[0], &b);
        recv_value(pipe23[0], &t);
        x = a + b;
        pi = ( x * x ) / ( 4.0 * t );
        printf("%.30f\n", pi);
    }
}


void spawn_processes(void)
{
    pid_t p = fork();
    abort_on_error(p == -1, "Error in 1st fork()");
    if (p == 0)
        do_P1_work();
    p = fork();
    abort_on_error( p == -1, "Error in 2nd fork()");
    if (p == 0)
        do_P2_work();
    do_P3_work();
}

// La funzione init comm channels( ) crea le due pipe, salvando i quattro de-
// scrittori in due vettori globali.

void init_comm_channels(void)
{
    int rc = pipe(pipe12);
    abort_on_error(rc == -1, "Error in 1st pipe()");
    rc = pipe(pipe23);
    abort_on_error(rc == -1, "Error in 2nd pipe()");
}

int main() {
    init_comm_channels();
    spawn_processes();
}

