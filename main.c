#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdatomic.h>

#define DEFSZ 0x10000

void funny(int);
static void *glob_var;
static uint8_t *flag;

int main(void)
{
    int i = 0;
    int pid = 0;
    pthread_cond_t *cond = mmap(NULL,sizeof(pthread_cond_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS, -1, 0); //condition variable synchronizing signal handling
    sem_t *sem = mmap(NULL, sizeof(sem_t), PROT_READ |PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    pthread_mutex_t *mut = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    glob_var = mmap(NULL, DEFSZ, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    flag = mmap(NULL,sizeof(uint8_t),PROT_READ|PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    pthread_cond_init(cond, NULL);
    sem_init(sem, 1, 1);
    memset(glob_var, '0', DEFSZ);
    memset(flag,0,sizeof(uint8_t));

    pid = fork();
    if (pid == 0) {          /* child */
    	pthread_mutex_lock(mut);
    	signal(SIGUSR1,funny); //signal handler init
    	*flag = 1;
    	pthread_cond_signal(cond);
    	pthread_mutex_unlock(mut); // release mutex, raise flag and signal the father process that the initialization is over

        while (1) {
            sem_wait(sem);
            sprintf((char *) glob_var, "%i", i);
            sem_post(sem);
            i=i+5;
        }
        exit(EXIT_SUCCESS);
    }

    while (1) {                 /* parent */
        sscanf(glob_var, "%i", &i);
        printf("father : %i\n", i);
        pthread_mutex_lock(mut);
        while(*flag == 0){
        	fflush(stdout);
        	pthread_cond_wait(cond, mut); //wait for signal handling in child process
        }
        pthread_mutex_unlock(mut);
        kill(pid,SIGUSR1); //send signal to other process
    }

    sem_destroy(sem); // erase all memory not needed anymore
    munmap(sem, sizeof(sem_t));
    munmap(glob_var, DEFSZ);
    munmap(cond,sizeof(pthread_cond_t));
    munmap(flag,sizeof(uint8_t));
    return 0;
}

void funny(int signo)
{
	printf("In signal handler\n");
	fflush(stdout);
}
