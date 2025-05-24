#ifndef INTERVAL_H
#define INTERVAL_H

#include "common.h"

int interval_exists(uint64_t low, uint64_t size);
void interval_add(uint64_t low, uint64_t size);
uint64_t interval_remove(uint64_t low);
uint64_t interval_size(uint64_t low);

#endif
