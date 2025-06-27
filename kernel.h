#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>
#include "trap.h"
#define MAX_CPU 8
#define KSTACK_SIZE 4096

struct CPU {
	int cpu_id;
	struct trapframe* tf;
	char kstack[KSTACK_SIZE];	
};

extern struct CPU cpu_data[MAX_CPU];

void init_cpu();
void* get_kernel_stack_top(int cpu_id);
int get_cpu_id();

#endif
