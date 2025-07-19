#include <stdint.h>
#include <stddef.h>
#include "utils.h"
#include "mem.h"

extern char end[];
uintptr_t next_free = (uintptr_t)end;

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

// Minimal page table switch test - step by step
void minimal_page_table_switch_test(uintptr_t* user_pagetable) {
    utils_printf("=== Minimal Page Table Switch Test ===\n");
    
    // Debug current stack mapping
    debug_current_stack_mapping(user_pagetable);
    
    // Map kernel to user page table (you already do this)
    kernel_map_user_debug(user_pagetable);
    
    // Get current SATP
    uintptr_t old_satp;
    asm volatile("csrr %0, satp" : "=r"(old_satp));
    utils_printf("Current SATP: %x\n", old_satp);

	debug_page_mapping(user_pagetable, UART_ADDR);
    
    // Prepare new SATP
    uintptr_t new_satp = (8UL << 60) | ((uintptr_t)user_pagetable >> 12);
    utils_printf("New SATP: %x\n", new_satp);

	utils_printf("About to test critical addresses:\n");

	// Test the current instruction address
	uintptr_t current_pc;
	asm volatile("auipc %0, 0" : "=r"(current_pc));
	utils_printf("Current PC: %x\n", current_pc);

	// Test where user_pagetable variable is stored
	utils_printf("user_pagetable variable at: %x\n", (uintptr_t)&user_pagetable);
	utils_printf("user_pagetable points to: %x\n", (uintptr_t)user_pagetable);

	// Test stack pointer
	uintptr_t sp;
	asm volatile("mv %0, sp" : "=r"(sp));
	utils_printf("Current stack pointer: %x\n", sp);

	// Test if these addresses have mappings
	debug_page_mapping(user_pagetable, (uintptr_t)&user_pagetable);
	debug_page_mapping(user_pagetable, sp);
	debug_page_mapping(user_pagetable, current_pc);
	debug_page_mapping(user_pagetable, 0x800022b8);

	utils_printf("user_pagetable addr: %x (aligned = %s)\n", user_pagetable,
		((uintptr_t)user_pagetable & 0xFFF) ? "NO" : "YES");

    utils_printf("Step 1: About to write new SATP...\n");
    
	asm volatile(
		"csrw satp, %1\n\t"
		"sfence.vma\n\t"
		"lb t1, %2\n\t"           // Load 'O' character (79)
		"sb t1, 0(%0)\n\t"        // Write to UART
		"addi t1, t1, -4\n\t"     // Change to 'K' (79-4=75)  
		"sb t1, 0(%0)\n\t"        // Write to UART
		"li t1, 10\n\t"           // '\n'
		"sb t1, 0(%0)\n\t"        // Write to UART
		:: "r"(UART_ADDR), "r"(new_satp), "m"(*(char*)&"O"[0])
		: "memory", "t1"
	);
    
    utils_printf("Step 2: SATP written, about to flush TLB...\n");

	volatile char *uart = (volatile char *)UART_ADDR;
	*uart = 'O';  // Just write a single character
	*uart = 'K';
	*uart = '\n';
    
    utils_printf("Step 3: TLB flushed successfully!\n");
    utils_printf("If you see this, the page table switch worked!\n");
    
    // Try to access some memory to verify it's working
    uintptr_t test_addr = (uintptr_t)kernel_map_user;  // Should be in kernel range
    utils_printf("Step 4: Testing memory access at %x...\n", test_addr);
    
    // Read an instruction from the kernel function
    uint32_t instruction = *(uint32_t*)test_addr;
    utils_printf("Step 5: Successfully read instruction: %x\n", instruction);
    
    utils_printf("SUCCESS: Page table switch completed!\n");
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

void enable_paging(uint64_t* pagetable) {
	uintptr_t satp = (8UL << 60) | ((uintptr_t)pagetable >> 12);

	asm volatile("csrw satp, %0" :: "r"(satp));
	asm volatile("sfence.vma zero, zero");
}

void enable_paging_user(uint64_t* user_pagetable) {

    uintptr_t current_sp;
    asm volatile("mv %0, sp" : "=r"(current_sp));
    
    utils_printf("Current SP: %x\n", current_sp);
    utils_printf("K_STACK_TOP: %x\n", KSTACK_TOP);
    utils_printf("Mapped kstack virtual: %x\n", KSTACK_TOP - PAGE_SIZE);
    utils_printf("Mapped kstack physical: %x\n", (uintptr_t)kstack_pa);
    utils_printf("enable_paging addr: %x\n", (uintptr_t)enable_paging);

	kernel_map_user(user_pagetable);
	enable_paging(user_pagetable);
}

void kernel_map_user_debug(uintptr_t* user_pagetable) {
    utils_printf("=== kernel_map_user_debug ===\n");
    utils_printf("KERNEL_START: %x\n", KERNEL_START);
    utils_printf("KERNEL_END: %x\n", KERNEL_END);
    utils_printf("PAGE_SIZE: %x\n", PAGE_SIZE);
    
    int pages_mapped = 0;
    for (uintptr_t va = KERNEL_START; va < KERNEL_END; va += PAGE_SIZE) {
        map_page(user_pagetable, (void*)va, (void*)va, PTE_V | PTE_R | PTE_W | PTE_X);
        pages_mapped++;
        
        // Print first few and last few mappings
        if (pages_mapped <= 3 || pages_mapped > (KERNEL_END - KERNEL_START)/PAGE_SIZE - 3) {
            utils_printf("Mapped kernel page: VA=%x -> PA=%x\n", va, va);
        }
    }
    utils_printf("Total kernel pages mapped: %d\n", pages_mapped);

    // Map UART
    map_page(user_pagetable, (void*)UART_ADDR, (void*)UART_ADDR, PTE_V | PTE_R | PTE_W);
    utils_printf("Mapped UART: %x\n", UART_ADDR);

	/*
    // Map kernel stack
    map_page(user_pagetable, (void*)(KSTACK_TOP - PAGE_SIZE), kstack_pa, PTE_V | PTE_R | PTE_W);
    utils_printf("Mapped kstack: VA=%x -> PA=%x\n", KSTACK_TOP - PAGE_SIZE, (uintptr_t)kstack_pa);
	*/
    
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

void kernel_map_user(uintptr_t* user_pagetable) {

    for (uintptr_t va = KERNEL_START; va < KERNEL_END; va += PAGE_SIZE) {
        // Skip the page that contains our kernel stack
		/*
        if (va == (KSTACK_TOP - PAGE_SIZE)) {
            continue; // Don't identity map this page
        }
		*/
        map_page(user_pagetable, (void*)va, (void*)va, PTE_V | PTE_R | PTE_W | PTE_X);
    }

    // Map UART (essential for kernel I/O!)
    map_page(user_pagetable, (void*)UART_ADDR, (void*)UART_ADDR, PTE_V | PTE_R | PTE_W);
    
    // Map kernel stack (essential for trap handling!)
	map_page(user_pagetable, (void*) (KSTACK_TOP - PAGE_SIZE), kstack_pa, PTE_V | PTE_R | PTE_W);

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

// SATP MUST BE 4KB-ALIGNED !!!!!!!!!
void* alloc_page() {
    // Align next_free to PAGE_SIZE if not already aligned
    if (next_free & (PAGE_SIZE - 1)) {
        next_free = (next_free + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    }

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
			memset((void*)page, 0, PAGE_SIZE);
			root[index] = ((page >> 12) << 10) | PTE_V;
		}
		root = (uintptr_t*)((root[index] >> 10) << 12);
	}
	return &root[VPN(0, v_addr)];
}

void* walk_noalloc(uintptr_t* root, void* v_addr) {
	for (int level = 2 ; level > 0 ; level--) {
		uint64_t index = VPN(level, v_addr);
		if ((root[index] & PTE_V) == 0) {
			return NULL;  // missing intermediate page table
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
