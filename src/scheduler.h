#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"
#include "memory.h"
#include "vfs.h"

void scheduler_add_task(uint8_t *data, uint64_t size, int stdin, int stdout, int stderr);
void scheduler_update(void);
void scheduler_init(page_table_t kernel_table);

void scheduler_suspend(void);
void scheduler_sleep(uint64_t ticks);

struct fd_table *scheduler_get_fd_table(void);

#endif
