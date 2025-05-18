#include "stdio.h"
#include "syscalls.h"

typedef long ssize_t;
typedef unsigned long size_t;
typedef long off_t;

struct dirent {
	unsigned long d_ino;
	off_t d_off;
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[];
};

#define O_RDONLY 00

int main(int argc, char *argv[]) {
	int fd = open(argc > 1 ? argv[1] : ".", O_RDONLY);
	if (fd == -1)
		return 1;

	for (;;) {
		char buffer[1024];
		long nread = getdents64(fd, buffer, sizeof buffer);
		if (nread < -1)
			return 1;

		if (nread == 0)
			break;

		for (long bpos = 0; bpos < nread;) {
			struct dirent *d = (struct dirent *)(buffer + bpos);
			printf("%s\n", d->d_name);
			bpos += d->d_reclen;
		}
	}

	return 0;
}
