#include <stdint.h>
void umain() {
    register long a7 asm("a7") = 0x2; // syscall number for SYS_write
	const char* msg = "Hello from the user program!\n";
    register long a0 asm("a0") = (uintptr_t)msg; // syscall number for SYS_write
    asm volatile("ecall");
}
