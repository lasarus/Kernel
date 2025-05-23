#include "scheduler.h"
#include "common.h"
#include "elf_loader.h"
#include "interrupts.h"
#include "memory.h"
#include "taskswitch.h"
#include "vfs.h"
#include "vga_text.h"

#define MAX_TASKS 16
#define MAX_ARGC 128
#define MAX_ARGV_CONTENTS 2048

int n_tasks = 0;
struct task tasks[MAX_TASKS];
struct task *current_task;

static int pid_counter = 0;

void disasm(uint8_t *addr, int len);

void *push_to_stack(void **rsp, const void *src, size_t size) {
	*(uint8_t **)rsp -= size;
	*rsp = (void *)((uint64_t)*rsp & -16); // Align rsp to 16 byte boundary.
	memcpy(*(uint8_t **)rsp, src, size);
	return *rsp;
}

static size_t staged_argv_size = 0;
static char staged_argv_buffer[MAX_ARGV_CONTENTS];
static size_t staged_argc = 0;
static size_t staged_argv[MAX_ARGC]; // Offsets into `staged_argv_buffer`.

void stage_argv(const char *const *argv) {
	staged_argc = 0;
	staged_argv_size = 0;

	if (!argv)
		return;

	for (; argv[staged_argc] && staged_argc < MAX_ARGC; staged_argc++) {
		int i = 0;
		staged_argv[staged_argc] = staged_argv_size;
		for (; argv[staged_argc][i]; i++) {
			staged_argv_buffer[staged_argv_size++] = argv[staged_argc][i];
		}
		staged_argv_buffer[staged_argv_size++] = '\0';
	}
}

int scheduler_execve(const char *filename, const char *const *argv, const char *const *envp) {
	uint64_t rip;
	if (elf_loader_stage(current_task->pages, filename, &rip)) {
		memory_page_table_delete_pdpt(current_task->pages, ELF_STAGE_INDEX);
		return 1;
	}

	stage_argv(argv); // Needs to be done before destroying old pages.

	// We are now commited to a new executable. The old one can be thrown away.
	memory_page_table_delete(current_task->pages, 1);
	memory_page_table_move_pdpt(current_task->pages, 0, ELF_STAGE_INDEX);

	const uint64_t user_stack_size = 4 * 4UL * KIBIBYTE;
	const uint64_t user_stack_pos = GIBIBYTE;

	// Set up user stack.
	memory_allocate_range(current_task->pages, user_stack_pos, NULL, user_stack_size, 1);

	void *rsp = (void *)(user_stack_size + user_stack_pos);

	// Set up argc and argv.
	struct stack {
		uint64_t argc;
		char *argv[MAX_ARGC];
	} __attribute__((packed)) stack;

	if (argv) {
		void *staged_argv_buffer_offset = push_to_stack(&rsp, staged_argv_buffer, staged_argv_size);
		for (unsigned i = 0; i < staged_argc; i++) {
			stack.argv[i] = (char *)staged_argv_buffer_offset + staged_argv[i];
		}

		stack.argv[staged_argc] = NULL;
	}

	stack.argc = staged_argc;

	push_to_stack(&rsp, &stack, sizeof(uint64_t) + (staged_argc + 1) * sizeof(char *));

	// Jump to usermode. %rdi, %rsi, %rdx are not used.
	usermode_jump((uint64_t)rsp, rip, 0, 0, 0);
	return 0;
}

int get_next_task(void) {
	static int prev_task = 0;

	int next_task = prev_task;

	for (int i = 0; i < n_tasks; i++) {
		next_task++;
		if (next_task >= n_tasks)
			next_task = 0;

		int resume = 0;
		switch (tasks[next_task].status) {
		case STATUS_CLOCK_SLEEP:
			if ((tasks[next_task].sleep_until < timer) || (tasks[next_task].sleep_until > (timer - UINT64_MAX / 2))) {
				tasks[next_task].status = STATUS_RUNNING;
				resume = 1;
			}
			break;

		case STATUS_RUNNING: resume = 1; break;

		case STATUS_WAIT_FOR_PID: resume = 0; break;

		default: break;
		}

		if (resume)
			break;
	}

	prev_task = next_task;
	return next_task;
}

