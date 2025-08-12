#include <stdint.h>
#include <stddef.h>
#include "mem.h"
#include "utils.h"
#include "riscv.h"

extern uint64_t* user_pagetable;
extern uint64_t* kernel_l2_pagetable;

void map_to_scratch(void* src) {
	// currently using user PT, src is accessible
	// map_page(kernel_l2_pagetable, (void*)SCRATCH_VA_START, (void*)((uint64_t)src << 12), PTE_V | PTE_R, alloc_page);	
	map_page(kernel_l2_pagetable, (void*)SCRATCH_VA_START, (void*)src, PTE_V | PTE_R, alloc_page);	
}

int copy_from_user(void* dest, const void* user_src, size_t len) {
	size_t copied = 0;
	uintptr_t src = (uintptr_t)user_src;
	size_t bytes_left_in_page = 0;

	enable_sum();

	while (copied < len) {
		size_t offset = (src & 0xFFF);
		bytes_left_in_page = PAGE_SIZE - offset;
		size_t chunk = (len - copied < bytes_left_in_page) ? (len - copied) : (bytes_left_in_page);

		/*
		pte_t* pte = walk_noalloc(user_pagetable, (void*)(src & ~0xFFF));
		if (!pte || !(*pte & PTE_V) || !(*pte & PTE_U) || !(*pte & PTE_R)) {
			disable_sum();
			return -1;  // Invalid user page
		}
		*/

		memcpy((char*)dest + copied, (void*)src, chunk);

		src += chunk;
		copied += chunk;
	}

	disable_sum();

	return 0;
}

