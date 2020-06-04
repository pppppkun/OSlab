#include "x86.h"
#include "device.h"
#include "fs.h"

#define SYS_WRITE 0
#define SYS_FORK 1
#define SYS_EXEC 2
#define SYS_SLEEP 3
#define SYS_EXIT 4
#define SYS_READ 5
#define SYS_SEM 6
#define SYS_GETPID 7
#define SYS_RAND 8
#define SYS_OPEN 9
#define SYS_LSEEK 10
#define SYS_CLOSE 11
#define SYS_REMOVE 12
#define SYS_LS 13
#define SYS_CAT 14

#define STD_OUT 0
#define STD_IN 1
#define SH_MEM 3

#define SEM_INIT 0
#define SEM_WAIT 1
#define SEM_POST 2
#define SEM_DESTROY 3

extern TSS tss;
extern ProcessTable pcb[MAX_PCB_NUM];
extern int current;

extern Semaphore sem[MAX_SEM_NUM];
extern Device dev[MAX_DEV_NUM];

extern int displayRow;
extern int displayCol;

extern uint32_t keyBuffer[MAX_KEYBUFFER_SIZE];
extern int bufferHead;
extern int bufferTail;

uint8_t shMem[MAX_SHMEM_SIZE];

void printHelp(char *src, int size);

void syscallHandle(struct TrapFrame *tf);
void syscallWrite(struct TrapFrame *tf);
void syscallRead(struct TrapFrame *tf);
void syscallFork(struct TrapFrame *tf);
void syscallExec(struct TrapFrame *tf);
void syscallSleep(struct TrapFrame *tf);
void syscallExit(struct TrapFrame *tf);
void syscallSem(struct TrapFrame *tf);
void syscallGetPid(struct TrapFrame *tf);
void syscallRand(struct TrapFrame *tf);
void syscallOpen(struct TrapFrame *tf);
void syscallLseek(struct TrapFrame *tf);
void syscallClose(struct TrapFrame *tf);
void syscallRemove(struct TrapFrame *tf);
void syscallLs(struct TrapFrame *tf);
void syscallCat(struct TrapFrame *tf);

void syscallWriteStdOut(struct TrapFrame *tf);
void syscallReadStdIn(struct TrapFrame *tf);
void syscallWriteShMem(struct TrapFrame *tf);
void syscallReadShMem(struct TrapFrame *tf);

void GProtectFaultHandle(struct TrapFrame *tf);

void timerHandle(struct TrapFrame *tf);
void keyboardHandle(struct TrapFrame *tf);

void syscallSemInit(struct TrapFrame *tf);
void syscallSemWait(struct TrapFrame *tf);
void syscallSemPost(struct TrapFrame *tf);
void syscallSemDestroy(struct TrapFrame *tf);

