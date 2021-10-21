#include "common.h"
#include "vga_text.h"
#include "multiboot.h"
#include "memory.h"
#include "interrupts.h"
#include "taskswitch.h"
#include "scheduler.h"

void kmain(uint32_t magic, struct multiboot *mb) {
	(void)magic;
	clear_screen();

	print("Kernel successfully booted into long mode.\n");

	page_table_t kernel_table = memory_init(mb);
	print("Memory initialized.\n");

	interrupts_init();
	print("IDT initialized.\n");

	scheduler_init(kernel_table);

	struct multiboot_module *modules = (void *)((uint64_t)mb->mods_addr + HIGHER_HALF_OFFSET);
	for (unsigned i = 0; i < mb->mods_count; i++) {
		uint8_t *data = (uint8_t *)(modules[i].mod_start + HIGHER_HALF_OFFSET);
		scheduler_add_task(data, modules[i].mod_end - modules[i].mod_start);
	}

	print("End of kmain.\n");
}
