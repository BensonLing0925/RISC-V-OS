#include <stdint.h>

#define UART_ADDR 0x10000000UL
#define START_ADDR 0x80000000UL
#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)
#define PTE_G (1L << 5)
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

uint64_t __attribute__((aligned(4096))) kernel_l2_pagetable[512];

void setup_pmp() {
    // Set up PMP to permit access to all memory
    // PMP entry 0: Allow all access to entire address space
    
    // Set pmpaddr0 to maximum value (all 1s shifted right by 2)
    // This covers the entire address space
    uintptr_t pmpaddr = 0xffffffffffffffffUL >> 2;
    asm volatile("csrw pmpaddr0, %0" :: "r"(pmpaddr));
    
    // Set pmpcfg0 with:
    // - A = 01 (TOR - Top of Range)
    // - R = 1 (Read permission)
    // - W = 1 (Write permission) 
    // - X = 1 (Execute permission)
    // This gives us: 0x0F (bits 3:0 = 1111)
    uintptr_t pmpcfg = 0x0F;
    asm volatile("csrw pmpcfg0, %0" :: "r"(pmpcfg));
}

void setup_pagetable() {
	kernel_l2_pagetable[START_ADDR >> 30] = 
		((START_ADDR >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X;

    // Map UART MMIO 0x10000000 - 0x101FFFFF
    kernel_l2_pagetable[UART_ADDR >> 30] = 
        ((UART_ADDR >> 12) << 10) | PTE_V | PTE_R | PTE_W;
}

void enable_paging() {
	uintptr_t pagetable = (uintptr_t)kernel_l2_pagetable;
	uintptr_t satp = (8UL << 60) | (pagetable >> 12);

	asm volatile("csrw satp, %0" :: "r"(satp));
	asm volatile("sfence.vma zero, zero");
}

void* alloc_page() {
	
}
