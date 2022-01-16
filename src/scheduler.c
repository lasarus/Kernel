#include "scheduler.h"
#include "memory.h"
#include "taskswitch.h"
#include "vga_text.h"
#include "interrupts.h"
#include "vfs.h"
#include "elf_loader.h"

int n_tasks = 0;
struct task tasks[16];
struct task *current_task;

static int pid_counter = 0;

void disasm(uint8_t *addr, int len);

int scheduler_execve(const char *filename, const char *const *argv, const char *const *envp) {
	int len = strlen(filename);
	char path_buffer[256];
	for (int i = 0; i <= len; i++)
		path_buffer[i] = filename[i];

	uint64_t rip;
	memory_page_table_delete(current_task->pages, 1);
	elf_loader_load(current_task->pages, path_buffer, &rip);

	const uint64_t user_stack_size = 4 * KIBIBYTE;
	const uint64_t user_stack_pos = GIBIBYTE;

	// Set up user stack.
	memory_allocate_range(current_task->pages, user_stack_pos, NULL, user_stack_size, 1);

	//disasm((void *)rip, 16);

	usermode_jump(user_stack_size + user_stack_pos, rip, 0, 0, 0);
	return 0;
}

int get_next_task() {
	static int prev_task = 0;

	int next_task = prev_task;

	for (int i = 0; i < n_tasks; i++) {
		next_task++;
		if (next_task >= n_tasks)
			next_task = 0;

		int resume = 0;
		switch (tasks[next_task].status) {
		case STATUS_CLOCK_SLEEP:
			if ((tasks[next_task].sleep_until < timer) ||
				(tasks[next_task].sleep_until > (timer - UINT64_MAX / 2))) {
				tasks[next_task].status = STATUS_RUNNING;
				resume = 1;
			}
			break;

		case STATUS_RUNNING:
			resume = 1;
			break;
		default: break;
		}

		if (resume)
			break;
	}

	prev_task = next_task;
	return next_task;
}

void scheduler_init(page_table_t kernel_table) {
	n_tasks++;
	tasks[0].pages = kernel_table;
	tasks[0].cr3 = memory_get_cr3(kernel_table);
	tasks[0].fd_table = vfs_init_fd_table();
	tasks[0].pid = pid_counter++;

	current_task = &tasks[0];

	set_kernel_stack(KERNEL_STACK_POS + KERNEL_STACK_SIZE);
}

// Scheduling is about fairly distributing CPU time among the tasks.
void scheduler_update(void) {
	int next_task = get_next_task();

	for (int i = 0; i < n_tasks; i++) {
		if (tasks[i].status == STATUS_CLEANUP &&
			current_task - tasks != i) {
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
	new->cr3 = memory_get_cr3(new->pages);
	new->pid = pid_counter++;

	n_tasks++;
}

int scheduler_fork(void) {
	struct task *task = tasks + n_tasks;

	// Race condition...?

	int new_process = low_fork(current_task, task);

	if (new_process)
		return 0;
	else
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

void scheduler_unwait(struct task_wait *wait) {
	if (wait->pid == -1)
		return;

	int pid = wait->pid;
	wait->pid = -1;
	tasks[pid].status = STATUS_RUNNING;
}

void scheduler_exit(int error_code) {
	current_task->status = STATUS_CLEANUP;

	scheduler_update();
}
