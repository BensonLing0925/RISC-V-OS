#include <stddef.h>
#include <stdint.h>
#include "utils.h"

extern char __bss_start;
extern char __bss_end;
extern char sstack_top[];
extern char sstack_bottom[];
extern void trap_entry();
extern void s_trap_entry();
extern void smain();
extern void setup_pagetable();
extern void enable_paging();
extern void setup_pmp();

void zero_bss() {
	char* bss = &__bss_start;
	while (bss < &__bss_end) {
		*bss++ = 0;
	}
}

void init_trap() {
	uintptr_t addr = (uintptr_t)trap_entry;
	asm volatile("csrw mtvec, %0" :: "r"(addr));
}

void jump_to_smode() {
    uintptr_t smain_addr = (uintptr_t)smain;
    uintptr_t sstack_addr = (uintptr_t)sstack_top;
    
    // CRITICAL: Set up PMP before mret
    setup_pmp();
    
    // Set up page table and enable paging BEFORE configuring delegation
    setup_pagetable();
    enable_paging();
    
    // Configure mstatus for S-mode transition
    uintptr_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));
    
    mstatus &= ~(3UL << 11);  // Clear MPP (bits 12:11)
    mstatus |= (1UL << 11);   // Set MPP to S-mode (01)
    mstatus |= (1UL << 13);   // Set MPIE to enable interrupts after mret
    
    asm volatile("csrw mstatus, %0" :: "r"(mstatus));
    
    // Verify mstatus was written correctly
    uintptr_t mstatus_readback;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus_readback));
    
    // Set return address for mret
    asm volatile("csrw mepc, %0" :: "r"(smain_addr));
    
    // Verify mepc
    uintptr_t mepc_readback;
    asm volatile("csrr %0, mepc" : "=r"(mepc_readback));
    
    // Set up S-mode trap vector
    asm volatile("csrw stvec, %0" :: "r"((uintptr_t)s_trap_entry));
    
    // Initialize sstatus
    asm volatile("csrw sstatus, %0" :: "r"(0));
    
    // Store stack pointer in sscratch for S-mode to use
    asm volatile("csrw sscratch, %0" :: "r"(sstack_addr));
    
    utils_printf("About to execute mret to switch to S-mode...\n");
    
    asm volatile("csrw medeleg, %0" :: "r"(0xffff));
    asm volatile("csrw mideleg, %0" :: "r"(0xffff));

	asm volatile("mret");
}
