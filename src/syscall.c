#include "common.h"
#include "pipe.h"
#include "scheduler.h"
#include "vfs.h"
#include "vga_text.h"

// I'm trying to emulate Linux syscalls a little bit.
uint64_t syscall(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t rax) {
	(void)arg0, (void)arg1, (void)arg2, (void)arg3, (void)arg4;
	switch (rax) {
	case 0: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		struct file *file = fd_table_get_file(fd_table, arg0);

		return vfs_read(file, (void *)arg1, arg2);
	} break;

	case 1: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		struct file *file = fd_table_get_file(fd_table, arg0);

		return vfs_write(file, (void *)arg1, arg2);
	} break;

	case 2: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		struct file *file = vfs_open(current_task->cwd, (const char *)arg0, arg1);
		if (!file)
			return -1;
		return fd_table_assign_open_file(fd_table, file);
	} break;

	case 3: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		fd_table_close(fd_table, (int)arg0);
		return 0;
	} break;

	case 22: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		int *fds = (int *)arg0;
		struct file *read = NULL, *write = NULL;
		create_pipe(&read, &write);

		if (!read || !write)
			return -1;

		fds[0] = fd_table_assign_open_file(fd_table, read);
		fds[1] = fd_table_assign_open_file(fd_table, write);

		return 0;
	} break;

	case 33: {
		int old_fd = arg0;
		int new_fd = arg1;
		struct fd_table *fd_table = scheduler_get_fd_table();
		fd_table_duplicate(fd_table, old_fd, new_fd);
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

	case 79: {
		return get_path(current_task->cwd, (char *)arg0, arg1);
	} break;

	case 80: {
		current_task->cwd = vfs_resolve(current_task->cwd, (char *)arg0);
		return 0;
	} break;

	case 83: {
		vfs_mkdir(current_task->cwd, (char *)arg0);
		return 0;
	} break;

	case 217: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		struct file *file = fd_table_get_file(fd_table, arg0);

		struct dirent *dirent = (void *)arg1;
		size_t size = arg2;

		return vfs_fill_dirent(file, dirent);
	} break;

	default: kprintf("Got unknown syscall: %d\n", (int)rax);
	}

	return 0;
}
