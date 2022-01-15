#ifndef TASK_SWITCH_H
#define TASK_SWITCH_H

#include "common.h"
#include "scheduler.h"

void switch_task_to(struct task *task);
void usermode_jump(uint64_t user_stack, uint64_t entry, uint64_t rdi, uint64_t rsi, uint64_t rdx);

#endif
