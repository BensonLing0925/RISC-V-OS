#include <stdint.h>
#include "syscall.h"
#include "utils.h"

// a7 for syscall_id, a0-2 for arg0-2. System call convention
void syscall_dispatch(uintptr_t syscall_id, uintptr_t a0, uintptr_t a1, uintptr_t a2) {
	switch (syscall_id) {
		case SYSCALL_WRITE:	// takes 1 argument
			utils_printf("a0: %x\n", a0);
			utils_printf("SYSCALL_WRITE: %c\n", a0);	
			break;
		case SYSCALL_WRITE_STR:	// takes 1 argument
			utils_printf("SYSCALL_WRITE_STR: %s\n", a0);	
			break;
		case SYSCALL_EXIT:
			utils_printf("System Exit\n");
			while (1) ;  // halting
	}
}
