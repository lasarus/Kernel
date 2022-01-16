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
#include "keyboard.h"

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

	struct inode *error = root->create_child(root, "terminal_error", 14);
	terminal_red_init_inode(error);

	struct inode *keyboard = root->create_child(root, "keyboard", 8);
	keyboard_init_inode(keyboard);

	int stdin = -1, stdout = -1, stderr = -1;
	stdin = vfs_open("/keyboard", O_RDONLY);
	stdout = vfs_open("/terminal", O_WRONLY);
	stderr = vfs_open("/terminal_error", O_WRONLY);
	(void)stdin, (void)stdout, (void)stderr;

	struct inode *bin = root->create_child(root, "bin", 3);
	tmpfs_init_dir(bin);

	struct multiboot_module *modules = (void *)((uint64_t)mb->mods_addr + HIGHER_HALF_OFFSET);

	static const char *names[] = { "init", "hello", "p2" };
	for (unsigned i = 0; i < mb->mods_count; i++) {
		uint8_t *data = (uint8_t *)(modules[i].mod_start + HIGHER_HALF_OFFSET);

		struct inode *file = bin->create_child(bin, names[i], strlen(names[i]));
		tmpfs_init_file(file, data, modules[i].mod_end - modules[i].mod_start);
	}

	scheduler_init(kernel_table);

	int init_pid = scheduler_fork();
	if (init_pid == 0) {
		/* Set up stdin/stdout/stderr. */
		/* We are in the init process now. */
		fd_table_set_standard_streams(current_task->fd_table, stdin, stdout, stderr);
		scheduler_execve("/bin/init", NULL, NULL);
	}

	print("End of kmain.\n");
}
