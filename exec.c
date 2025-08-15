#include "exec.h"
#include "utils.h"

extern uintptr_t next_user_free;
extern uint64_t* __attribute__((aligned(4096))) kernel_l2_pagetable;
uint64_t* __attribute__((aligned(4096))) user_pagetable;

// Add this right after setup_kernel_pagetable() in your main()
void test_direct_mapping() {
    utils_printf("Testing direct mapping...\n");
    
    // Test a few addresses in the direct mapping region
    volatile uint64_t* test1 = (volatile uint64_t*)PHYS_TO_VIRT(0x80000000);
    volatile uint64_t* test2 = (volatile uint64_t*)PHYS_TO_VIRT(0x80100000);
    volatile uint64_t* test3 = (volatile uint64_t*)PHYS_TO_VIRT(0x80200000);  
    volatile uint64_t* test4 = (volatile uint64_t*)PHYS_TO_VIRT(0x80201000);  // This should work
    
    utils_printf("Access 0xC0000000: %x\n", *test1);  // Should work
    utils_printf("Access 0xC0100000: %x\n", *test2);  // Should work
    utils_printf("Access 0xC0200000: %x\n", *test3);  // Should work  
    utils_printf("Access 0xC0201000: %x\n", *test4);  // This might fault!
}

// single user process first
uintptr_t load_user_program(const char* user_program, size_t program_size) {

	test_direct_mapping();

	user_pagetable = (uint64_t*) alloc_page();
	utils_printf("user_pagetable start: %x\n", &user_pagetable);

	// copy user program onto pages
	for (size_t offset = 0 ; offset < program_size ; offset += PAGE_SIZE) {
		void* page = alloc_page();
		map_page(user_pagetable, (void*) (USER_BASE + offset), page, PTE_V | PTE_X | PTE_R | PTE_U | PTE_W, alloc_page);		
		size_t copy_size = (program_size - offset) < PAGE_SIZE ? (program_size - offset) : PAGE_SIZE;
		// memcpy(page, (const void*) user_program + offset, copy_size);
    	memcpy((void*)PHYS_TO_VIRT((uintptr_t)page), 
        (const void*) user_program + offset, copy_size);
	}

	// allocate user stack
	uintptr_t user_stack = (uintptr_t)alloc_page();
	map_page(user_pagetable, (void*) (USER_STACK_TOP - PAGE_SIZE), (void*) user_stack, PTE_V | PTE_W | PTE_U | PTE_R | PTE_X, alloc_page);	

	utils_printf("User stack mapped: vaddr=%x, paddr=%x\n", USER_STACK_TOP - PAGE_SIZE, user_stack);

	return USER_BASE;
}
