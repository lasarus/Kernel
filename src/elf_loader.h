#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include "common.h"
#include "memory.h"

#define ELF_STAGE_INDEX 509
#define ELF_STAGE_OFFSET PML4_PAGE(ELF_STAGE_INDEX)

int elf_loader_stage(page_table_t table, const char *path, uint64_t *rip);

#endif
