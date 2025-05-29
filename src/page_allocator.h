#ifndef PAGE_ALLOCATOR_H
#define PAGE_ALLOCATOR_H

#include "common.h"

void page_allocator_add_range(uintptr_t start, uintptr_t end);
void *allocate_page(void);
void free_page(void *physical);

#endif
