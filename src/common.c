#include "common.h"

uint64_t round_up_4096(uint64_t val) {
	int r = val & 0xfff;
	if (r)
		val = ((val & ~0xfff) + 0x1000);
	return val;
}
