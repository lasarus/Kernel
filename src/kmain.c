#include "common.h"
#include "vga_text.h"
#include "multiboot.h"

#include "interrupts.h"

void kmain(uint32_t magic, struct multiboot *mb) {
	clear_screen();

	print("Kernel successfully booted into long mode.\n");
	int var = 1;

	interrupts_init();
	print("IDT initialized.\n");

	struct multiboot_module *modules = (void *)(uint64_t)mb->mods_addr;

	for (unsigned i = 0; i < mb->mods_count; i++) {
		for (char *it = (char *)(uint64_t)modules[i].mod_start;
			 it < (char *)(uint64_t)modules[i].mod_end; it++) {
			print_char(*it);
		}
	}
}
