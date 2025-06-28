#ifndef PROC_H
#define PROC_H

#include <stdint.h>
#include "trap.h"

enum proc_state {
	UNUSED,
	RUNNABLE,
	RUNNING,
	SLEEPING,
	ZOMBIE	
};

struct proc {
	int pid;
	uintptr_t pagetable;
	struct trapframe* tf;
	enum proc_state state;
	uintptr_t user_stack;
};

#endif
