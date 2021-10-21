#ifndef MEMORY_H
#define MEMORY_H

#include "multiboot.h"

// Returns pointer to beginning of a free 4KiB chunk of memory.
void *memory_alloc(void);
void memory_free(void *ptr);
void memory_init(struct multiboot *mb);

typedef int page_table_t;

page_table_t memory_new_page_table(void);
void memory_switch_page_table(page_table_t table);
void memory_page_add(uint64_t virtual_addr, void *high_addr, int user);
void memory_allocate_range(uint64_t base, uint64_t size, int user);

void set_kernel_stack(uint64_t stack);

#endif
