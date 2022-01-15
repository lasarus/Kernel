#include "syscalls.h"

int main(int argc, char **argv) {
	for (;;) {
		write(1, "Hello World\n", 12);
		struct timespec req = { .tv_sec = 1, .tv_nsec = 0 };
		nanosleep(&req, NULL);
	}
}
