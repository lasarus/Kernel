#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include "common.h"
#include "memory.h"

int elf_loader_stage(struct pml4 *table, const char *path, uint64_t *rip);

#endif
