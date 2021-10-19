#include "vga_text.h"

#include "common.h"

static volatile uint8_t * const display = (void *)0xB8000;
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
			display[(i * 80 + j) * 2] = ' ';
			display[(i * 80 + j) * 2 + 1] = 0x0f;
		}
	}
}

void print_char(char c) {
	if (c == '\n') {
		x = 0;
		y++;
	} else {
		display[(y * 80 + x) * 2] = c;
		display[(y * 80 + x) * 2 + 1] = 0x0f;
		x++;
		if (x > 80) {
			x = 0;
			y++;
		}
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
}
