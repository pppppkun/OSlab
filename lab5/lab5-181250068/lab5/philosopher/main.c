#include "lib.h"
#include "types.h"
#define N 5
sem_t fork_[N];

void philosopher(int i)
{
	while(1)
	{
		printf("Philosopher %d: think\n", i);
		sleep(rand());
		if(i%2==0)
		{
			sem_wait(&fork_[i]);
			sleep(rand());
			sem_wait(&fork_[(i+1)%N]);
			sleep(rand());
		}
		else
		{
			sem_wait(&fork_[(i+1)%N]);
			sleep(rand());
			sem_wait(&fork_[i]);
			sleep(rand());
		}
		printf("Philosopher %d: eat\n", i);
		sleep(rand());
		sem_post(&fork_[i]);
		sleep(rand());
		sem_post(&fork_[(i+1)%N]);
		sleep(rand());
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
