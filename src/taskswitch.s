
	# This is based on 8.9.3 in AMD64 manual.
	# Figure 6-8 Intel manual is also useful.
	.global usermode_jump
usermode_jump:	#void usermode_jump(uint64_t user_stack, uint64_t kernel_stack, uint64_t entry)
	#movw $(32 | 3), %ax
	#movw %ax, %ds
	#movw %ax, %es
	#movw %ax, %fs
	#movw %ax, %gs

	pushq $(32 | 3)
	pushq %rdi # user_stack
	pushfq # Should be loaded from task data at a later stage.
	pushq $(24 | 3)
	pushq %rdx

	movq $0, %rax
	iretq

	retq

	.global switch_task_to
switch_task_to: # void switch_task_to(struct task *task)
	# When returning from this function, we shall be back in the task that called it to begin with.
	# Assuming that we are already in ring 0 when calling this (which is kind of obvious.)

	movq current_task, %rax
	movq $_on_return, 0(%rax)
	movq %rax, 8 * 1(%rax)
	movq %rcx, 8 * 2(%rax)
	movq %rdx, 8 * 3(%rax)
	movq %rbx, 8 * 4(%rax)
	movq %rsp, 8 * 5(%rax)
	movq %rbp, 8 * 6(%rax)
	movq %rsi, 8 * 7(%rax)
	movq %rdi, 8 * 8(%rax)
	movq %r8, 8 * 9(%rax)
	movq %r9, 8 * 10(%rax)
	movq %r10, 8 * 11(%rax)
	movq %r11, 8 * 12(%rax)
	movq %r12, 8 * 13(%rax)
	movq %r13, 8 * 14(%rax)
	movq %r14, 8 * 15(%rax)
	movq %r15, 8 * 16(%rax)

	# State is now saved.

	movq %rdi, current_task
	movq %rdi, %rax

	cmpl $1, 8 * 17(%rax)
	je _to_userland

	movq 8 * 1(%rax), %rdi
	movq 8 * 2(%rax), %rcx
	movq 8 * 3(%rax), %rdx
	movq 8 * 4(%rax), %rbx
	movq 8 * 5(%rax), %rsp
	movq 8 * 6(%rax), %rbp
	movq 8 * 7(%rax), %rsi
	movq 8 * 8(%rax), %rdi
	movq 8 * 9(%rax), %r8
	movq 8 * 10(%rax), %r9
	movq 8 * 11(%rax), %r10
	movq 8 * 12(%rax), %r11
	movq 8 * 13(%rax), %r12
	movq 8 * 14(%rax), %r13
	movq 8 * 15(%rax), %r14
	movq 8 * 16(%rax), %r15

	movq 0(%rax), %rax
	jmp *%rax

_to_userland:	
	movl $0, 8 * 17(%rax) # Will no longer be userland.
	movq 8 * 5(%rax), %rdi
	movq 8 * 0(%rax), %rdx

	pushq $(32 | 3)
	pushq %rdi # user_stack
	pushfq # Should be loaded from task data at a later stage.
	pushq $(24 | 3)
	pushq %rdx

	iretq

_on_return:	
	retq
