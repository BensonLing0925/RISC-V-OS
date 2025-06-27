#include <stdint.h>
#include "utils.h"

void smain() {
    // CRITICAL: First instruction - set up stack pointer from sscratch
    asm volatile("csrw satp, zero");
    // Immediate debug output to confirm we reached smain
    utils_printf("=== SMAIN ENTERED ===\n");
    utils_printf("smain: Successfully entered S-mode!\n");

	uint64_t syscall = 2;
	const char* msg = "This is a test";
	asm volatile("mv a7, %0" :: "r"(syscall));
	asm volatile("mv a0, %0" :: "r"(msg));
	asm volatile("ecall");

	utils_printf("Back from s_handle_trap\n");
    
    // Simple infinite loop without function calls to avoid stack issues
    while (1) {
        asm volatile("wfi"); // Wait for interrupt - safe in S-mode
    }
}
