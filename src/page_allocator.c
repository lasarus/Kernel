#include "page_allocator.h"
#include "common.h"
#include "memory.h"

#include "vga_text.h"

struct memory_list {
	struct memory_list *next;
	uint64_t n_entries;
	void *entries[510];
};

_Static_assert(sizeof(struct memory_list) == PAGE_SIZE, "Invalid size of memory_list.");

_Alignas(PAGE_SIZE) struct memory_list last_elem;
static struct memory_list *head = &last_elem;

void page_allocator_add_range(uintptr_t start, uintptr_t end) {
	start = round_up_4096(start);
	end = round_down_4096(end);

	for (; start < end; start += PAGE_SIZE)
		free_page(PHYSICAL_TO_VIRTUAL(start));
}

void *allocate_page(void) {
	if (head->n_entries == 0) {
		void *chunk = head;
		head = head->next;
		return chunk;
	}
	head->n_entries--;
	return head->entries[head->n_entries];
}

void free_page(void *physical) {
	// Assume ptr is not present in the memory list.
	if (head->n_entries == 510) {
		struct memory_list *prev_head = head;
		head = physical;
		head->next = prev_head;
		head->n_entries = 0;
	} else {
		head->entries[head->n_entries++] = physical;
	}
}
