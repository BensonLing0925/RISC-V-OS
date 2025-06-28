#include "exec.h"

uint64_t* __attribute__((aligned(4096))) user_pagetable;

// single user process first
uintptr_t load_user_program(const char* user_program, size_t program_size) {

	user_pagetable = alloc_page();

	// copy user program onto pages
	for (size_t offset = 0 ; offset < program_size ; offset += PAGE_SIZE) {
		void* page = alloc_page();
		map_page(user_pagetable, (void*) (USER_BASE + offset), page, PTE_V | PTE_X | PTE_R | PTE_U);		
		memcpy(page, user_program, PAGE_SIZE);
	}

	// allocate user stack
	uintptr_t user_stack = (uintptr_t)alloc_page();
	map_page(user_pagetable, (void*) (USER_STACK_TOP - PAGE_SIZE), (void*) user_stack, PTE_V | PTE_W | PTE_U);		
	return USER_BASE;
}
