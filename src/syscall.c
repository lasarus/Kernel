#include "common.h"
#include "vga_text.h"
#include "scheduler.h"

// I'm trying to emulate Linux syscalls a little bit.
void syscall(uint64_t rax, uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	(void)arg0, (void)arg1, (void)arg2, (void)arg3, (void)arg4;
	switch (rax) {
	case 1: {
		char *buf = (char *)arg1;
		uint64_t len = arg2;

		for (uint64_t i = 0; i < len; i++) {
			print_char(buf[i]);
		}
	} break;

	default:
		print("Got unknown syscall: ");
		print_int(rax);
		print("\n");
	}

	scheduler_suspend();
}
