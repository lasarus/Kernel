#ifndef SYSCALLS_H
#define SYSCALLS_H

enum {
	O_RDONLY = 1 << 0,
	O_WRONLY = 1 << 1,
	O_CREAT = 1 << 6,
	O_RDWR = O_RDONLY | O_WRONLY,
};

#define NULL 0
typedef long ssize_t;

typedef long time_t;

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

struct rusage; // Not implemented.

typedef unsigned long size_t;
ssize_t read(unsigned int fd, char *buf, size_t count);
void write(unsigned int fd, const char *buf, size_t count);
void nanosleep(struct timespec *req, struct timespec *rem);
void execve(const char *filename, const char *const *argv, const char *const *envp);
int fork(void);
void exit(int error_code);
ssize_t getdents64(int fd, void *dirp, size_t count);
int open(const char *pathname, int flags);
int close(int fd);
int wait4(int pid, int *wstatus, int options, struct rusage *rusage);
int getcwd(char *buffer, size_t size);
int chdir(const char *path);
int mkdir(const char *path);
int pipe(int *fds);
int dup2(int old_fd, int new_fd);

#endif
