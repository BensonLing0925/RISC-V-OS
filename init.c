#include <stddef.h>
#include <stdint.h>
#include "mem.h"
#include "utils.h"

extern char __bss_start;
extern char __bss_end;
extern char sstack_top[];
extern char sstack_bottom[];
extern uintptr_t kernel_l2_pagetable;
extern void trap_entry();
extern void s_trap_entry();
extern void smain();
extern void umain();
extern void setup_kernel_pagetable();
extern void enable_paging(uintptr_t* pagetable);
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

void init_vm() {
    // CRITICAL: Set up PMP before mret
    setup_pmp();
    setup_kernel_pagetable();
    enable_paging(&kernel_l2_pagetable);
}

void jump_to_smode() {
    uintptr_t smain_addr = (uintptr_t)smain;
    // uintptr_t sstack_addr = (uintptr_t)sstack_top;
    
    
	uintptr_t sstack_addr = (uintptr_t)alloc_page();
	map_page(&kernel_l2_pagetable, (void*)SSTACK_ADDR, (void*)sstack_addr, PTE_V | PTE_W | PTE_R );

    // Configure mstatus for S-mode transition
    uintptr_t mstatus;
    asm volatile("csrr %0, mstatus" : "=r"(mstatus));
    
    mstatus &= ~(3UL << 11);  // Clear MPP (bits 12:11)
    mstatus |= (1UL << 11);   // Set MPP to S-mode (01)
    mstatus |= (1UL << 13);   // Set MPIE to enable interrupts after mret
    
    asm volatile("csrw mstatus, %0" :: "r"(mstatus));
    
    // Set return address for mret
    asm volatile("csrw mepc, %0" :: "r"(smain_addr));
    
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

void jump_to_umode(uintptr_t user_prog_entry, uintptr_t pagetable) {
    
    // Configure mstatus for S-mode transition
    uintptr_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));

	enable_paging(&pagetable);
    
	// set SPP to user mode
	sstatus &= ~(1L << 8);

	// enable interrupt in user mode
	sstatus |= (1L << 5);

	asm volatile("mv sp, %0" :: "r"(USER_STACK_TOP));

	uintptr_t user_satp = (8UL << 60) | ((uintptr_t)pagetable >> 12);  // MODE = 8 = SV39
	asm volatile("csrw satp, %0" :: "r"(user_satp));
	asm volatile("sfence.vma");  // flush TLB

	asm volatile("csrw sstatus, %0" :: "r"(sstatus));
	asm volatile("csrw sepc, %0" :: "r"(user_prog_entry));

    asm volatile("csrw sedeleg, %0" :: "r"(0xffff));
    asm volatile("csrw sideleg, %0" :: "r"(0xffff));

	asm volatile("sret");
}


