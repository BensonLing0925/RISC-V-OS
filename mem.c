#include <stdint.h>
#include <stddef.h>
#include "utils.h"
#include "mem.h"

extern char end[];
uintptr_t next_free = (uintptr_t)end;
uintptr_t next_user_free = (uintptr_t)KERNEL_END;

uint64_t* __attribute__((aligned(4096))) kernel_l2_pagetable;
uint64_t* kstack_pa;


void debug_page_mapping(uintptr_t* pagetable, uintptr_t va) {
    // Walk the page table manually to see if mapping exists
    uintptr_t* pte = walk_noalloc(pagetable, (void*) va);
    if (pte && (*pte & PTE_V)) {
        utils_printf("✓ Mapping exists for VA=%x, PTE=%x\n", va, *pte);
    } else {
        utils_printf("✗ No mapping for VA=%x\n", va);
    }
}

// Add this function to debug the exact page table mapping issue
void debug_current_stack_mapping(uintptr_t* user_pagetable) {
    utils_printf("=== Current Stack Mapping Debug ===\n");
    
    uintptr_t current_sp;
    asm volatile("mv %0, sp" : "=r"(current_sp));
    utils_printf("Current SP: %x\n", current_sp);
    
    // Calculate which page the current stack is in
    uintptr_t stack_page = current_sp & ~(PAGE_SIZE - 1);
    utils_printf("Current stack page (virtual): %x\n", stack_page);
    
    // This should be the kernel stack page we mapped
    utils_printf("Expected kernel stack page: %x\n", KSTACK_TOP - PAGE_SIZE);
    utils_printf("Kernel stack physical: %x\n", (uintptr_t)kstack_pa);
    
    if (stack_page == (KSTACK_TOP - PAGE_SIZE)) {
        utils_printf("✓ Current SP is in expected kernel stack page\n");
    } else {
        utils_printf("✗ Current SP is NOT in expected kernel stack page!\n");
        utils_printf("This could cause issues when switching page tables\n");
    }
}

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
	kstack_pa = (uint64_t*) stack;
    map_page_physical(kernel_pagetable, (void*)(KSTACK_TOP - PAGE_SIZE), (void*)stack, PTE_V | PTE_R | PTE_W, alloc_page);

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
		
	// identity mapping
	for ( uint64_t va = KERNEL_START ; va < KERNEL_END ; va += PAGE_SIZE ) {
        uintptr_t* entry = walk_physical(kernel_l2_pagetable, (void*)va, alloc_page);
        *entry = ((va >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X;
	}	

	// direct mapping
    for ( uintptr_t pa = PHYS_BASE ; pa < PHYS_END ; pa += PAGE_SIZE ) {
        uintptr_t va = PHYS_TO_VIRT(pa);
        uintptr_t* entry = walk_physical(kernel_l2_pagetable, (void*)va, alloc_page);
        *entry = ((pa >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X;
    }

	// identity mapping for copy from user, fixed
	for ( uintptr_t va = SCRATCH_VA_START ; va < SCRATCH_VA_END ; va += PAGE_SIZE ) {
		uintptr_t* entry = walk_physical(kernel_l2_pagetable, (void*)va, alloc_page);
		*entry = ((va >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X;
	}

	// identity mapping for UART
	map_page_physical(kernel_l2_pagetable, (void*)UART_ADDR, (void*)UART_ADDR, PTE_V | PTE_R | PTE_W, alloc_page);

	setup_kernel_stack(kernel_l2_pagetable);
}

void enable_paging(uint64_t* pagetable) {
	uintptr_t satp = (8UL << 60) | ((uintptr_t)pagetable >> 12);

	asm volatile("csrw satp, %0" :: "r"(satp));
	asm volatile("sfence.vma zero, zero");
}

void kernel_map_user_debug(uintptr_t* user_pagetable) {

	utils_printf("NEXT_PAGE: %x\n", next_free);

    utils_printf("=== kernel_map_user_debug ===\n");
    utils_printf("KERNEL_START: %x\n", KERNEL_START);
    utils_printf("KERNEL_END: %x\n", KERNEL_END);
    utils_printf("PAGE_SIZE: %x\n", PAGE_SIZE);
    
    int pages_mapped = 0;
    for (uintptr_t va = KERNEL_START; va < KERNEL_END; va += PAGE_SIZE) {
        map_page(user_pagetable, (void*)va, (void*)va, PTE_V | PTE_R | PTE_W | PTE_X, alloc_page);
        pages_mapped++;
        
        // Print first few and last few mappings
        if (pages_mapped <= 3 || pages_mapped > (KERNEL_END - KERNEL_START)/PAGE_SIZE - 3) {
            utils_printf("Mapped kernel page: VA=%x -> PA=%x\n", va, va);
        }
    }
    utils_printf("Total kernel pages mapped: %d\n", pages_mapped);

    // Map UART
    map_page(user_pagetable, (void*)UART_ADDR, (void*)UART_ADDR, PTE_V | PTE_R | PTE_W, alloc_page);
    utils_printf("Mapped UART: %x\n", UART_ADDR);
    
    // Check if current PC is in mapped range
    uintptr_t current_pc;
    asm volatile("auipc %0, 0" : "=r"(current_pc));
    utils_printf("Current PC: %x\n", current_pc);
    
    if (current_pc >= KERNEL_START && current_pc < KERNEL_END) {
        utils_printf("✓ Current PC is in mapped kernel range\n");
    } else {
        utils_printf("✗ Current PC is NOT in mapped kernel range!\n");
        utils_printf("This will cause a page fault when switching page tables\n");
    }
}

/*
void kernel_map_user(uintptr_t* user_pagetable) {

    for (uintptr_t va = KERNEL_START; va < KERNEL_END; va += PAGE_SIZE) {
        map_page(user_pagetable, (void*)va, (void*)va, PTE_V | PTE_R | PTE_W | PTE_X);
    }

    // Map UART (essential for kernel I/O!)
    map_page(user_pagetable, (void*)UART_ADDR, (void*)UART_ADDR, PTE_V | PTE_R | PTE_W);
    
    // Map kernel stack (essential for trap handling!)
	map_page(user_pagetable, (void*) (KSTACK_TOP - PAGE_SIZE), kstack_pa, PTE_V | PTE_R | PTE_W);

}
*/

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

// SATP MUST BE 4KB-ALIGNED !!!!!!!!!
void* alloc_page() {
    // Align next_free to PAGE_SIZE if not already aligned
    if (next_free & (PAGE_SIZE - 1)) {
        next_free = (next_free + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    }

    uintptr_t page = next_free;
    memset((void*)page, 0, PAGE_SIZE);
    next_free += PAGE_SIZE;
	utils_printf("next_free: %x\n", next_free);
    return (void*)page;
}

void* alloc_user_page() {
    if (next_user_free & (PAGE_SIZE - 1)) {
        next_user_free = (next_user_free + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    }

    uintptr_t page = next_user_free;
    memset((void*)(PHYS_TO_VIRT(page)), 0, PAGE_SIZE);
    next_user_free += PAGE_SIZE;
    return (void*)page;
}

// if L2's entry is missing, walk will create L1 and L0 table and its entry
pte_t* walk(uintptr_t* root, void* v_addr, page_alloc_fn allocator) {
	for (int level = 2 ; level > 0 ; level--) {
		uint64_t index = VPN(level, v_addr);
		// allocate PTE
		if ((root[index] & PTE_V) == 0) {
			uintptr_t page = (uintptr_t)allocator();
			// memset((void*)page, 0, PAGE_SIZE);
			memset((void*)PHYS_TO_VIRT(page), 0, PAGE_SIZE);
			root[index] = ((page >> 12) << 10) | PTE_V;
		}
		uintptr_t pa = ((root[index] >> 10) << 12);
		root = (uintptr_t*) PHYS_TO_VIRT(pa);
	}
	return &root[VPN(0, v_addr)];
}

pte_t* walk_noalloc(uintptr_t* root, void* v_addr) {
	for (int level = 2 ; level > 0 ; level--) {
		uint64_t index = VPN(level, v_addr);
		if ((root[index] & PTE_V) == 0) {
			return NULL;  // missing intermediate page table
		}
		uintptr_t pa = ((root[index] >> 10) << 12);
		root = (uintptr_t*) PHYS_TO_VIRT(pa);
	}
	return &root[VPN(0, v_addr)];
}

// for M mode setup only
pte_t* walk_physical(uintptr_t* root, void* v_addr, page_alloc_fn allocator) {
    for (int level = 2 ; level > 0 ; level--) {
        uint64_t index = VPN(level, v_addr);
        if ((root[index] & PTE_V) == 0) {
            uintptr_t page = (uintptr_t)allocator();  // Physical address (e.g., 0x80205000)
            memset((void*)page, 0, PAGE_SIZE);        // Access physical directly in M-mode
            root[index] = ((page >> 12) << 10) | PTE_V;
        }
        
        // In M-mode: use physical address directly (no PHYS_TO_VIRT conversion)
        uintptr_t phys_addr = (root[index] >> 10) << 12;
        root = (uintptr_t*)phys_addr;  // Direct physical access
    }
    return &root[VPN(0, v_addr)];
}

void map_page_physical(uintptr_t* l2_table, void* v_addr, void* p_addr, uint64_t flags, page_alloc_fn allocator) {
    uintptr_t* entry = walk_physical(l2_table, v_addr, allocator);
    *entry = ((uintptr_t)p_addr >> 12 << 10) | flags;
}

// map the physical address to virtual address
void map_page(uintptr_t* l2_table, void* v_addr, void* p_addr, uint64_t flags, page_alloc_fn allocator) {
    uintptr_t* entry = (uintptr_t*)walk(l2_table, v_addr, allocator);
    *entry = ((uintptr_t)p_addr >> 12 << 10) | flags;
}
