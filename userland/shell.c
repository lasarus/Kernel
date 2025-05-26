#include "stdio.h"
#include "syscalls.h"

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

int atoi(const char *str) {
	int number = 0;
	for (size_t i = 0; str[i]; i++) {
		number *= 10;
		number += str[i] - '0';
	}
	return number;
}

const char *flush_whitespace(const char *ptr) {
	while (isspace(*ptr))
		ptr++;
	return ptr;
}

// Variables for holding the parsed command
static size_t buffer_size = 0;
static char buffer[1024];

static size_t parts_size = 0;
const char *parts[128];

void split_by_whitespace(const char *string) {
	string = flush_whitespace(string);
	buffer_size = 0;
	parts_size = 0;
	while (*string) {
		parts[parts_size++] = buffer + buffer_size;
		size_t i = 0;
		for (; string[i] && !isspace(string[i]); i++) {
			buffer[buffer_size++] = string[i];
		}
		string += i;
		buffer[buffer_size++] = '\0';

		string = flush_whitespace(string);
	}
}

int execute_builtins(void) {
	if (strcmp(parts[0], "cd") == 0) {
		if (parts_size >= 2)
			chdir(parts[1]);
		else
			chdir("/home/");
		return 1;
	}
	if (strcmp(parts[0], "exit") == 0) {
		if (parts_size >= 2)
			exit(atoi(parts[1]));
		exit(0);
	}
	return 0;
}

int execute(void) {
	char path[256] = "/bin/";
	int path_size = strlen(path);
	for (size_t i = 0; parts[0][i]; i++) {
		path[path_size++] = parts[0][i];
	}
	path[path_size++] = '\0';

	const char *pipe_to = NULL;

	int argv_size = 0;
	const char *argv[128];
	argv[argv_size++] = path;
	for (size_t i = 1; i < parts_size; i++) {
		if (parts[i][0] == '>')
			pipe_to = parts[i] + 1;
		else
			argv[argv_size++] = parts[i];
	}
	argv[argv_size++] = NULL;

	int pipe_fds[2];
	pipe(pipe_fds);

	int pid = fork();
	if (pid == -1) {
		printf("Could not fork?\n");
	} else if (pid == 0) {
		close(pipe_fds[0]);
		dup2(pipe_fds[1], 1);
		close(pipe_fds[1]);

		execve(path, argv, NULL);
		printf("Could not execute: \"%s\"\n", path);
		exit(1);
	} else {
		close(pipe_fds[1]);

		int output_fd = 1;
		if (pipe_to)
			output_fd = open(pipe_to, O_WRONLY | O_CREAT);
		char buffer[4096];
		ssize_t nread;
		while ((nread = read(pipe_fds[0], buffer, sizeof(buffer))) > 0) {
			write(output_fd, buffer, nread);
		}
		close(pipe_fds[0]);
		if (pipe_to)
			close(output_fd);

		int status;

		int waited = wait4(pid, &status, 0, NULL);
		if (waited == -1) {
			printf("Could not wait\n");
			exit(1);
		}

		return status;
	}
	return 1;
}

int parse_and_execute(const char *string) {
	split_by_whitespace(string);

	if (parts_size == 0)
		return 0;

	if (execute_builtins()) {
		return 0;
	}

	return execute();
}

int main(int argc, char **argv) {
	int exec_status = 0;
	for (;;) {
		char cwd_buffer[1024];
		getcwd(cwd_buffer, sizeof cwd_buffer);
		if (exec_status) {
			printf("%s %d $ ", cwd_buffer, exec_status);
		} else {
			printf("%s $ ", cwd_buffer);
		}
		char buffer[256];
		getsn(buffer, 256);

		int len = strlen(buffer);
		buffer[len - 1] = '\0';

		exec_status = parse_and_execute(buffer);
	}
}
