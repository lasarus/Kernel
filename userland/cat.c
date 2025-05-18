#include "stdio.h"
#include "syscalls.h"

#define O_RDONLY 00

void print_file(const char *path) {
	int fd = open(path, O_RDONLY);
	if (fd == -1)
		exit(1);

	char buffer[1024];
	for (;;) {
		long nread = read(fd, buffer, sizeof buffer);

		if (nread < -1)
			exit(1);
		if (nread == 0)
			break;
		write(1, buffer, nread);
	}

	close(fd);
}

int main(int argc, char *argv[]) {
	for (int i = 1; argv[i]; i++) {
		print_file(argv[i]);
	}
	return 0;
}
