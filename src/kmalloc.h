#ifndef KMALLOC_H
#define KMALLOC_H

#include "common.h"

void kmalloc_init(void);

void *kaligned_alloc(size_t alignment, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t size);

#endif
