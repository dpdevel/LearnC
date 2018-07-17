/******************************************************************
Implementare un programma che riceva in input tramite argv[2] un numero
intero N maggiore o uguale ad 1 (espresso come una stringa di cifre 
decimali), e generi N nuovi processi. Ciascuno di questi leggera' in modo 
continuativo un valore intero da standard input, e lo comunichera' al
processo padre tramite memoria condivisa. Il processo padre scrivera' ogni
nuovo valore intero ricevuto su di un file, come sequenza di cifre decimali. 
I valori scritti su file devono essere separati dal carattere ' ' (blank).
Il pathname del file di output deve essere comunicato all'applicazione 
tramite agv[1].
Nel caso in cui non vi sia immissione in input, l'applicazione non deve 
consumare piu' del 5% della capacita' di lavoro della CPU.

*****************************************************************/
#ifdef Posix_compile
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <fcntl.h>
#else
#endif

//posix version
//please compile with 'gcc prog-B-solution.c -DPosix_compile'

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <asm/errno.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <fcntl.h>


#define PAGE_SIZE 4096

int num_proc;
int *values;
int sem;

void run(){

	int val;
	struct sembuf oper;
	int ret;

	signal(SIGINT,SIG_IGN);

	while(1){
		ret = scanf("%d",&val);
		if(ret == 0){
			printf("scanf error\n");
			exit(-1);
		}

		printf("proc %d - read value %d\n",getpid(),val);

		oper.sem_num = 1;
		oper.sem_op = -1;
		oper.sem_flg = SEM_UNDO;
		ret = semop(sem,&oper,1);
		if(ret == -1){
			printf("semop error\n");
			exit(-1);
		}
		

		printf("proc %d - writing value %d\n",getpid(),val);

		*values = val;

		oper.sem_num = 0;
		oper.sem_op = 1;
		oper.sem_flg = SEM_UNDO;
		ret = semop(sem,&oper,1);
		if(ret == -1){
			printf("semop error\n");
			exit(-1);
		}

	}

}

char command[1024];

void printer(int signo){

	system(command);

	return;
}



int main(int argc, char** argv){

	int i;
	int key = 1234;
	struct sembuf oper;
	int fd;
	FILE *file;
	int ret;

	if(argc < 3){
		printf("usage: prog output_file num_proc\n");
		return -1;
	}

	num_proc = strtol(argv[2],NULL,10);

	values = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, 0, 0);

	if (values == NULL){
		printf("mmap error\n");
		exit(-1);
	}

	sem = semget(key,2,IPC_CREAT|0666);
	semctl(sem,1,IPC_RMID,NULL);
	sem = semget(key,2,IPC_CREAT|0666);

	if(sem == -1){
		printf("semget error\n");
		return -1;
	}

	ret = semctl(sem,0,SETVAL,0);
	if(ret == -1){
		printf("semctl error\n");
		exit(-1);
	}
	ret = semctl(sem,1,SETVAL,1);
	if(sem == -1){
		printf("semctl error\n");
		exit(-1);
	}


	printf("spawning %d processes\n",num_proc);


	signal(SIGINT,printer);

	for(i=0; i< num_proc; i++){
		if(fork()) continue;//assuming fork does not fail
		else run();
	}	

	sprintf(command,"cat %s",argv[1]);

	fd = open(argv[1],O_CREAT|O_RDWR,0666);
	file = fdopen(fd,"r+");

	printf("going for read phase\n");


	while(1){
		oper.sem_num = 0;
		oper.sem_op = -1;
		oper.sem_flg = SEM_UNDO;
wait1:
		ret = semop(sem,&oper,1);
		if(ret ==-1 && errno == EINTR) goto wait1;

		printf("found value is %d\n",*values);
rewrite:
		ret = fprintf(file,"%d ",*values);
		if(ret == -1 && errno == EINTR) goto rewrite;
		fflush(file);

		oper.sem_num = 1;
		oper.sem_op = 1;
		oper.sem_flg = SEM_UNDO;
wait2:
		ret = semop(sem,&oper,1);
		if(ret == -1 && errno == EINTR) goto wait2;

	}

	pause();

	return 0;
}
