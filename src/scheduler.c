#include "scheduler.h"
#include "memory.h"
#include "taskswitch.h"
#include "vga_text.h"
#include "interrupts.h"

struct task {
	uint64_t rip, gp_regs[16];
	uint64_t cr3;
	uint8_t is_usermode;

	uint8_t sleep;
	uint64_t sleep_until;
	// TODO: XMM0-15

	page_table_t pages;
};

void switch_task_to(struct task *task);

int n_tasks = 0;
struct task tasks[16];
struct task *current_task;

#define KERNEL_STACK_SIZE (4 * KIBIBYTE)
#define KERNEL_STACK_POS (510ULL << (9 * 2 + 12))

void scheduler_add_task(uint8_t *data, uint64_t size) {
	struct task *task = tasks + n_tasks;

	task->pages = memory_new_page_table();

	memory_allocate_range(task->pages, 0x100000, data, size, 1);

	memory_allocate_range(task->pages, KERNEL_STACK_POS, NULL, KERNEL_STACK_SIZE, 0);

	// Set up user stack.
	const uint64_t user_stack_size = 4 * KIBIBYTE;
	const uint64_t user_stack_pos = GIBIBYTE;
	memory_allocate_range(task->pages, user_stack_pos, NULL, user_stack_size, 1);

	task->is_usermode = 1;
	task->rip = 0x100000;

	task->gp_regs[4] = user_stack_pos + user_stack_size;
	task->cr3 = memory_get_cr3(task->pages);

	n_tasks++;
}

int get_next_task() {
	static int prev_task = 0;

	int next_task = prev_task;

	for (int i = 0; i < n_tasks; i++) {
		next_task++;
		if (next_task >= n_tasks)
			next_task = 0;

		if (tasks[next_task].sleep) {
			if ((tasks[next_task].sleep_until < timer) ||
				(tasks[next_task].sleep_until > (timer - UINT64_MAX / 2))) {
				tasks[next_task].sleep = 0;
				break;
			} else {
			}
		} else {
			break;
		}
	}

	prev_task = next_task;
	return next_task;
}

void scheduler_init(page_table_t kernel_table) {
	n_tasks++;
	tasks[0].pages = kernel_table;
	tasks[0].cr3 = memory_get_cr3(kernel_table);
	current_task = &tasks[0];

	//memory_allocate_range(kernel_table, kernel_stack_pos, NULL, kernel_stack_size, 0);

	set_kernel_stack(KERNEL_STACK_POS + KERNEL_STACK_SIZE);
}

// Scheduling is about fairly distributing CPU time among the tasks.
void scheduler_update(void) {
	int next_task = get_next_task();

	struct task *task = tasks + next_task;

	switch_task_to(task);
}

void scheduler_suspend(void) {
	switch_task_to(tasks + 0);
}

void scheduler_sleep(uint64_t ticks) {
	current_task->sleep = 1;
	current_task->sleep_until = timer + ticks;
	scheduler_update();
}
