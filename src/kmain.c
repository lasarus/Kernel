#include "common.h"
#include "vga_text.h"
#include "multiboot.h"
#include "memory.h"
#include "interrupts.h"
#include "taskswitch.h"

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

	uint8_t *virt_mem = (void *)0x100000;
	memory_allocate_range((uint64_t)virt_mem, size, 1);
	for (unsigned i = 0; i < size; i++)
		virt_mem[i] = data[i];

	// Set up kernel stack. Probably only needs one page for now.
	// Set up straight below the 1GiB upper segment of memory.
	// Or maybe not? It is probably better to have the stack in upper half? right?
	// Below 1GiB would be upper half anyways.

	const uint64_t kernel_stack_size = 4096 * 4;
	const uint64_t kernel_stack_pos = HIGHER_HALF_OFFSET - kernel_stack_size * 1;
	memory_allocate_range(kernel_stack_pos, kernel_stack_size, 0);

	// Set up user stack.
	const uint64_t user_stack_size = 4 * MEBIBYTE;
	const uint64_t user_stack_pos = GIBIBYTE;
	memory_allocate_range(user_stack_pos, user_stack_size, 1);

	process->alive = 1;

	set_kernel_stack(kernel_stack_pos + kernel_stack_size - 128); // Stack grows downwards.
	usermode_jump(user_stack_pos, kernel_stack_pos, 0x100000);

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

		//usermode_jump(
		//((void (*)(void))0x100000)();
	}

	print("End of kmain.\n");
}
