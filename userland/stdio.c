#include "stdio.h"
#include "syscalls.h"
#include <stdarg.h>

int getchar() {
	char c;
	read(0, &c, 1);
	return c;
}

int putchar(int c) {
	char cc = c;
	write(1, &cc, 1);
	return cc;
}

void puts(const char *str) {
	while (*str)
		putchar(*str++);
	putchar('\n');
}

static void print_string(const char *str) {
	while (*str)
		putchar(*str++);
}

static void print_int(int num) {
	char buffer[11];
	int idx = 0;
	for (; num; num /= 10)
		buffer[idx++] = (num % 10) + '0';

	if (idx == 0) {
		buffer[0] = '0';
		idx++;
	}

	for (idx--; idx >= 0; idx--) {
		putchar(buffer[idx]);
	}
}

static void print_hex(int num) {
	char buffer[16];
	int idx = 0;
	for (; num; num /= 16) {
		int digit = num % 16;
		if (digit < 10) {
			buffer[idx++] = digit + '0';
		} else {
			buffer[idx++] = (digit - 10) + 'A';
		}
	}

	if (idx == 0) {
		buffer[0] = '0';
		idx++;
	}

	for (idx--; idx >= 0; idx--) {
		putchar(buffer[idx]);
	}
}

void printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	for (; *fmt; fmt++) {
		if (*fmt == '%') {
			switch (*++fmt) {
			case 's': print_string(va_arg(args, const char *)); break;
			case 'd': print_int(va_arg(args, int)); break;
			case 'x': print_hex(va_arg(args, int)); break;
			case 'c': putchar(va_arg(args, int)); break;
			default: break;
			}
		} else {
			putchar(*fmt);
		}
	}

	va_end(args);
}
