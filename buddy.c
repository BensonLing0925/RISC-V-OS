#include "buddy.h"
#include "utils.h"

// from 0x80000000 to 0x84000000
struct page* page_arr;
struct buddy_zone* zone;
size_t npages;

void init_free_area(size_t reserved_page, size_t free_start_pfn) {
	utils_printf("Total pages: %x\n", npages);
	utils_printf("Reserved pages: %x\n", reserved_page);
	size_t free_page = npages - reserved_page;
	utils_printf("Free pages: %x\n", free_page);
	size_t order = MAX_ORDER - 1;
	while (free_page) {
		while (order > 0 && free_page < (1UL << order)) order--;	
		// free_list point to page_arr element, not the actual memory itself
		// since page_arr is already va, no need to convert explicitly
		struct page* p = &page_arr[free_start_pfn];
		// The actual page memory content doesn’t matter here; we only initialize metadata.
		// meta data page is allocated via alloc_user_page, which memset whole page to 0
		// no initialization for meta data needed
		p->order = order;
		// zone's free_list inititally NULL
		p->next = zone->order_area[order].free_list;	
		// add new memory block to the head of the list
		zone->order_area[order].free_list = p;
		zone->order_area[order].nr_free++;

		free_page -= (1UL << order);
		free_start_pfn += (1UL << order);
	}
}

void buddy_init() {
	npages = (PHYS_END - PHYS_BASE) / PAGE_SIZE;
	size_t page_arr_size = npages * sizeof(struct page);
	// since we allocate page_arr and zone separately
	// we need to calculate pages separately too
	size_t page_arr_npages = (page_arr_size + PAGE_SIZE - 1) / PAGE_SIZE;
	size_t zone_npages = (sizeof(struct buddy_zone) + PAGE_SIZE - 1) / PAGE_SIZE;

	// IMPORTANT:
	// when pointer dereferenced, it will go through MMU(pointer contains va, not pa)
	paddr_t page_arr_pa = (uintptr_t)alloc_user_page();
	page_arr = (struct page*) PHYS_TO_VIRT(page_arr_pa);
	utils_printf("PAGE ARRAY AT: %x\n", page_arr);
	for ( size_t i = 1 ; i < page_arr_npages ; ++i ) {
		alloc_user_page();  // update next_free_user mem address
	}

	paddr_t zone_pa = (uintptr_t)alloc_user_page();
	for (size_t i = 1; i < zone_npages; ++i)
		alloc_user_page();
	zone = (struct buddy_zone*)PHYS_TO_VIRT(zone_pa);
	utils_printf("ZONE META VIRT ADDR at: %x\n", zone);
	zone->start_paddr = PHYS_BASE;
	zone->end_paddr = PHYS_END;
	zone->npages = npages;
	zone->page_meta_base = page_arr;
	
	// mark kernel image(0x80000000 to 0x80200000) as reserved	
	size_t kernel_start_pfn = (KERNEL_START - PHYS_BASE) / PAGE_SIZE;
	size_t kernel_end_pfn   = (KERNEL_END   - PHYS_BASE) / PAGE_SIZE;

	for (size_t idx = kernel_start_pfn; idx < kernel_end_pfn; idx++) {
		page_arr[idx].flags |= PG_RESERVED;
		page_arr[idx].flags &= ~PG_FREE;
	}

	size_t meta_start_pfn = ((uintptr_t)page_arr_pa - PHYS_BASE) / PAGE_SIZE;
	size_t meta_end_pfn   = meta_start_pfn + page_arr_npages;

	for (size_t idx = meta_start_pfn; idx < meta_end_pfn; idx++) {
		page_arr[idx].flags |= PG_RESERVED;
		page_arr[idx].flags &= ~PG_FREE;
	}

	/*
	size_t reserved_page = (kernel_end_pfn - kernel_start_pfn) + page_arr_npages + zone_npages;
	size_t aligned_start = ((meta_end_pfn + (1UL << 12) - 1) / (1UL << 12)) * (1UL << 12);
	init_free_area(reserved_page + (aligned_start - meta_end_pfn), aligned_start);
	*/

	size_t reserved_page = (kernel_end_pfn - kernel_start_pfn) + page_arr_npages + zone_npages;
	init_free_area(reserved_page, meta_end_pfn);
}

