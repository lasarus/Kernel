#include "common.h"
#include "pipe.h"
#include "scheduler.h"
#include "vfs.h"

enum syscall_index {
	SYSCALL_READ = 0,
	SYSCALL_WRITE = 1,
	SYSCALL_OPEN = 2,
	SYSCALL_CLOSE = 3,
	SYSCALL_PIPE = 22,
	SYSCALL_DUP2 = 33,
	SYSCALL_NANOSLEEP = 35,
	SYSCALL_FORK = 57,
	SYSCALL_EXECVE = 59,
	SYSCALL_EXIT = 60,
	SYSCALL_WAIT4 = 61,
	SYSCALL_GETCWD = 79,
	SYSCALL_CHDIR = 80,
	SYSCALL_MKDIR = 83,
	SYSCALL_GETDENTS64 = 217,

	SYSCALL_MAXIMUM,
};

typedef uint64_t (*syscall_function_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
#define SYSCALL_MAX 256

uint64_t syscall_read(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	struct fd_table *fd_table = scheduler_get_fd_table();
	struct file *file = fd_table_get_file(fd_table, arg0);

	return vfs_read(file, (void *)arg1, arg2);
}

uint64_t syscall_write(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	struct fd_table *fd_table = scheduler_get_fd_table();
	struct file *file = fd_table_get_file(fd_table, arg0);

	return vfs_write(file, (void *)arg1, arg2);
}

uint64_t syscall_open(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	struct fd_table *fd_table = scheduler_get_fd_table();
	struct file *file = vfs_open(current_task->cwd, (const char *)arg0, arg1);
	if (!file)
		return -1;
	return fd_table_assign_open_file(fd_table, file);
}

uint64_t syscall_close(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	struct fd_table *fd_table = scheduler_get_fd_table();
	fd_table_close(fd_table, (int)arg0);
	return 0;
}

uint64_t syscall_pipe(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	struct fd_table *fd_table = scheduler_get_fd_table();
	int *fds = (int *)arg0;
	struct file *read = NULL, *write = NULL;
	create_pipe(&read, &write);

	if (!read || !write)
		return -1;

	fds[0] = fd_table_assign_open_file(fd_table, read);
	fds[1] = fd_table_assign_open_file(fd_table, write);

	return 0;
}

uint64_t syscall_dup2(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	int old_fd = arg0;
	int new_fd = arg1;
	struct fd_table *fd_table = scheduler_get_fd_table();
	fd_table_duplicate(fd_table, old_fd, new_fd);
	return 0;
}

uint64_t syscall_nanosleep(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	struct timespec *req = (void *)arg0;
	struct timespec *rem = (void *)arg1; // rem is ignored for now.
	scheduler_sleep(req->tv_sec); // TODO: Make this actually nanosleep.
	return 0;
}

uint64_t syscall_fork(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	return scheduler_fork();
}

uint64_t syscall_execve(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	return scheduler_execve((const char *)arg0, (const char *const *)arg1, (const char *const *)arg2);
}

uint64_t syscall_exit(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	scheduler_exit(arg0);
	return -1;
}

uint64_t syscall_wait4(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	return scheduler_wait_for_pid((int)arg0, (int *)arg1);
}

uint64_t syscall_getcwd(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	return get_path(current_task->cwd, (char *)arg0, arg1);
}

uint64_t syscall_chdir(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	struct path_node *new_cwd = vfs_resolve(current_task->cwd, (char *)arg0);
	if (!new_cwd)
		return -1;
	current_task->cwd = new_cwd;
	return 0;
}

uint64_t syscall_mkdir(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	return vfs_mkdir(current_task->cwd, (char *)arg0);
}

uint64_t syscall_getdents64(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4) {
	struct fd_table *fd_table = scheduler_get_fd_table();
	struct file *file = fd_table_get_file(fd_table, arg0);

	struct dirent *dirent = (void *)arg1;
	size_t size = arg2;

	return vfs_fill_dirent(file, dirent);
}

syscall_function_t syscall_table[SYSCALL_MAXIMUM] = {
	[SYSCALL_READ] = syscall_read,           [SYSCALL_WRITE] = syscall_write, [SYSCALL_OPEN] = syscall_open,
	[SYSCALL_CLOSE] = syscall_close,         [SYSCALL_PIPE] = syscall_pipe,   [SYSCALL_DUP2] = syscall_dup2,
	[SYSCALL_NANOSLEEP] = syscall_nanosleep, [SYSCALL_FORK] = syscall_fork,   [SYSCALL_EXECVE] = syscall_execve,
	[SYSCALL_EXIT] = syscall_exit,           [SYSCALL_WAIT4] = syscall_wait4, [SYSCALL_GETCWD] = syscall_getcwd,
	[SYSCALL_CHDIR] = syscall_chdir,         [SYSCALL_MKDIR] = syscall_mkdir, [SYSCALL_GETDENTS64] = syscall_getdents64,
};

// I'm trying to emulate Linux syscalls a little bit.
uint64_t syscall(uint64_t arg0, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t rax) {
	syscall_function_t syscall = NULL;

	if (rax < SYSCALL_MAXIMUM)
		syscall = syscall_table[rax];

	if (!syscall)
		return -1;

	return syscall(arg0, arg1, arg2, arg3, arg4);
}
