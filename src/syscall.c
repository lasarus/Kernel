#include "common.h"
#include "scheduler.h"
#include "vfs.h"
#include "vga_text.h"

// I'm trying to emulate Linux syscalls a little bit.
uint64_t syscall(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t rax) {
	(void)arg0, (void)arg1, (void)arg2, (void)arg3, (void)arg4;
	switch (rax) {
	case 0: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		int file = fd_table_get_file(fd_table, arg0);

		return vfs_read_file(file, (void *)arg1, arg2);
	} break;

	case 1: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		int file = fd_table_get_file(fd_table, arg0);

		return vfs_write_file(file, (void *)arg1, arg2);
	} break;

	case 2: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		int vfs_fd = vfs_open((const char *)arg0, arg1);
		if (vfs_fd == -1)
			return -1;
		return fd_table_assign_open_file(fd_table, vfs_fd);
	} break;

	case 3: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		int file = fd_table_get_file(fd_table, arg0);
		vfs_close_file(file);
		return 0;
	} break;

	case 35: {
		struct timespec *req = (void *)arg0;
		struct timespec *rem = (void *)arg1; // rem is ignored for now.
		scheduler_sleep(req->tv_sec); // TODO: Make this actually nanosleep.
	} break;

	case 57: {
		return scheduler_fork();
	} break;

	case 59: {
		return scheduler_execve((const char *)arg0, (const char *const *)arg1, (const char *const *)arg2);
	} break;

	case 60: {
		scheduler_exit(arg0);
	} break;

	case 61: {
		return scheduler_wait_for_pid((int)arg0, (int *)arg1);
	} break;

	case 217: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		int file = fd_table_get_file(fd_table, arg0);

		struct dirent *dirent = (void *)arg1;
		size_t size = arg2;

		return vfs_fill_dirent(file, dirent);
	} break;

	default: kprintf("Got unknown syscall: %d\n", (int)rax);
	}

	return 0;
}
