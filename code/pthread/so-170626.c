/*
Un file cifrato contiene una sequenza di byte c(i) (0 ≤ i < L) che sono il risultato di una
operazione di XOR (operatore C “ˆ”) tra i caratteri di un testo sconosciuto p(i) (0 ≤ i < L)
ed una password segreta s di lunghezza al più 32 byte. La password segreta in generale è più
corta del contenuto del file, quindi viene ripetuta ciclicamente:

 c i = p(i) XOR s(j) , 0 ≤ i < L, j = i mod strlen(s)

Si assuma che la password segreta sia costituita esclusivamente da caratteri alfabetici minuscoli.

Scrivere una applicazione C/POSIX multithread costituita da 128 thread che cerca esaustiva-
mente la password di un file cifrato il cui nome è specificato sulla linea comando. L’applicazione
deve cercare tra tutti i possibili valori della password segreta quelli che decodificano il file ci-
frato in modo tale che la percentuale di caratteri alfabetici (maiuscoli e minuscoli) tra quelli
decodificati sia maggiore del 90%. I valori ammissibili della password devono essere scritti
sullo standard output.

Il lavoro di ricerca deve essere suddiviso tra i 128 thread, che debbono lavorare in parallelo.
Si presti attenzione ai possibili problemi di sincronizzazione.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#define MAX_PASS_LEN 4
#define Nthreads 10

char last_pw[MAX_PASS_LEN+1];
size_t length;
char *map;
int finished;
pthread_mutex_t mtx;

#define abort_on_error(cond, msg) do { \
    if (cond) { \
        fprintf (stderr, "%s (errno=%d [%m])\n", msg, errno); \
        exit(EXIT_FAILURE); \
    } \
} while(0)


char * open_and_map_file(const char *name, size_t *plenght)
{
    char *map;
    off_t filelen;
    int fd = open(name, O_RDONLY);
    abort_on_error(fd == -1, "Cannot open input file");

    filelen = lseek(fd, 0, SEEK_END);
    abort_on_error(filelen == (off_t) -1, "Cannot seek the end file");

    map = mmap(NULL, (size_t) filelen, PROT_READ, MAP_PRIVATE, fd, 0);
    abort_on_error(map == MAP_FAILED, "Cannote map input file");

    *plenght = (size_t) filelen;

    return map;
}


void init_data_structures(void)
{
    int i, rc;
    for (i=0; i<MAX_PASS_LEN+1; ++i)
        //memset()
        last_pw[i]='\0';
    rc = pthread_mutex_init(&mtx, NULL);
    abort_on_error((errno = rc), "Can't initialize the mutex");
    finished = 0;
}

void get_lock(void)
{
    int rc = pthread_mutex_lock(&mtx) ;
    abort_on_error((errno = rc) , "Can’t get mutex\n" );
}

void release_lock(void)
{
    int rc = pthread_mutex_unlock(&mtx) ;
    abort_on_error((errno = rc) , "Can’t release mutex\n") ;
}


int get_next_password(void)
{
    int l, pl;
    if (finished == 1)
        return  0;

    l = pl = (int) strlen(last_pw);
    while (l > 0 && last_pw[l-1] == 'z') {
        last_pw[l-1] = 'a';
        --l;
    }

    if (l > 0) {
        last_pw[l-1]++;
        return 1;
    }

    if (pl == MAX_PASS_LEN) {
        finished = 1;
        return 0;
    }

    last_pw[pl] = 'a';
    return 1;
}


void try_password(char *pw)
{
    char *p = map;
    size_t f, nalp = 0;
    int j=0, pwlen = (int) strlen(pw);
    for (p=map, f=0; f<length; ++p, ++f) {
        char c = *p ^ pw[j];  //XOR
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            ++nalp;
        if (++j == pwlen)
            j = 0;
    }
    if (nalp*10 > length*9)
        printf("%s\n", pw);
}

void *search_passwords(void *args)
{
    char my_pw[MAX_PASS_LEN+1];
    //arg = arg;  /* avoid gcc's arning */
    for (;;) {
        get_lock();
        if (!get_next_password()) {
            release_lock();
            pthread_exit(NULL);
        }
        strncpy(my_pw, last_pw, MAX_PASS_LEN+1);
        release_lock();
        printf("%s\n", my_pw);
        try_password(my_pw);
    }
}


void spawn_threads(void)
{
    int i, rc;
    for (i=0; i<Nthreads; ++i) {
        pthread_t tid;
        rc = pthread_create(&tid, NULL, search_passwords, NULL);
        abort_on_error((errno=rc), "Can't spawn a thread");
    }
}


int main(int argc, char *argv[])
{
    abort_on_error(argc != 2,"Wrong number of command line arguments");
    map = open_and_map_file(argv[1], &length);
    init_data_structures();
    spawn_threads();
    pthread_exit(NULL);
}




