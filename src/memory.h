#ifndef MEMORY_H
#define MEMORY_H

#include "multiboot.h"

// Returns pointer to beginning of a free 4KiB chunk of memory.
void *memory_alloc(void);
void memory_free(void *ptr);
struct pml4 *memory_init(struct multiboot *mb);

struct pml4 *memory_new_page_table(void);
struct pml4 *memory_page_table_copy(struct pml4 *old);
void memory_page_table_delete(struct pml4 *old, int only_user);

void memory_page_table_delete_pdpt(struct pml4 *old, int idx);
void memory_page_table_move_pdpt(struct pml4 *old, int dest, int src);

void memory_page_add(struct pml4 *table, uint64_t virtual_addr, void *high_addr, int user);
void memory_allocate_range(struct pml4 *table, uint64_t base, uint8_t *data, uint64_t size, int user);

void set_kernel_stack(uint64_t stack);

#define PML4_PAGE(INDEX) (0xffff000000000000 | ((unsigned long long)(INDEX) << (9 * 3 + 12)))

// Modifying these also requires modifying syscall_handler.s.
#define KERNEL_STACK_SIZE (8ull * KIBIBYTE)
#define KERNEL_STACK_POS PML4_PAGE(510)

#define PHYSICAL_TO_VIRTUAL(X) ((void *)((uintptr_t)(X) + HIGHER_HALF_IDENTITY))
#define VIRTUAL_TO_PHYSICAL(X) ((uintptr_t)(X) - HIGHER_HALF_IDENTITY)

#endif
