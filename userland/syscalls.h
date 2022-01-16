#ifndef SYSCALLS_H
#define SYSCALLS_H

#define NULL 0

typedef long time_t;

struct timespec {
	time_t tv_sec;
	long tv_nsec;
};

typedef unsigned long size_t;
void read(unsigned int fd, char *buf, size_t count);
void write(unsigned int fd, const char *buf, size_t count);
void nanosleep(struct timespec *req, struct timespec *rem);
void execve(const char *filename, const char *const *argv, const char *const *envp);
int fork(void);
void exit(int error_code);

#endif
