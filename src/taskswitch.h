#ifndef TASK_SWITCH_H
#define TASK_SWITCH_H

#include "common.h"

void usermode_jump(uint64_t user_stack, uint64_t kernel_stack, uint64_t entry);

#endif
