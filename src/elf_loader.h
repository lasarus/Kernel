#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include "common.h"
#include "memory.h"

void elf_loader_load(page_table_t table, uint8_t *data, uint64_t size, uint64_t *rip);

#endif
