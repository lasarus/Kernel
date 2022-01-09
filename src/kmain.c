#include "common.h"
#include "vga_text.h"
#include "multiboot.h"
#include "memory.h"
#include "interrupts.h"
#include "taskswitch.h"
#include "scheduler.h"
#include "vfs.h"
#include "tmpfs.h"
#include "terminal.h"
#include "syscall.h"

void kmain(uint32_t magic, struct multiboot *mb) {
	(void)magic;
	clear_screen();

	print("Kernel successfully booted into long mode.\n");

	page_table_t kernel_table = memory_init(mb);
	print("Memory initialized.\n");

	interrupts_init();
	print("IDT initialized.\n");

	syscall_init();
	print("Syscall initialized.\n");

	vfs_init();
	print("VFS initialized.\n");

	struct inode *root = vfs_resolve(NULL, "/");
	tmpfs_init_dir(root);

	struct inode *terminal = root->create_child(root, "terminal", 8);
	terminal_init_inode(terminal);

	struct inode *error = root->create_child(root, "terminal_error", 8);
	terminal_red_init_inode(error);

	int stdin = -1, stdout = -1, stderr = -1;
	stdout = vfs_open_file(terminal, O_WRONLY);
	stderr = vfs_open_file(error, O_WRONLY);

	scheduler_init(kernel_table);

	struct multiboot_module *modules = (void *)((uint64_t)mb->mods_addr + HIGHER_HALF_OFFSET);
	for (unsigned i = 0; i < mb->mods_count; i++) {
		uint8_t *data = (uint8_t *)(modules[i].mod_start + HIGHER_HALF_OFFSET);
		scheduler_add_task(data, modules[i].mod_end - modules[i].mod_start, stdin, stdout, stderr);
	}

	print("End of kmain.\n");
}
