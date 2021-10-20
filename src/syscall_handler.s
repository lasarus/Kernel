.global syscall
.global syscall_handler
syscall_handler:
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rdi
	pushq %rsi
	pushq %rcx
	pushq %rdx
	pushq %rax

	movq %r8, %r9
	movq %r10, %r8
	movq %rdx, %rcx
	movq %rsi, %rdx
	movq %rdi, %rsi
	movq %rax, %rdi

	cld
	call syscall
	popq %rax
	popq %rdx
	popq %rcx
	popq %rsi
	popq %rdi
	popq %r8
	popq %r9
	popq %r10
	popq %r11
	iretq
