#include "lib.h"
#include "types.h"
#define N 6
int buffer[N];
int setPoint = 0;
int getPoint = 0;
void Deposit(int id, sem_t *eb, sem_t *fb, sem_t *mutex)
{
	sem_wait(eb);
	sleep(128);
	sem_wait(mutex);
	sleep(128);
	buffer[setPoint] = 1;
	setPoint = (setPoint + 1) % N;
	printf("Producer %d: produce\n", id);
	sleep(128);
	sem_post(mutex);
	sleep(128);
	sem_post(fb);
}

void Remove(sem_t *eb, sem_t *fb, sem_t *mutex)
{
	sem_wait(fb);
	sleep(128);
	sem_wait(mutex);
	sleep(128);
	buffer[getPoint] = 0;
	getPoint = (getPoint + 1) % N;
	printf("Consumer : consume\n");
	sleep(128);
	sem_post(mutex);
	sleep(128);
	sem_post(eb);
}

int main(void) {
	// TODO in lab4
	printf("bounded_buffer\n");
	sem_t mutex;
	sem_t fullBuffers;
	sem_t emptyBuffers;
	int ret = 0;
	ret = sem_init(&mutex, 1);
	if(ret==-1)
	{
		printf("init mutex error\n");
		return 0;
	}
	ret = sem_init(&fullBuffers, 0);
	if(ret==-1)
	{
		printf("init fullBuffers error\n");
		return 0;
	}
	ret = sem_init(&emptyBuffers, N);

	ret = fork();
	if(ret==0)
	{
		while(1)
		{
			Remove(&emptyBuffers, &fullBuffers, &mutex);
		}
	}
	else
	{
		fork();
		fork();
		while(1)
		{
			Deposit(getpid(), &emptyBuffers, &fullBuffers, &mutex);
		}
	}
	exit();
	return 0;
}
