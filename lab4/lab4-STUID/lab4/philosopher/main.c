#include "lib.h"
#include "types.h"
#define N 5
sem_t fork_[N];

void philosopher(int i)
{
	while(1)
	{
		printf("Philosopher %d: think\n", i);
		sleep(128);
		if(i%2==0)
		{
			sem_wait(&fork_[i]);
			sleep(128);
			sem_wait(&fork_[(i+1)%N]);
			sleep(128);
		}
		else
		{
			sem_wait(&fork_[(i+1)%N]);
			sleep(128);
			sem_wait(&fork_[i]);
			sleep(128);
		}
		printf("Philosopher %d: eat\n", i);
		sleep(128);
		sem_post(&fork_[i]);
		sleep(128);
		sem_post(&fork_[(i+1)%N]);
		sleep(128);
	}
}


int main(void) {
	// TODO in lab4
	printf("philosopher\n");
	
	for(int i = 0;i<N;i++)
	{
		sem_init(&fork_[i], 1);
	}
	int ret;
	ret = fork();
	if(ret == 0)
	{
		fork();
		fork();	
	}

	philosopher(getpid()-1);
	exit();
	return 0;
}
