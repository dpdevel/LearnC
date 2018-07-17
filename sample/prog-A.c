/******************************************************************
Implementare un programma che riceva in input tramite argv[1] un numero
intero N maggiore o uguale ad 1 (espresso come una stringa di cifre 
decimali), e generi N nuovi thread. Ciascuno di questi, a turno, dovra'
inserire in una propria lista basata su memoria dinamica un record
strutturato come segue:

typedef struct _data{
	int val;
	struct _data* next;
} data; 

I record vengono generati e popolati dal main thread, il quale rimane
in attesa indefinita di valori interi da standard input. Ad ogni nuovo
valore letto avverra' la generazione di un nuovo record, che verra'
inserito da uno degli N thread nella sua lista. 
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
stampare a terminale il contenuto corrente di tutte le liste (ovvero 
i valori interi presenti nei record correntemente registrati nelle liste
di tutti gli N thread). 

*****************************************************************/
#ifdef Posix_compile
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#else
#endif

//posix version
//please compile with 'gcc prog-A-solution.c -lpthread -DPosix_compile'

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>


typedef struct _data{
	int val;
	struct _data* next;
} data;


pthread_mutex_t lock;
pthread_mutex_t next;

data *lists;
int num_threads;
int val;

void * worker(void* dummy){

	data *aux;
	int ret;

	printf("thread %ld alive\n", (long)dummy);

	while(1){
		aux = malloc(sizeof(data)); //pre-malloc for critical path speedup
		if (aux == NULL){
			printf("malloc error\n");
			exit(-1);
		}

		ret = pthread_mutex_lock(&lock);
		if (ret != 0){
			printf("locking error\n");
			exit(-1);
		}

		printf("thread %ld - found value is %d\n",(long)dummy,val);
		aux->val = val;
		ret = pthread_mutex_unlock(&next);
		if (ret != 0){
			printf("unlocking error\n");
			exit(-1);
		}

		aux->next = lists[(long)dummy].next;
		lists[(long)dummy].next = aux;
	}

	return 0;

}



void printer(int signo, siginfo_t *a, void* b){

	data aux;
	int i;

	for(i=0;i<num_threads;i++){
		aux = lists[i];
		printf("printing list %d\n",i);
 		while(aux.next){
			printf("%d ",aux.next->val);
			aux = *(aux.next);
		}
		printf("\n");
	}
}


int main(int argc, char** argv){

	long i;
	pthread_t tid;
	int ret;

	sigset_t set;
	struct sigaction act;

	
	if (argc < 2){
		printf("usage: prog num_threads\n");
		return -1;
	}

	num_threads = strtol(argv[1],NULL,10);
	printf("spawning %d threads\n",num_threads);

	lists = malloc(num_threads*sizeof(data));
	if (lists == NULL){
		printf("malloc error\n");
		exit(-1);
	}

	for(i = 0; i< num_threads; i++){
		lists[i].val = -1;
		lists[i].next = NULL;
	}

	ret = pthread_mutex_init(&lock,NULL);
	if (ret != 0){
		printf("lock init error\n");
		exit(-1);
	}

	ret = pthread_mutex_init(&next,NULL);
	if (ret != 0){
		printf("next init error\n");
		exit(-1);
	}

	ret = pthread_mutex_lock(&lock);
	if (ret != 0){
		printf("locking error\n");
		exit(-1);
	}


	sigfillset(&set);
	act.sa_sigaction = printer;
	act.sa_mask = set;
	act.sa_flags = 0;
	sigaction(SIGKILL, &act, NULL);
	
	for (i=0;i<num_threads;i++){
		ret = pthread_create(&tid,NULL,worker,(void*)i);
		if (ret != 0){
			printf("pthread create error\n");
			exit(-1);
		}

	}

	while(1){

step1:
		ret = pthread_mutex_lock(&next);
		if(ret !=0 && errno == EINTR) goto step1;
step2:
		ret = scanf("%d",&val);
		if(ret == EOF) goto step2;
		if(ret == 0){
			printf("non compliant input\n");
			exit(-1);
		}
step3:		
		ret = pthread_mutex_unlock(&lock);
		if(ret != 0 && errno == EINTR) goto step3;

	}
	return 0;
}
