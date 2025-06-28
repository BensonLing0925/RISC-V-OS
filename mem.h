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
#define USER_STACK_TOP 0x7FFFFFF0UL      // top of user stack
#define USER_STACK_SIZE PAGE_SIZE        // 1 page for now

#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)
#define PTE_G (1L << 5)
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

#define IS_BIT_SET(n,x)   (((n & (1 << x)) != 0) ? 1 : 0)
#define VPN(level, addr)	(((uintptr_t)addr >> (12 + 9 * (level))) & 0x1ff)

void setup_pmp();
void setup_pagetable();
void enable_paging(uintptr_t* pagetable);
void memset(void* mem, char ch, uint32_t size);
void memcpy(void* mem1, const void* mem2, size_t n);
void* alloc_page();
void* walk(uintptr_t* root, void* v_addr);
void map_page(uintptr_t* l2_table, void* v_addr, void* p_addr, uint64_t flags);

#endif
