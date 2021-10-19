#include "common.h"
#include "vga_text.h"

#include "interrupts.h"

void kmain(void) {

	clear_screen();
	print("Kernel successfully booted into long mode.\n");

	interrupts_init();
	print("IDT initialized.\n");

	for (unsigned int i = 0;;i++) {
		volatile uint8_t *display = (void *)0xB8000;
		display[0] = "|/-\\"[i % 5];
	}
}
