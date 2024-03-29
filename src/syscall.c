#include "common.h"
#include "scheduler.h"
#include "vga_text.h"

// I'm trying to emulate Linux syscalls a little bit.
uint64_t syscall(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t rax) {
	(void)arg0, (void)arg1, (void)arg2, (void)arg3, (void)arg4;
	switch (rax) {
	case 0: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		int file = fd_table_get_file(fd_table, arg0);

		vfs_read_file(file, (void *)arg1, arg2);
	} break;

	case 1: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		int file = fd_table_get_file(fd_table, arg0);

		vfs_write_file(file, (void *)arg1, arg2);
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

	default:
		print("Got unknown syscall: ");
		print_int(rax);
		print("\n");
	}

	return 0;
}
