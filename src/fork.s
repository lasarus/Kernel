.global setup_fork
.global low_fork
low_fork:	# int low_fork(struct task *old, struct task *new)
	# Put 1 in rax if in new process, 0 otherwise.
	# This does not concern intself about pages or stacks.

	movq $_new, 0(%rsi)
	movq %rbx, 8 * 4(%rsi)
	movq %rsp, 8 * 5(%rsi)
	movq %rbp, 8 * 6(%rsi)
	movq %r12, 8 * 13(%rsi)
	movq %r13, 8 * 14(%rsi)
	movq %r14, 8 * 15(%rsi)
	movq %r15, 8 * 16(%rsi)

	call setup_fork

	movq $0, %rax
	ret

_new:	
	movq $1, %rax
	ret
