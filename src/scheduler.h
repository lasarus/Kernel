#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"
#include "memory.h"

void scheduler_add_task(uint8_t *data, uint64_t size);
void scheduler_update(void);
void scheduler_init(page_table_t kernel_table);

void scheduler_suspend(void);

#endif
