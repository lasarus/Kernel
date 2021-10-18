typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

static volatile uint8_t * const display = (void *)0xB8000;
static int x = 0, y = 0;

void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);

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

void print(const char *str) {
	for (; *str; str++) {
		if (*str == '\n') {
			x = 0;
			y++;
		} else {
			display[(y * 80 + x) * 2] = *str;
			x++;
			if (x > 80) {
				x = 0;
				y++;
			}
		}
	}

	update_cursor();
}

void kmain(void) {
	volatile uint8_t *display = (void *)0xB8000;

	clear_screen();
	print("Kernel successfully booted into long mode.\n");
}