struct task *new_task(void) {
	// First try to find a dead task to replace.
	int idx = 0;
	for (; idx < n_tasks && tasks[idx].status != STATUS_DEAD; idx++)
		;

	// If none found, add to end.
	if (idx == n_tasks)
		n_tasks++;

	if (n_tasks >= MAX_TASKS)
		ERROR("Too many concurrent tasks.");

	tasks[idx] = (struct task) {
		.status = STATUS_UNLIMITED_SLEEP,
	};

	return &tasks[idx];
}

void scheduler_init(struct pml4 *kernel_table) {
	current_task = new_task();
	*current_task = (struct task) {
		.pages = kernel_table,
		.fd_table = vfs_init_fd_table(),
		.pid = pid_counter++,
	};

	set_kernel_stack(KERNEL_STACK_POS + KERNEL_STACK_SIZE);
}

// Scheduling is about fairly distributing CPU time among the tasks.
void scheduler_update(void) {
	int next_task = get_next_task();

	for (int i = 0; i < n_tasks; i++) {
		if (tasks[i].status == STATUS_CLEANUP && current_task - tasks != i) {
			tasks[i].status = STATUS_DEAD;
			memory_page_table_delete(tasks[i].pages, 0);
		}
	}

	struct task *task = tasks + next_task;

	switch_task_to(task);
}

void scheduler_suspend(void) {
	switch_task_to(tasks + 0);
}

void scheduler_sleep(uint64_t ticks) {
	current_task->status = STATUS_CLOCK_SLEEP;
	current_task->sleep_until = timer + ticks;
	scheduler_update();
}

struct fd_table *scheduler_get_fd_table(void) {
	return current_task->fd_table;
}

int low_fork(struct task *old, struct task *new);

void setup_fork(struct task *old, struct task *new) {
	new->pages = memory_page_table_copy(old->pages);
	new->fd_table = vfs_copy_fd_table(old->fd_table);
	new->is_usermode = 0;
	new->pid = pid_counter++;
	new->status = STATUS_RUNNING;
	new->cwd = old->cwd;
}

int scheduler_fork(void) {
	struct task *task = new_task();

	// Race condition...?
	int new_process = low_fork(current_task, task);

	if (new_process)
		return 0;
	return task->pid;
}

int scheduler_wait(struct task_wait *wait) {
	if (wait->pid != -1)
		return 0;

	wait->pid = current_task->pid;

	current_task->status = STATUS_UNLIMITED_SLEEP;

	scheduler_update();
	return 1;
}

int task_is_alive(struct task *task) {
	return task->status != STATUS_DEAD && task->status != STATUS_CLEANUP;
}

static struct task *get_task_from_pid(int pid) {
	for (int i = 0; i < n_tasks; i++) {
		if (tasks[i].pid == pid && task_is_alive(tasks + i)) {
			return &tasks[i];
		}
	}
	return NULL;
}

void scheduler_unwait(struct task_wait *wait) {
	if (wait->pid == -1)
		return;

	struct task *task = get_task_from_pid(wait->pid);

	if (task)
		task->status = STATUS_RUNNING;

	wait->pid = -1;
}

void scheduler_exit(int error_code) {
	(void)error_code;
	current_task->status = STATUS_CLEANUP;

	for (int i = 0; i < current_task->n_waiting; i++) {
		struct task *task = get_task_from_pid(current_task->waiting[i]);
		if (task->status != STATUS_WAIT_FOR_PID)
			continue;
		task->wait_status = error_code;
		task->status = STATUS_RUNNING;
	}

	scheduler_update();
}

int scheduler_wait_for_pid(int pid, int *result) {
	struct task *task = get_task_from_pid(pid);
	if (!task)
		return -1;

	current_task->status = STATUS_WAIT_FOR_PID;

	task->waiting[task->n_waiting++] = current_task->pid;

	scheduler_update();

	*result = current_task->wait_status;

	return 0;
}
