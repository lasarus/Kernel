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
	for (len = 0; *str; len++, str++)
		;
	return len;
}

int isspace(int c) {
	return c == ' ' || c == '\t';
}

void flush_whitespace(const char **ptr) {
	while (isspace(**ptr))
		(*ptr)++;
}

void execute(const char *command) {
	char path[256];
	const char *prefix = "/bin/";
	int i = 0;
	while (*prefix) {
		path[i++] = *prefix;
		prefix++;
	}

	flush_whitespace(&command);
	for (; *command && !isspace(*command); command++) {
		path[i++] = *command;
	}
	path[i] = '\0';

	const char *argv[128];
	char argv_buffer[256];
	int argv_buffer_size = 0;
	int argv_counter = 0;
	argv[argv_counter++] = path;

	const char *argv_start = argv_buffer;
	flush_whitespace(&command);
	while (*command) {
		argv[argv_counter++] = &argv_buffer[argv_buffer_size];

		while (*command && !isspace(*command)) {
			argv_buffer[argv_buffer_size++] = *command;
			command++;
		}
		argv_buffer[argv_buffer_size++] = '\0';

		flush_whitespace(&command);
	}

	argv[argv_counter++] = NULL;

	int pid = fork();
	if (pid) {
		execve(path, argv, NULL);
		puts("Could not execute: \"");
		puts(path);
		puts("\"\n");

		exit(1);
	}
}

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
