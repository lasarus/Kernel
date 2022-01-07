#ifndef MEMORY_H
#define MEMORY_H

#include "multiboot.h"

// Returns pointer to beginning of a free 4KiB chunk of memory.
typedef int page_table_t;

void *memory_alloc(void);
void memory_free(void *ptr);
page_table_t memory_init(struct multiboot *mb);

page_table_t memory_new_page_table(void);
void memory_page_add(page_table_t table, uint64_t virtual_addr, void *high_addr, int user);
void memory_allocate_range(page_table_t table, uint64_t base, uint8_t *data, uint64_t size, int user);
uint64_t memory_get_cr3(page_table_t table);

void set_kernel_stack(uint64_t stack);

#define PML4_PAGE(INDEX) (0xffff000000000000 | ((unsigned long long)(INDEX) << (9 * 3 + 12)))

#define KERNEL_STACK_POS PML4_PAGE(510)

#endif
