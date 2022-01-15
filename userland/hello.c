#include "syscalls.h"

int getchar() {
	char c;
	read(0, &c, 1);
	return c;
}

void getsn(char *str, int n) {
	while (n-- > 1) {
		char c = getchar();
		*str++ = c;
		if (c == '\n') {
			*str++ = '\0';
			break;
		}
	}
}

int main(int argc, char **argv) {
	write(1, "> ", 2);
	char buffer[256];
	getsn(buffer, 256);

	for (int i = 0; i < 256 && buffer[i]; i++) {
		write(1, &buffer[i], 1);
	}

	write(1, "Hello!\n", 7);

	for (;;) {
		struct timespec req = { .tv_sec = 1, .tv_nsec = 0 };
		nanosleep(&req, NULL);
	}
}
