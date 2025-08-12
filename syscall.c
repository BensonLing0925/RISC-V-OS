#include <stdint.h>
#include "riscv.h"
#include "mem.h"
#include "syscall.h"
#include "utils.h"

#define BUF_SIZE 128

extern int copy_from_user(void* dest, const void* user_src, size_t len);
// a7 for syscall_id, a0-2 for arg0-2. System call convention
void syscall_dispatch(uintptr_t syscall_id, uintptr_t a0, uintptr_t a1, uintptr_t a2) {
	switch (syscall_id) {
		case SYSCALL_WRITE:	// takes 1 argument
			utils_printf("a0: %x\n", a0);
			utils_printf("SYSCALL_WRITE: %c\n", a0);	
			break;
		case SYSCALL_WRITE_STR:	// takes 1 argument
			char buf[BUF_SIZE];
			buf[BUF_SIZE-1] = '\0';
			uintptr_t sstatus = read_sstatus();

			// Determine if syscall came from U-mode or S-mode
			if ((sstatus & (1UL << 8)) == 0) {
				// syscall from U-mode, must safely copy
				if (copy_from_user(buf, (const char *)a0, BUF_SIZE - 1) < 0) {
					utils_printf("Invalid user string pointer\n");
					break;
				}
			} else {
				// syscall from S-mode — just copy directly (only for testing!)
				memcpy(buf, (const char *)a0, BUF_SIZE - 1);
				buf[BUF_SIZE - 1] = '\0';
			}

			utils_printf("SYSCALL_WRITE_STR: %s\n", buf);	
			break;
		case SYSCALL_EXIT:
			utils_printf("System Exit\n");
			while (1) ;  // halting
	}
}
