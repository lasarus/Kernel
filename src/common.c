#include "common.h"

uint64_t round_up_4096(uint64_t val) {
	uint64_t r = val & 0xfff;
	if (r)
		val = ((val & ~0xfff) + 0x1000);
	return val;
}

int strncmp(const char *s1, const char *s2, size_t n) {
	while (n && *s1 && (*s1 == *s2)) {
		s1++;
		s2++;
		n--;
	}
	return n == 0 ? 0 : *(unsigned char *)s1 - *(unsigned char *)s2; // UB?
}

size_t strlen(const char *s) {
	const char *start = s;
	for (; *s; s++)
		;
	return s - start;
}
