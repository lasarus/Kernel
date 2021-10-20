#include "common.h"
#include "vga_text.h"
#include "multiboot.h"
#include "memory.h"
#include "interrupts.h"

struct process {
	int alive;
	uint64_t pid;
	page_table_t pages;
};

static int processes_n = 0;
static struct process *process_current;
static struct process processes[16];

struct process *init_process(uint8_t *data, uint64_t size) {
	uint64_t idx = processes_n++;
	struct process *process = processes + idx;
	process->pid = 0x1234 + idx;
	process->pages = memory_new_page_table();
	process_current = process;

	memory_switch_page_table(process->pages);

	uint64_t virt_mem = 0x100000;
	for (uint64_t page = 0; page < size; page += 4096) {
		uint8_t *chunk = memory_alloc();

		for (unsigned i = 0; i < 4096; i++) {
			chunk[i] = data[page + i];
		}

		memory_page_add(virt_mem, chunk);
		virt_mem += 4096;
	}

	process->alive = 1;
	return process;
}

void kmain(uint32_t magic, struct multiboot *mb) {
	(void)magic;
	clear_screen();

	print("Kernel successfully booted into long mode.\n");

	memory_init(mb);
	print("Memory initialized.\n");

	interrupts_init();
	print("IDT initialized.\n");

	struct multiboot_module *modules = (void *)((uint64_t)mb->mods_addr + HIGHER_HALF_OFFSET);

	for (unsigned i = 0; i < mb->mods_count; i++) {
		uint8_t *data = (uint8_t *)(modules[i].mod_start + HIGHER_HALF_OFFSET);
		init_process(data, modules[i].mod_end - modules[i].mod_start);

		void (*func)(void) = (void (*)(void))0x100000;
		func();
	}

	page_table_t user1_table = memory_new_page_table();
	page_table_t user2_table = memory_new_page_table();
	memory_switch_page_table(user1_table);

	uint8_t *addr = (uint8_t *)0xBBBB10000;
	uint8_t *mem1 = memory_alloc();
	memory_page_add((uint64_t)addr, mem1);

	*addr = 10;

	memory_switch_page_table(user2_table);
	uint8_t *mem2 = memory_alloc();
	memory_page_add((uint64_t)addr, mem2);

	*addr = 40;

	print_int(*addr);
	memory_switch_page_table(user1_table);
	print_int(*addr);
}
