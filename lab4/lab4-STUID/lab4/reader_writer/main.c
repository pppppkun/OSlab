#include "lib.h"
#include "types.h"
#define wc 4
#define rc 4

void writer(int id, sem_t *wm)
{
	sem_wait(wm);
	sleep(64);
	printf("Writer %d: write\n", id);
	sem_post(wm);
}
//write(SH_MEM, (uint8_t *)&data, 4, 0);
//read(SH_MEM, (uint8_t *)&data1, 4, 0);
void reader(int id, sem_t *wm, sem_t *cm, int Rc)
{
	sem_wait(cm);
	sleep(64);
	read(SH_MEM, (uint8_t*)&Rc, 4, 0);
	if(Rc == 0)
	{
		sem_wait(wm);
	}
	++(Rc);
	write(SH_MEM, (uint8_t *)&Rc, 4, 0);
	sleep(64);
	sem_post(cm);
	printf("Reader %d: read, total %d reader\n", id, Rc);
	sem_wait(cm);
	sleep(64);
	read(SH_MEM, (uint8_t*)&Rc, 4, 0);
	--(Rc);
	if(Rc == 0)
	{
		sem_post(wm);
	}
	write(SH_MEM, (uint8_t *)&Rc, 4, 0);
	sleep(64);
	sem_post(cm);
}

int main(void) {
	// TODO in lab4
	printf("reader_writer\n");
	
	sem_t WriteMutex;
	int Rcount = 0;
	sem_t CountMutex;
	sem_init(&WriteMutex, 1);
	sem_init(&CountMutex, 1);
	int ret = fork();
	if(ret==0)
	{
		fork();
		fork();
		while(1)
		{	
			reader(getpid(), &WriteMutex, &CountMutex, Rcount);
		}
	}
	else
	{
		fork();
		fork();
		while(1)
		{
			writer(getpid(), &WriteMutex);
		}
	}

	exit();
	return 0;
}