void printHelp(char *src, int size){
	int i;
	uint16_t data;
	int pos;
	for (i = 0; i < size; i++) {
		if (src[i] == '\n') {
			displayRow++;
			displayCol = 0;
			if (displayRow == 25){
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else {
			data = src[i] | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80){
				displayRow++;
				displayCol = 0;
				if (displayRow == 25){
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
	}
	updateCursor(displayRow, displayCol);
}

void irqHandle(struct TrapFrame *tf) { // pointer tf = esp
	/*
	 * 中断处理程序
	 */
	/* Reassign segment register */
	asm volatile("movw %%ax, %%ds"::"a"(KSEL(SEG_KDATA)));

	uint32_t tmpStackTop = pcb[current].stackTop;
	pcb[current].prevStackTop = pcb[current].stackTop;
	pcb[current].stackTop = (uint32_t)tf;

	switch(tf->irq) {
		case -1:
			break;
		case 0xd:
			GProtectFaultHandle(tf); // return
			break;
		case 0x20:
			timerHandle(tf);         // return or iret
			break;
		case 0x21:
			keyboardHandle(tf);      // return
			break;
		case 0x80:
			syscallHandle(tf);       // return
			break;
		default:assert(0);
	}

	pcb[current].stackTop = tmpStackTop;
}

void syscallHandle(struct TrapFrame *tf) {
	switch(tf->eax) { // syscall number
		case SYS_WRITE:
			syscallWrite(tf);
			break; // for SYS_WRITE
		case SYS_READ:
			syscallRead(tf);
			break; // for SYS_READ
		case SYS_FORK:
			syscallFork(tf);
			break; // for SYS_FORK
		case SYS_EXEC:
			syscallExec(tf);
			break; // for SYS_EXEC
		case SYS_SLEEP:
			syscallSleep(tf);
			break; // for SYS_SLEEP
		case SYS_EXIT:
			syscallExit(tf);
			break; // for SYS_EXIT
		case SYS_SEM:
			syscallSem(tf);
			break; // for SYS_SEM
		case SYS_GETPID:
			syscallGetPid(tf);
			break; // for SYS_GETPID
		case SYS_RAND:
			syscallRand(tf);
			break; // for SYS_RAND
		case SYS_OPEN:
			syscallOpen(tf);
			break;
		case SYS_LSEEK:
			syscallLseek(tf);
			break;
		case SYS_CLOSE:
			syscallClose(tf);
			break;
		case SYS_REMOVE:
			syscallRemove(tf);
			break;
		case SYS_LS:
			syscallLs(tf);
			break;
		case SYS_CAT:
			syscallCat(tf);
			break;
		default:break;
	}
}




void timerHandle(struct TrapFrame *tf) {
	uint32_t tmpStackTop;
	int i = (current + 1) % MAX_PCB_NUM;
	while (i != current) {
		if (pcb[i].state == STATE_BLOCKED) {
			pcb[i].sleepTime--;
			if (pcb[i].sleepTime == 0) {
				pcb[i].state = STATE_RUNNABLE;
			}
		}
		i = (i + 1) % MAX_PCB_NUM;
	}

	if (pcb[current].state == STATE_RUNNING &&
			pcb[current].timeCount != MAX_TIME_COUNT) {
		pcb[current].timeCount++;
		return;
	}
	else {
		if (pcb[current].state == STATE_RUNNING) {
			pcb[current].state = STATE_RUNNABLE;
			pcb[current].timeCount = 0;
		}
		i = (current + 1) % MAX_PCB_NUM;
		while (i != current) {
			if (i != 0 && pcb[i].state == STATE_RUNNABLE) {
				break;
			}
			i = (i + 1) % MAX_PCB_NUM;
		}
		if (pcb[i].state != STATE_RUNNABLE) {
			i = 0;
		}
		current = i;
		// putChar('0' + current);
		pcb[current].state = STATE_RUNNING;
		pcb[current].timeCount = 1;
		tmpStackTop = pcb[current].stackTop;
		pcb[current].stackTop = pcb[current].prevStackTop;
		tss.esp0 = (uint32_t)&(pcb[current].stackTop);
		asm volatile("movl %0, %%esp"::"m"(tmpStackTop)); // switch kernel stack
		asm volatile("popl %gs");
		asm volatile("popl %fs");
		asm volatile("popl %es");
		asm volatile("popl %ds");
		asm volatile("popal");
		asm volatile("addl $8, %esp");
		asm volatile("iret");
	}
	return;
}

void keyboardHandle(struct TrapFrame *tf) {
	// TODO in lab4
	keyBuffer[bufferTail] = getKeyCode();
	bufferTail = (bufferTail+1)%MAX_KEYBUFFER_SIZE;
	if(dev[STD_IN].value < 0)
	{
		dev[STD_IN].value = 0;
		ProcessTable * pt = (ProcessTable*)((uint32_t)(dev[STD_IN].pcb.prev) - (uint32_t)&(((ProcessTable*)0)->blocked));
		dev[STD_IN].pcb.prev = (dev[STD_IN].pcb.prev)->prev;
		(dev[STD_IN].pcb.prev)->next = &(dev[STD_IN].pcb);
		pt->state=STATE_RUNNABLE;
		pt->sleepTime=0;
	}

	return;
}

void syscallWrite(struct TrapFrame *tf) {
	switch(tf->ecx) { // file descriptor
		case STD_OUT:
			if (dev[STD_OUT].state == 1) {
				syscallWriteStdOut(tf);
			}
			break; // for STD_OUT
		case SH_MEM:
			if (dev[SH_MEM].state == 1) {
				syscallWriteShMem(tf);
			}
			break; // for SH_MEM
		default:break;
	}
}

void syscallWriteStdOut(struct TrapFrame *tf) {
	int sel = tf->ds; //TODO segment selector for user data, need further modification
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int i = 0;
	int pos = 0;
	char character = 0;
	uint16_t data = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	for (i = 0; i < size; i++) {
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		if (character == '\n') {
			displayRow++;
			displayCol = 0;
			if (displayRow == 25){
				displayRow = 24;
				displayCol = 0;
				scrollScreen();
			}
		}
		else {
			data = character | (0x0c << 8);
			pos = (80 * displayRow + displayCol) * 2;
			asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
			displayCol++;
			if (displayCol == 80){
				displayRow++;
				displayCol = 0;
				if (displayRow == 25){
					displayRow = 24;
					displayCol = 0;
					scrollScreen();
				}
			}
		}
	}
	
	updateCursor(displayRow, displayCol);
	//TODO take care of return value
}

//syscall(SYS_WRITE, fd, (uint32_t)buffer, size, index, 0);
//        eax, ecx, edx, ebx, esi, edi;
void syscallWriteShMem(struct TrapFrame *tf) {
	// TODO in lab4
	int sel = tf->ds;
//	int fd = tf->ecx;
	char *str = (char *)tf->edx;
	int size = tf->ebx;
	int index = tf->esi;
	char character = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	putString("writeShMem\n");
	int i = 0;
	int j = 0;
	while(i<size)
	{
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
		putChar(character);
		shMem[j+index] = character;
		i = i + 1;
		j = (j + 1) % MAX_SHMEM_SIZE;
	}
	pcb[current].regs.eax = i;
	return ;
}

void syscallRead(struct TrapFrame *tf) {
	switch(tf->ecx) {
		case STD_IN:
			if (dev[STD_IN].state == 1) {
				syscallReadStdIn(tf);
			}
			break;
		case SH_MEM:
			if (dev[SH_MEM].state == 1) {
				syscallReadShMem(tf);
			}
			break;
		default:
			break;
	}
}

/*
	scanf
	1. 调用scanf, 进入内核态，然后起一个scanf的线程
	2. 因为用户不是实时输入，所以把这个线程堵塞
	3. 当用户输入时，硬件中断，然后唤醒这个线程
	4. 唤醒线程后读取keyBuffer，然后输出到QEMU
*/

void syscallReadStdIn(struct TrapFrame *tf) {
	// TODO in lab4
	if(dev[STD_IN].value==0)
	{
		dev[STD_IN].value--;
		pcb[current].blocked.next = dev[STD_IN].pcb.next;
		pcb[current].blocked.prev = &(dev[STD_IN].pcb);
		dev[STD_IN].pcb.next = &(pcb[current].blocked);
		(pcb[current].blocked.next)->prev = &(pcb[current].blocked);

		pcb[current].state=STATE_BLOCKED;
		pcb[current].sleepTime = -1;

		asm volatile("int $0x20");
		int sel = tf->ds;
		char *str = (char*)tf->edx;
		int size = tf->ebx;
		char character = 0;
		asm volatile("movw %0, %%es"::"m"(sel));
		int i = 0;
		while(i<size)
		{
			if(bufferHead!=bufferTail)
			{
				character = getChar(keyBuffer[bufferHead]);

				bufferHead = (bufferHead+1) % MAX_KEYBUFFER_SIZE;
				if(character!=0)
				{
					asm volatile("movb %0, %%es:(%1)"::"r"(character),"r"(str + i));
					putChar(character);
					if (character == '\n') {
						displayRow++;
						displayCol = 0;
						if (displayRow == 25){
							displayRow = 24;
							displayCol = 0;
							scrollScreen();
						}
					}
					else {
						uint16_t data = character | (0x0c << 8);
						int pos = (80 * displayRow + displayCol) * 2;
						asm volatile("movw %0, (%1)"::"r"(data),"r"(pos + 0xb8000));
						displayCol++;
						if (displayCol == 80){
							displayRow++;
							displayCol = 0;
							if (displayRow == 25){
								displayRow = 24;
								displayCol = 0;
								scrollScreen();
								}
							}
						}
					updateCursor(displayRow, displayCol);
					i++;
				}
			}
			else break;
		}
		pcb[current].regs.eax=i;
		return;
	}
	else
	{
		pcb[current].regs.eax=-1;
		return;
	}
	return;
}

void syscallReadShMem(struct TrapFrame *tf) {
	// TODO in lab4
	int sel = tf->ds;
//	int fd = tf->ecx;
	uint8_t *str = (uint8_t *)tf->edx;
	int size = tf->ebx;
	int index = tf->esi;
	char character = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	putString("ReadShMem\n");
	int i = 0;
	int j = 0;
	while(i<size)
	{
		character = shMem[j+index];
		asm volatile("movb %0, %%es:(%1)"::"r"(character),"r"(str + i));
		i = i + 1;
		j = (j + 1) % MAX_SHMEM_SIZE;
	}
	pcb[current].regs.eax = i;
	return;
}

void syscallFork(struct TrapFrame *tf) {
	int i, j;
	for (i = 0; i < MAX_PCB_NUM; i++) {
		if (pcb[i].state == STATE_DEAD) {
			break;
		}
	}
	if (i != MAX_PCB_NUM) {
		pcb[i].state = STATE_PREPARING;
		enableInterrupt();
		for (j = 0; j < 0x100000; j++) {
			*(uint8_t *)(j + (i + 1) * 0x100000) = *(uint8_t *)(j + (current + 1) * 0x100000);
		}
		disableInterrupt();

		pcb[i].stackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].stackTop);
		pcb[i].prevStackTop = (uint32_t)&(pcb[i].stackTop) -
			((uint32_t)&(pcb[current].stackTop) - pcb[current].prevStackTop);
		pcb[i].state = STATE_RUNNABLE;
		pcb[i].timeCount = pcb[current].timeCount;
		pcb[i].sleepTime = pcb[current].sleepTime;
		pcb[i].pid = i;
		pcb[i].regs.ss = USEL(2 + i * 2);
		pcb[i].regs.esp = pcb[current].regs.esp;
		pcb[i].regs.eflags = pcb[current].regs.eflags;
		pcb[i].regs.cs = USEL(1 + i * 2);
		pcb[i].regs.eip = pcb[current].regs.eip;
		pcb[i].regs.eax = pcb[current].regs.eax;
		pcb[i].regs.ecx = pcb[current].regs.ecx;
		pcb[i].regs.edx = pcb[current].regs.edx;
		pcb[i].regs.ebx = pcb[current].regs.ebx;
		pcb[i].regs.xxx = pcb[current].regs.xxx;
		pcb[i].regs.ebp = pcb[current].regs.ebp;
		pcb[i].regs.esi = pcb[current].regs.esi;
		pcb[i].regs.edi = pcb[current].regs.edi;
		pcb[i].regs.ds = USEL(2 + i * 2);
		pcb[i].regs.es = pcb[current].regs.es;
		pcb[i].regs.fs = pcb[current].regs.fs;
		pcb[i].regs.gs = pcb[current].regs.gs;
		/*XXX set return value */
		pcb[i].regs.eax = 0;
		pcb[current].regs.eax = i;
	}
	else {
		pcb[current].regs.eax = -1;
	}
	return;
}

void syscallExec(struct TrapFrame *tf) {
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	char tmp[128];
	int i = 0;
	char character = 0;
	int ret = 0;
	uint32_t entry = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	while (character != 0) {
		tmp[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	}
	tmp[i] = 0;

	ret = loadElf(tmp, (current + 1) * 0x100000, &entry);
	if (ret == -1) {
		tf->eax = -1;
		return;
	}
	tf->eip = entry;
	return;
}

void syscallSleep(struct TrapFrame *tf) {
	if (tf->ecx == 0) {
		return;
	}
	else {
		pcb[current].state = STATE_BLOCKED;
		pcb[current].sleepTime = tf->ecx;
		asm volatile("int $0x20");
		return;
	}
	return;
}

void syscallExit(struct TrapFrame *tf) {
	pcb[current].state = STATE_DEAD;
	asm volatile("int $0x20");
	return;
}

void syscallSem(struct TrapFrame *tf) {
	switch(tf->ecx) {
		case SEM_INIT:
			syscallSemInit(tf);
			break;
		case SEM_WAIT:
			syscallSemWait(tf);
			break;
		case SEM_POST:
			syscallSemPost(tf);
			break;
		case SEM_DESTROY:
			syscallSemDestroy(tf);
			break;
		default:break;
	}
}
//syscall(SYS_SEM, SEM_INIT,  value, 0, 0, 0);
//        eax, ecx, edx, ebx, esi, edi;
void syscallSemInit(struct TrapFrame *tf) {
	// TODO in lab4
	uint32_t value = tf->edx;
	int i = 0;
	for(i = 0;i<MAX_SEM_NUM;i++)
	{
		if(sem[i].state==0) break;
	}

	if(i==MAX_SEM_NUM)
	{
		pcb[current].regs.eax=-1;
		return;
	}

	sem[i].state=1;
	sem[i].value=value;
	pcb[current].regs.eax=i;

	return;
}
// P
void syscallSemWait(struct TrapFrame *tf) {
	// TODO in lab4
	int i = tf->edx;
	if(sem[i].state==0)
	{
		pcb[current].regs.eax=-1;
		return;
	}
	sem[i].value--;
	if(sem[i].value<0)
	{
		pcb[current].blocked.next = sem[i].pcb.next;
		pcb[current].blocked.prev = &(sem[i].pcb);
		sem[i].pcb.next = &(pcb[current].blocked);
		(pcb[current].blocked.next)->prev = &(pcb[current].blocked);
		pcb[current].state=STATE_BLOCKED;
		pcb[current].sleepTime=-1;
		asm volatile("int $0x20");
	}
	pcb[current].regs.eax=0;
	return;
}
// V
void syscallSemPost(struct TrapFrame *tf) {
	// TODO in lab4
	int i = tf->edx;
	if(sem[i].state==0)
	{
		pcb[current].regs.eax=-1;
		return;
	}
	sem[i].value++;
	if(sem[i].value<=0)
	{
		ProcessTable* pt = (ProcessTable*)((uint32_t)(sem[i].pcb.prev) - 
			(uint32_t)&(((ProcessTable*)0)->blocked));
		sem[i].pcb.prev = (sem[i].pcb.prev)->prev;
		(sem[i].pcb.prev)->next = &(sem[i].pcb);
		pt->state=STATE_RUNNABLE;
		pt->sleepTime=0;
	}
	pcb[current].regs.eax=0;
	return;
}

void syscallSemDestroy(struct TrapFrame *tf) {
	// TODO in lab4
	int i = tf->edx;
	if(sem[i].state==0)
	{
		pcb[current].regs.eax=-1;
		return;
	}
	
	sem[i].state=0;
	pcb[current].regs.eax=0;
	asm volatile("int $0x20");
	
	return;
}

void syscallGetPid(struct TrapFrame *tf) {
	pcb[current].regs.eax = current;
	return;
}

void syscallRand(struct TrapFrame *tf)
{
	pcb[current].regs.eax = inByte(0x40);
	putInt(pcb[current].regs.eax);
	return;
}

void GProtectFaultHandle(struct TrapFrame *tf){
	assert(0);
	return;
}

void syscallOpen(struct TrapFrame *tf){
	//TODO
	return;
}

void syscallLseek(struct TrapFrame *tf){
	//TODO
	return;
}
void syscallClose(struct TrapFrame *tf){
	//TODO
	return;
}
void syscallRemove(struct TrapFrame *tf){
	//TODO
	return;
}

void syscallLs(struct TrapFrame *tf){
	//TODO
	int sel = tf->ds;
	char *str = (char *)tf->ecx;
	char tmp[128];
	int i = 0;
	char character = 0;
	asm volatile("movw %0, %%es"::"m"(sel));
	asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	while (character != 0) {
		tmp[i] = character;
		i++;
		asm volatile("movb %%es:(%1), %0":"=r"(character):"r"(str + i));
	}
	tmp[i] = 0;
	char dir[512];
	int size;
	ls(tmp, dir, &size);
	printHelp(dir, size);
}
void syscallCat(struct TrapFrame *tf){
	//TODO
	return;
}