
	# This is based on 8.9.3 in AMD64 manual.
	# Figure 6-8 Intel manual is also useful.
	.global usermode_jump
usermode_jump:	#void usermode_jump(uint64_t user_stack, uint64_t kernel_stack, uint64_t entry)
	movw $(32 | 3), %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs

	pushq $(32 | 3)
	pushq %rdi # user_stack
	pushfq # Should be loaded from task data at a later stage.
	pushq $(24 | 3)
	pushq %rdx

	movq $0, %rax
	iretq

	retq
