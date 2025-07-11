#include "common.h"

uintptr_t round_down_4096(uintptr_t address) {
	return address & ~(uintptr_t)(4096 - 1);
}

uintptr_t round_up_4096(uintptr_t address) {
	return (address + 4095) & ~(uintptr_t)(4096 - 1);
}

int strcmp(const char *s1, const char *s2) {
	size_t i = 0;
	for (; s1[i] && s2[i] && s1[i] == s2[i]; i++)
		;

	if (s1[i] == '\0' && s2[i] == '\0')
		return 0;
	return s1[i] - s2[i];
}

size_t strlen(const char *s) {
	const char *start = s;
	for (; *s; s++)
		;
	return s - start;
}

void *memcpy(void *dest, const void *src, size_t n) {
	uint8_t *dest_8 = dest;
	const uint8_t *src_8 = src;
	for (size_t i = 0; i < n; i++) {
		dest_8[i] = src_8[i];
	}
	return dest;
}

char *strcpy(char *dest, const char *src) {
	size_t i = 0;
	for (; src[i]; i++) {
		dest[i] = src[i];
	}
	dest[i] = '\0';
	return dest;
}

void *memset(void *source, int c, size_t n) {
	for (size_t i = 0; i < n; i++) {
		((uint8_t *)source)[i] = c;
	}
	return source;
}
