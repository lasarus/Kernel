#include "common.h"

uint64_t round_up_4096(uint64_t val) {
	int r = val & 0xfff;
	if (r)
		val = ((val & ~0xfff) + 0x1000);
	return val;
}

int strncmp(const char *s1, const char *s2, size_t n) {
	while (--n && *s1 && (*s1++ == *s2++));
	return n == 0 ? 0 : *s1 - *s2; // UB?
}
