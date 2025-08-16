#ifndef BUDDY_H
#define BUDDY_H

#include <stdint.h>
#include <stddef.h>
#include "mem.h"

#define MAX_ORDER 13

#define PG_RESERVED   (1U << 0)  // page is not allocatable by buddy
#define PG_FREE       (1U << 1)  // page is currently free
#define PG_ALLOCATED	(1U << 2)// page is allocated

#define MAX_PFN	(PHYS_END - PHYS_BASE) / PAGE_SIZE

// from 0x80000000 to 0x84000000
extern struct page* page_arr;
extern struct buddy_zone* zone;
extern size_t npages;

struct page {
	uint16_t order;
	uint16_t flags;
	uint32_t refcnt;
	struct page* next;
};

struct free_area {
	struct page* free_list;
	size_t nr_free;
	void* lock;	// type haven't decided yet
};

struct buddy_zone {
	paddr_t start_paddr;
	paddr_t end_paddr;
	size_t npages;
	struct page* page_meta_base;
	struct free_area order_area[MAX_ORDER];
	void* zone_lock;
};

void buddy_init();
void init_free_area(size_t free_start_pfn);
struct page* buddy_alloc(size_t order);
void buddy_free(paddr_t block_paddr, size_t order);
void test_buddy();
void test_buddy_basic();
void test_buddy_split_merge();
void test_buddy_exhaust();
void test_buddy_coalesce_order();
void test_buddy_fragmentation();

static inline paddr_t page_to_phys(struct page* p) {
	size_t pfn = p - page_arr;
	return PHYS_BASE + (pfn << 12);
}

static inline void* page_to_virt(struct page* p) {
	return PHYS_TO_VIRT((void*)page_to_phys(p));
}

static inline uintptr_t align_up(uintptr_t va, size_t order) {
    uintptr_t size = 1ULL << (order + PAGE_OFFSET);
    return (va + size - 1) & ~(size - 1);
}

#endif
