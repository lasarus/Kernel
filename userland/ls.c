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

void print_contents(const char *path, int show_hidden) {
	int fd = open(path, O_RDONLY);

	if (fd == -1)
		exit(1);

	char buffer[1024];
	for (;;) {
		long nread = getdents64(fd, buffer, sizeof buffer);
		if (nread < -1)
			exit(1);

		if (nread == 0)
			break;

		for (long bpos = 0; bpos < nread;) {
			struct dirent *d = (struct dirent *)(buffer + bpos);
			if (d->d_name[0] != '.' || show_hidden)
				printf("%s\n", d->d_name);
			bpos += d->d_reclen;
		}
	}
	close(fd);
}

int main(int argc, char *argv[]) {
	int show_hidden = 0;
	int n_directories = 0;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-' && argv[i][1] == 'a' && argv[i][2] == '\0') {
			show_hidden = 1;
		} else {
			n_directories++;
		}
	}

	if (n_directories == 0) {
		print_contents(".", show_hidden);
		return 0;
	}

	int first_directory = 1;
	for (int i = 1; i < argc; i++) {
		if (argv[i][0] == '-')
			continue;
		if (n_directories > 1) {
			if (!first_directory)
				printf("\n");
			printf("%s/:\n", argv[i]);
		}
		print_contents(argv[i], show_hidden);
		first_directory = 0;
	}

	return 0;
}
