#include "lib.h"
#include "types.h"
#define N 4
int buffer[N];
int setPoint = 0;
int getPoint = 0;
void Deposit(int i, sem_t *eb, sem_t *fb, sem_t *mutex)
{
	sem_wait(eb);
	sem_wait(mutex);
	buffer[setPoint] = 2;
	setPoint = (setPoint + 1) % N;
	sem_post(mutex);
	sem_post(fb);
}

void Remove(int i, sem_t *eb, sem_t *fb, sem_t *mutex)
{
	sem_wait(fb);
	sem_wait(mutex);

}

int main(void) {
	// TODO in lab4

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
		
	}
	else
	{
		
	}
	


	printf("bounded_buffer\n");
	exit();
	return 0;
}