struct page* buddy_alloc(size_t order) {
	for ( size_t i = order ; i < MAX_ORDER ; ++i ) {
		if (zone->order_area[i].free_list) {
			// split till the same order
			while (i > order) {
				struct page* p = zone->order_area[i].free_list;
				zone->order_area[i].free_list = zone->order_area[i].free_list->next;
				zone->order_area[i].nr_free--;

				i -= 1;

				struct page* upper_half = p + (1UL << (i));
				p->order = i;
				upper_half->order = i;
				p->next = zone->order_area[i].free_list;
				zone->order_area[i].free_list = p;
				upper_half->next = zone->order_area[i].free_list;
				zone->order_area[i].free_list = upper_half;
				zone->order_area[i].nr_free += 2;
			}
			struct page* ret = zone->order_area[order].free_list;
			// remove from free_list
			zone->order_area[order].free_list = zone->order_area[order].free_list->next;
			// mark it as allocated
			ret->next = NULL;
			ret->flags &= ~PG_FREE;
			ret->flags |= PG_ALLOCATED;
			return ret;
		}
	}
	return (struct page*)-1;
}

struct page* buddy_alloc_page(size_t order) {
	return buddy_alloc(order);
}

paddr_t buddy_alloc_phys(size_t order) {
	return page_to_phys(buddy_alloc(order));
}

void* buddy_alloc_kva(size_t order) {
	return page_to_virt(buddy_alloc(order));
}

static void unlink(struct page* buddy, size_t order) {
	struct page* prev = NULL;
	struct page* traverse = zone->order_area[order].free_list;
	while (traverse) {
		if (traverse == buddy) {
			if (!prev) {
				zone->order_area[order].free_list = traverse->next;
			}
			else {
				prev->next = traverse->next;
			}
			traverse->next = NULL;
			zone->order_area[order].nr_free--;
			break;
		}
		else {
			prev = traverse;
			traverse = traverse->next;
		}
	}
}

void buddy_free(paddr_t block_paddr, size_t order) {
	size_t pfn = (block_paddr - PHYS_BASE) / PAGE_SIZE;
	struct page* current = &page_arr[pfn];
	current->flags |= PG_FREE;
	current->flags &= ~PG_ALLOCATED;
	while (order < MAX_ORDER) {
		size_t buddy_pfn = pfn ^ (1UL << order);
		utils_printf("pfn: %d, buddy_pfn: %d\n", pfn, buddy_pfn);
		utils_printf("%d\n", MAX_PFN);
		if (buddy_pfn >= MAX_PFN) {
			break;
		}
		struct page* buddy_page = &page_arr[buddy_pfn];
		utils_printf("page->order: %d\n", current->order);
		utils_printf("buddy_page->order: %d\n", buddy_page->order);
		if ((buddy_page->flags & PG_FREE) && (buddy_page->order == order)) {
			// merge buddy
			unlink(buddy_page, order);
			pfn = (pfn < buddy_pfn) ? pfn : buddy_pfn;
			order++;
			struct page* merged_page = &page_arr[pfn];
			merged_page->next = zone->order_area[order].free_list;
			zone->order_area[order].free_list = merged_page;
		}
		else {
			break;
		}
	}
	struct page* p = &page_arr[pfn];
	p->flags |= PG_FREE;
	p->flags &= ~PG_ALLOCATED;
	p->order = order;
	p->next = zone->order_area[order].free_list;
	zone->order_area[order].free_list = p;
	zone->order_area[order].nr_free++;
}

void print_free_list() {
	utils_printf("========== print_free_list ==========\n");
	for ( size_t order = 0 ; order < MAX_ORDER ; ++order ) {
		struct page* p = zone->order_area[order].free_list;
		utils_printf("Order %d free_list: ", order);
		utils_printf("(page, pfn, paddr, vaddr)\n");
		while (p) {
			size_t pfn = p - page_arr;
			size_t paddr = PHYS_BASE + pfn * PAGE_SIZE;
			utils_printf("(%x, %x, %x, %x)\n", p, pfn, paddr, PHYS_TO_VIRT(paddr));
			p = p->next;
		}
	}
	utils_printf("========== print_free_list ==========\n");
}

void test_buddy() {

	buddy_init();
	print_free_list();

	struct page* p1 = buddy_alloc_page(6);
	print_free_list();
    buddy_free(page_to_phys(p1), 6);
	print_free_list();

	/*
    // Test allocation of 1 page
    struct page* p1 = buddy_alloc_page(0);
    utils_printf("Allocated page: %x\n", p1);

    // Test allocation of 2 pages (order 1)
    struct page* p2 = buddy_alloc_page(1);
    utils_printf("Allocated 2-page block: %x\n", p2);

    // Free first page
    buddy_free(page_to_phys(p1), 0);
    utils_printf("Freed page p1\n");

    // Free second block
    buddy_free(page_to_phys(p2), 1);
    utils_printf("Freed block p2\n");
	*/
}
