#ifndef SYSCALL_H
#define SYSCALL_H

enum syscall_id {
	SYSCALL_WRITE = 1,
	SYSCALL_WRITE_STR = 2,
	SYSCALL_EXIT = 3
};

void syscall_dispatch(uintptr_t syscall_id, uintptr_t a0, uintptr_t a1, uintptr_t a2);

#endif
