#include "syscalls.h"

int getchar() {
	char c;
	read(0, &c, 1);
	return c;
}

int putchar(int c) {
	char cc = c;
	write(1, &cc, 1);
	return cc;
}

void puts(const char *str) {
	while (*str)
		putchar(*str++);
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

int strcmp(const char *s1, const char *s2) {
	while (*s1 && *s1 == *s2) {
		s1++;
		s2++;
	}
	return *s1 - *s2;
}

int strlen(const char *str) {
	int len;
	for (len = 0; *str; len++, str++);
	return len;
}

void execute(const char *path) {
	char buff[256] = "/bin/";

	int i = 5;
	for (; path[i - 5]; i++) {
		buff[i] = path[i - 5];
	}
	buff[i] = '\0';

	int pid = fork();
	if (pid) {
		execve(buff, NULL, NULL);
	}
}

char str[] = "Hello World!\n";

int main(int argc, char **argv) {

	for (;;) {
		write(1, "$ ", 2);
		char buffer[256];
		getsn(buffer, 256);

		int len = strlen(buffer);
		buffer[len - 1] = '\0';

		if (strcmp("exit", buffer) == 0) {
			exit(1);
		} else {
			execute(buffer);
		}
	}
}
