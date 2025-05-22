#include "common.h"
#include "driver_major_numbers.h"
#include "interrupts.h"
#include "keyboard.h"
#include "kmalloc.h"
#include "memory.h"
#include "multiboot.h"
#include "scheduler.h"
#include "syscall.h"
#include "terminal.h"
#include "tmpfs.h"
#include "vfs.h"
#include "vga_text.h"

void kmain(uint32_t magic, struct multiboot *mb) {
	(void)magic;
	clear_screen();

	struct pml4 *kernel_table = memory_init(mb);
	interrupts_init();
	kmalloc_init();
	syscall_init();
	vfs_init();
	keyboard_init();
	terminal_init();

	struct path_node *root = vfs_resolve(NULL, "/");
	tmpfs_init(root->inode);

	vfs_mknod(root, "terminal", INODE_CHAR_DEVICE, MAJOR_TERMINAL, TERMINAL_MINOR_WHITE);
	vfs_mknod(root, "terminal_error", INODE_CHAR_DEVICE, MAJOR_TERMINAL, TERMINAL_MINOR_RED);
	vfs_mknod(root, "keyboard", INODE_CHAR_DEVICE, MAJOR_KEYBOARD, 0);

	vfs_mkdir(root, "bin");
	vfs_mkdir(root, "home");

	struct multiboot_module *modules = (void *)((uint64_t)mb->mods_addr + HIGHER_HALF_OFFSET);

	static const char *names[] = { "/bin/init", "/bin/hello", "/bin/echo", "/bin/cat", "/bin/ls", "/home/README.md" };
	for (unsigned i = 0; i < mb->mods_count; i++) {
		uint8_t *data = (uint8_t *)(modules[i].mod_start + HIGHER_HALF_OFFSET);
		struct file *file = vfs_open(NULL, names[i], O_WRONLY | O_CREAT);
		vfs_write(file, data, modules[i].mod_end - modules[i].mod_start);
		vfs_close_file(file);
	}

	scheduler_init(kernel_table);

	int init_pid = scheduler_fork();
	if (init_pid == 0) {
		/* We are in the init process now. */
		current_task->cwd = vfs_resolve(NULL, "/home/");

		struct file *stdin = vfs_open(NULL, "/keyboard", O_RDONLY), *stdout = vfs_open(NULL, "/terminal", O_WRONLY),
					*stderr = vfs_open(NULL, "/terminal_error", O_WRONLY);

		fd_table_set_standard_streams(current_task->fd_table, stdin, stdout, stderr);

		scheduler_execve("/bin/init", NULL, NULL);

		ERROR("Error starting init process.\n");
	}
}
