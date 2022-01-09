#include "common.h"
#include "vga_text.h"
#include "scheduler.h"

// I'm trying to emulate Linux syscalls a little bit.
void syscall(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t rax) {
	(void)arg0, (void)arg1, (void)arg2, (void)arg3, (void)arg4;
	switch (rax) {
	case 1: {
		struct fd_table *fd_table = scheduler_get_fd_table();
		int file = fd_table_get_file(fd_table, arg0);

		vfs_write_file(file, (void *)arg1, arg2);
	} break;

	case 35: {
		// Not really like linux, this just sleeps for the number of ticks determined by arg0.
		scheduler_sleep(arg0);
	} break;

	default:
		print("Got unknown syscall: ");
		print_int(rax);
		print("\n");
	}

	//scheduler_suspend();
}
