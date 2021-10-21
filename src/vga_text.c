#include "vga_text.h"

#include "common.h"

#define DISPLAY ((uint8_t *)(0xB8000 + HIGHER_HALF_OFFSET))
static int x = 0, y = 0;

void update_cursor(void) {
	uint16_t pos = y * 80 + x;
 
	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t)(pos & 0xFF));
	outb(0x3D4, 0x0E);

	outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void clear_screen(void) {
	//(*display) = 'K';
	for (int i = 0; i < 25; i++) {
		for (int j = 0; j < 80; j++) {
			DISPLAY[(i * 80 + j) * 2] = ' ';
			DISPLAY[(i * 80 + j) * 2 + 1] = 0x0f;
		}
	}
}

void print_char(char c) {
	if (c == '\n') {
		x = 0;
		y++;
	} else {
		DISPLAY[(y * 80 + x) * 2] = c;
		DISPLAY[(y * 80 + x) * 2 + 1] = 0x0f;
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
					DISPLAY[(i * 80 + j) * 2] = ' ';
					DISPLAY[(i * 80 + j) * 2 + 1] = 0x0f;
				} else {
					DISPLAY[(i * 80 + j) * 2] = DISPLAY[((i + 1) * 80 + j) * 2];
				}
			}
		}
		y--;
	}
}

void print(const char *str) {
	for (; *str; str++)
		print_char(*str);

	update_cursor();
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
		print_char(buffer[idx]);
	}
	update_cursor();
}
