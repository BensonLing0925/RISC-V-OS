#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096
#define KERNEL_START 0x80000000UL
#define KERNEL_END 0x80200000UL
#define KSTACK_TOP 0x80010000UL
#define KSTACK_SIZE PAGE_SIZE
#define SSTACK_ADDR 0x80020000UL
#define SSTACK_SIZE PAGE_SIZE
#define UART_ADDR 0x10000000UL

#define USER_BASE      0x00010000UL      // where user code starts
#define USER_MAX_VADDR 0x00030000UL
#define USER_STACK_TOP 0x80000000UL      // top of user stack 
#define USER_STACK_SIZE PAGE_SIZE        // 1 page for now

#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)
#define PTE_G (1L << 5)
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

#define IS_BIT_SET(n,x)	(((n & (1 << x)) != 0) ? 1 : 0)
#define VPN(level, addr)	(((uintptr_t)addr >> (12 + 9 * (level))) & 0x1ff)

// direct mapping for the rest of the physical memory region
#define PHYS_BASE	0x80000000
#define PHYS_END	0x84000000
#define DIRECT_MAP_OFFSET	0x40000000
// identity mapping range: 0x80000000 to 0x80200000
// direct mapping range: from (0x80000000 - 0x84000000) to (0xC0000000 - 0xC4000000)
#define PHYS_TO_VIRT(pa)	((pa) + DIRECT_MAP_OFFSET)
#define VIRT_TO_PHYS(va)	((va) - DIRECT_MAP_OFFSET)

// fixed memory region for copy_from_user
#define SCRATCH_VA_START	0xFFFF0000
#define SCRATCH_VA_END	0xFFFFFFFF

typedef uint64_t pte_t;
typedef pte_t* pagetable_t;
typedef void* (*page_alloc_fn)();
typedef uint64_t paddr_t;

void setup_pmp();
void setup_pagetable();
void enable_paging(uintptr_t* pagetable);
void enable_paging_user(uintptr_t* user_pagetable);
void kernel_map_user(uintptr_t* user_pagetable);
void kernel_map_user_debug(uintptr_t* user_pagetable);
void debug_current_stack_mapping(uintptr_t* user_pagetable);
void minimal_page_table_switch_test(uintptr_t* user_pagetable);
void memset(void* mem, char ch, uint32_t size);
void memcpy(void* mem1, const void* mem2, size_t n);
void* alloc_page();
void* alloc_user_page();
pte_t* walk(uintptr_t* root, void* v_addr, page_alloc_fn allocator);
pte_t* walk_noalloc(uintptr_t* root, void* v_addr);
pte_t* walk_physical(uintptr_t* root, void* v_addr, page_alloc_fn allocator);
void map_page(uintptr_t* l2_table, void* v_addr, void* p_addr, uint64_t flags, page_alloc_fn allocator);
void map_page_physical(uintptr_t* l2_table, void* v_addr, void* p_addr, uint64_t flags, page_alloc_fn allocator);

#endif
