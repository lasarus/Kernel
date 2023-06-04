#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "common.h"
#include "memory.h"
#include "vfs.h"

extern struct task *current_task;

enum {
	STATUS_RUNNING = 0,
	STATUS_CLOCK_SLEEP,
	STATUS_UNLIMITED_SLEEP,
	STATUS_CLEANUP,
	STATUS_DEAD,
};

struct task {
	uint64_t rip, gp_regs[16];
	uint64_t cr3;
	uint8_t is_usermode;

	uint8_t status;
	uint64_t sleep_until;
	// TODO: XMM0-15

	page_table_t pages;
	int pid;
	int parent;

	struct fd_table *fd_table;
};

struct task_wait {
	int pid;
};

#define TASK_WAIT_DEFAULT \
	{ .pid = -1 }

// Returns 0 if not able to wait.
int scheduler_wait(struct task_wait *wait);
void scheduler_unwait(struct task_wait *wait);

void scheduler_add_task(const char *path, int stdin, int stdout, int stderr);
void scheduler_update(void);
void scheduler_init(page_table_t kernel_table);
int scheduler_execve(const char *filename, const char *const *argv, const char *const *envp);

void scheduler_suspend(void);
void scheduler_sleep(uint64_t ticks);
int scheduler_fork(void);
void scheduler_exit(int error_code);

struct fd_table *scheduler_get_fd_table(void);

#endif
