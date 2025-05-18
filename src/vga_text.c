#include "vga_text.h"

#include "common.h"

#include <stdarg.h>

#define DISPLAY ((uint8_t *)(0xB8000 + HIGHER_HALF_OFFSET))
static int x = 0, y = 0;

static void update_cursor(void) {
	uint16_t pos = y * 80 + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));
	outb(0x3D4, 0x0E);

	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clear_screen(void) {
	for (int i = 0; i < 25; i++) {
		for (int j = 0; j < 80; j++) {
			DISPLAY[(i * 80 + j) * 2 + 0] = ' ';
			DISPLAY[(i * 80 + j) * 2 + 1] = 0x0f;
		}
	}
}

void print_char_color(char c, unsigned char color) {
	if (c == '\n') {
		x = 0;
		y++;
	} else {
		DISPLAY[(y * 80 + x) * 2 + 0] = c;
		DISPLAY[(y * 80 + x) * 2 + 1] = color;
		x++;
		if (x >= 80) {
			x = 0;
			y++;
		}
	}

	if (y >= 24) {
		// Move everything up one step.
		for (int i = 0; i < 25; i++) {
			for (int j = 0; j < 80; j++) {
				if (i == 24) {
					DISPLAY[(i * 80 + j) * 2 + 0] = ' ';
					DISPLAY[(i * 80 + j) * 2 + 1] = 0x0f;
				} else {
					DISPLAY[(i * 80 + j) * 2 + 0] = DISPLAY[((i + 1) * 80 + j) * 2 + 0];
					DISPLAY[(i * 80 + j) * 2 + 1] = DISPLAY[((i + 1) * 80 + j) * 2 + 1];
				}
			}
		}
		y--;
	}
}

void print_char(char c) {
	if (c == '\b') {
		if (x == 0) {
			if (y > 0)
				y--;
			x = 80;
		}
		x--;
		DISPLAY[(y * 80 + x) * 2 + 0] = ' ';
		DISPLAY[(y * 80 + x) * 2 + 1] = 0x0f;
	} else {
		print_char_color(c, 0x0f);
	}

	update_cursor();
}

void print(const char *str) {
	for (; *str; str++)
		print_char(*str);
}

void print_int(int num) {
	char buffer[11];
	int idx = 0;
	for (; num; num /= 10)
		buffer[idx++] = (num % 10) + '0';

	if (idx == 0) {
		buffer[0] = '0';
		idx++;
	}

	for (idx--; idx >= 0; idx--) {
		print_char(buffer[idx]);
	}
	update_cursor();
}

void print_hex(uint64_t num) {
	char buffer[16];
	int idx = 0;
	for (; num; num /= 16) {
		uint64_t digit = num % 16;
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
		print_char(buffer[idx]);
	}
	update_cursor();
}

void kprintf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	for (; *fmt; fmt++) {
		if (*fmt == '%') {
			switch (*++fmt) {
			case 's': print(va_arg(args, const char *)); break;
			case 'd': print_int(va_arg(args, int)); break;
			case 'x': print_hex(va_arg(args, int)); break;
			case 'c': print_char(va_arg(args, int)); break;
			default: break;
			}
		} else {
			print_char(*fmt);
		}
	}

	va_end(args);
}
