#include <stddef.h>
#include <stdint.h>
#include "mem.h"
#include "buddy.h"
#include "utils.h"

extern char __bss_start;
extern char __bss_end;
extern char sstack_top[];
extern char sstack_bottom[];
extern uint64_t* kernel_l2_pagetable;
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
}

void jump_to_smode() {

	// enable MMU first
    enable_paging(kernel_l2_pagetable);

    uintptr_t smain_addr = (uintptr_t)smain;
    
	uintptr_t sstack_addr = (uintptr_t)alloc_page();
	map_page_physical(kernel_l2_pagetable, (void*)SSTACK_ADDR, (void*)sstack_addr, PTE_V | PTE_W | PTE_R, alloc_page );

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

void jump_to_umode(uintptr_t user_prog_entry, uintptr_t* pagetable) {

	// buddy_init();	
	test_buddy();
    
    // Configure mstatus for S-mode transition
    uintptr_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    
	// set SPP to user mode
	sstatus &= ~(1L << 8);

	// enable interrupt in user mode
	sstatus |= (1L << 5);


	asm volatile("csrw sstatus, %0" :: "r"(sstatus));
	asm volatile("csrw sepc, %0" :: "r"(user_prog_entry));

    // asm volatile("csrw sedeleg, %0" :: "r"(0xffff));
    // asm volatile("csrw sideleg, %0" :: "r"(0xffff));

	// Map kernel into user page table but DON'T switch yet
    kernel_map_user_debug(pagetable);
    
    // Prepare SATP value but don't write it yet
    uintptr_t satp = (8UL << 60) | ((uintptr_t)pagetable >> 12);
    
    utils_printf("About to sret with page table switch\n");
    utils_printf("New SATP value: %x\n", satp);
    
    // CRITICAL FIX: Set stack pointer to within the mapped stack page
    uintptr_t user_stack_ptr = USER_STACK_TOP;
	utils_printf("user_stack_ptr: %x\n", user_stack_ptr);
	// asm volatile("mv sp, %0" :: "r"(user_stack_ptr));
    
	/*
    // Switch page table AND jump to user mode atomically
    asm volatile(
        "csrw satp, %1\n\t"          // Switch page table
        "sfence.vma zero, zero\n\t"   // Flush TLB
        "mv sp, %0\n\t"              // Set user stack pointer
        "sret"                       // Jump to user mode
        :: "r"(user_stack_ptr), "r"(satp)
        : "memory"
    );
	*/

	asm volatile(
		"csrw satp, %1\n\t"
		"sfence.vma\n\t"
		"mv sp, %3\n\t"
		"lb t1, %2\n\t"           // Load 'O' character (79)
		"sb t1, 0(%0)\n\t"        // Write to UART
		"addi t1, t1, -4\n\t"     // Change to 'K' (79-4=75)  
		"sb t1, 0(%0)\n\t"        // Write to UART
		"li t1, 10\n\t"           // '\n'
		"sb t1, 0(%0)\n\t"        // Write to UART
		"sret"
		:: "r"(UART_ADDR), "r"(satp), "m"(*(char*)&"O"[0]), "r"(user_stack_ptr)
		: "memory", "t1"
	);

}

void jump_to_umode_debug(uintptr_t user_prog_entry, uintptr_t* pagetable) {
    utils_printf("=== jump_to_umode_debug ===\n");
    
    // Configure sstatus for U-mode transition
    uintptr_t sstatus;
    asm volatile("csrr %0, sstatus" : "=r"(sstatus));
    utils_printf("Current sstatus: %x\n", sstatus);
    
    // set SPP to user mode
    sstatus &= ~(1L << 8);
    // enable interrupt in user mode
    sstatus |= (1L << 5);
    asm volatile("csrw sstatus, %0" :: "r"(sstatus));
    asm volatile("csrw sepc, %0" :: "r"(user_prog_entry));
    
    utils_printf("Set sepc to: %x\n", user_prog_entry);
    utils_printf("Set sstatus to: %x\n", sstatus);

    // Map kernel into user page table
    kernel_map_user_debug(pagetable);

    // Prepare SATP value
    uintptr_t satp = (8UL << 60) | ((uintptr_t)pagetable >> 12);
    utils_printf("New SATP value: %x\n", satp);
    utils_printf("Page table physical address: %x\n", (uintptr_t)pagetable);

    // Set user stack pointer
    uintptr_t user_stack_ptr = USER_STACK_TOP - 8;
    utils_printf("User stack pointer: %x\n", user_stack_ptr);
    
    // Get current stack pointer for comparison
    uintptr_t current_sp;
    asm volatile("mv %0, sp" : "=r"(current_sp));
    utils_printf("Current SP: %x\n", current_sp);
    
    utils_printf("About to switch page table and sret...\n");
    utils_printf("If this hangs, the issue is in the page table switch\n");
    
    // Try a safer approach - disable interrupts first
    asm volatile("csrci sstatus, 0x2"); // Disable interrupts
    
    // Switch page table AND jump to user mode atomically
    asm volatile(
        "fence\n\t"                   // Memory fence before switch
        "csrw satp, %1\n\t"          // Switch page table
        "sfence.vma zero, zero\n\t"   // Flush TLB
        "fence\n\t"                   // Memory fence after TLB flush
        "mv sp, %0\n\t"              // Set user stack pointer
        "sret"                        // Jump to user mode
        :: "r"(user_stack_ptr), "r"(satp)
        : "memory"
    );
}

