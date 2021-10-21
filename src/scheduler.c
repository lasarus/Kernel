#include "scheduler.h"
#include "memory.h"
#include "taskswitch.h"
#include "vga_text.h"
#include "interrupts.h"

struct task {
	uint64_t rip, gp_regs[16];
	uint8_t is_usermode;

	uint8_t sleep;
	uint64_t sleep_until;
	// TODO: XMM0-15

	page_table_t pages;
};

void switch_task_to(struct task *task);

static int n_tasks = 0;
//int current_task = -1; // Set this to 0 when starting kernel thread.
static struct task tasks[16];
struct task *current_task;

void scheduler_add_task(uint8_t *data, uint64_t size) {
	struct task *task = tasks + n_tasks;

	task->pages = memory_new_page_table();
	memory_switch_page_table(task->pages);

	uint64_t virt_mem = 0x100000;
	memory_allocate_range(virt_mem, size, 1);
	for (unsigned i = 0; i < size; i++)
		((uint8_t*)virt_mem)[i] = data[i];

	const uint64_t kernel_stack_size = 4096 * 4;
	const uint64_t kernel_stack_pos = HIGHER_HALF_OFFSET - kernel_stack_size;
	memory_allocate_range(kernel_stack_pos, kernel_stack_size, 0);

	// Set up user stack.
	const uint64_t user_stack_size = 4 * MEBIBYTE;
	const uint64_t user_stack_pos = GIBIBYTE;
	memory_allocate_range(user_stack_pos, user_stack_size, 1);

	task->is_usermode = 1;
	task->rip = 0x100000;

	task->gp_regs[4] = user_stack_pos + user_stack_size;

	n_tasks++;

	print("Added task\n");
	// usermode_jump(user_stack_pos, kernel_stack_pos, 0x100000);
}

int get_next_task() {
	static int prev_task = 0;

	int next_task = prev_task;

	for (int i = 0; i < n_tasks; i++) {
		next_task++;
		if (next_task >= n_tasks)
			next_task = 0;

		if (tasks[next_task].sleep) {
			if (tasks[next_task].sleep_until > timer ||
				tasks[next_task].sleep_until > (timer - UINT64_MAX / 2)) {
				tasks[next_task].sleep = 0;
				break;
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
	current_task = &tasks[0];

	memory_switch_page_table(kernel_table); // After this, memory_switch_page_table, should probably only be called by the kernel?
	// set_kernel_stack(kernel_stack_pos + kernel_stack_size - 128); // Stack grows downwards.
}

// Scheduling is about fairly distributing CPU time among the tasks.
void scheduler_update(void) {
	int next_task = get_next_task();

	struct task *task = tasks + next_task;
	memory_switch_page_table(task->pages);
	//set_kernel_stack(task->

	const uint64_t kernel_stack_size = 4096 * 4;
	const uint64_t kernel_stack_pos = HIGHER_HALF_OFFSET - kernel_stack_size;

	set_kernel_stack(kernel_stack_pos + kernel_stack_size);

	switch_task_to(task);
}

void scheduler_suspend(void) {
	switch_task_to(tasks + 0);
}
