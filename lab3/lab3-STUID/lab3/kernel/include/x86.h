#ifndef __X86_H__
#define __X86_H__

#include "x86/cpu.h"
#include "x86/memory.h"
#include "x86/io.h"
#include "x86/irq.h"

void initSeg(void);
void initProc(void);
int loadElf(const char *filename, uint32_t physAddr, uint32_t *entry);

#endif
