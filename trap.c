#include <stdint.h>
#include "utils.h"

void handle_trap() {

	uintptr_t mcause, mepc, mtval;
	
	asm volatile("csrr %0, mcause" : "=r"(mcause));
	asm volatile("csrr %0, mepc" : "=r"(mepc));
	asm volatile("csrr %0, mtval" : "=r"(mtval));

	utils_printf("[trap] mcause = %x\n", mcause);
	utils_printf("[trap] mepc = %x\n", mepc);
	utils_printf("[trap] mtval = %x\n", mtval);

    // Handle different trap causes appropriately
    switch (mcause) {
        case 0x2: // Illegal instruction
            utils_printf("[trap] Illegal instruction at %x\n", mepc);
            // Don't advance mepc - this indicates a real problem
            while(1); // Halt for debugging
            break;
            
        case 0x1: // Instruction access fault
            utils_printf("[trap] Instruction access fault at %x\n", mepc);
            // Don't advance mepc - this indicates invalid memory access
            while(1); // Halt for debugging
            break;
            
        case 0x8: // Environment call from U-mode
        case 0x9: // Environment call from S-mode
        case 0xB: // Environment call from M-mode
            utils_printf("[trap] Environment call (ecall)\n");
            // For ecall, we should advance mepc to continue after the ecall
            mepc += 4;
            asm volatile("csrw mepc, %0" :: "r"(mepc));
            break;
            
        default:
            utils_printf("[trap] Unknown trap cause: %x\n", mcause);
            while(1); // Halt for debugging
            break;
    }

}
