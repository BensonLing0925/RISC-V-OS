#include <stdint.h>
#include <stddef.h>
#include "mem.h"

extern char end[];
uintptr_t next_free = (uintptr_t)end;

uint64_t* __attribute__((aligned(4096))) kernel_l2_pagetable;

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

__attribute__((noreturn))
void setup_kernel_stack(uint64_t* kernel_pagetable) {
    void* stack = alloc_page();
    map_page(kernel_pagetable, (void*)(KSTACK_TOP - PAGE_SIZE), (void*)stack, PTE_V | PTE_R | PTE_W);

    asm volatile(
        "mv sp, %0\n"
        "j jump_to_smode\n" 
        :
        : "r"(KSTACK_TOP)
    );

    __builtin_unreachable(); // make compiler happy
}

void setup_kernel_pagetable() {

	kernel_l2_pagetable = alloc_page();
		
	for ( uint64_t va = KERNEL_START ; va < KERNEL_END ; va += PAGE_SIZE ) {
		// identity
		map_page(kernel_l2_pagetable, (void*)va, (void*)va, PTE_V | PTE_R | PTE_W | PTE_X);
	}	

	map_page(kernel_l2_pagetable, (void*)UART_ADDR, (void*)UART_ADDR, PTE_V | PTE_R | PTE_W);

	setup_kernel_stack(kernel_l2_pagetable);
}

void enable_paging(uintptr_t* pagetable) {
	uintptr_t satp = (8UL << 60) | ((uintptr_t)pagetable >> 12);

	asm volatile("csrw satp, %0" :: "r"(satp));
	asm volatile("sfence.vma zero, zero");
}

void memset(void* mem, char ch, uint32_t size) {
	char* ptr = (char*)mem;
	for ( uint32_t i = 0 ; i < size ; ++i ) {
		ptr[i] = ch;
	}
}

void memcpy(void* mem1, const void* mem2, size_t n) {
	uintptr_t d = (uintptr_t)mem1;
	uintptr_t s = (uintptr_t)mem2;

	// check alignment
	while (n > 0 && (d & (sizeof(uintptr_t)-1))) {
		*(unsigned char*)d = *(const unsigned char*)s;
		d++;
		s++;
		n--;
	}

	// copy chunks of data
	while (n >= sizeof(uintptr_t)) {
		*(uintptr_t*)d = *(const uintptr_t*)s;
		n -= sizeof(uintptr_t);
		d += sizeof(uintptr_t);
		s += sizeof(uintptr_t);
	}

	while (n > 0) {
		*(unsigned char*)d = *(const unsigned char*)s;
		d++;
		s++;
		n--;
	}
}

void* alloc_page() {
	uintptr_t page = next_free;
	memset((void*)page, 0, PAGE_SIZE);
	next_free += PAGE_SIZE;
	return (void*)page;
}

// if L2's entry is missing, walk will create L1 and L0 table and its entry
void* walk(uintptr_t* root, void* v_addr) {
	for (int level = 2 ; level > 0 ; level--) {
		uint64_t index = VPN(level, v_addr);
		// allocate PTE
		if ((root[index] & PTE_V) == 0) {
			uintptr_t page = (uintptr_t)alloc_page();
			root[index] = ((page >> 12) << 10) | PTE_V;
		}
		root = (uintptr_t*)((root[index] >> 10) << 12);
	}
	return &root[VPN(0, v_addr)];
}

// map the physical address to virtual address
void map_page(uintptr_t* l2_table, void* v_addr, void* p_addr, uint64_t flags) {
    uintptr_t* entry = (uintptr_t*)walk(l2_table, v_addr);
    *entry = ((uintptr_t)p_addr >> 12 << 10) | flags;
}
