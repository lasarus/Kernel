	.section .text
	.global _start
_start:	
	movq $1, %rax
	movq $1, %rdi
	movq $msg_len, %rdx
	movq $msg, %rsi
	syscall
	movq $35, %rax
	movq $2, %rdi
	syscall
	jmp _start
	retq
msg:	.ascii "B"
	.set msg_len, . - msg
	.global print_interrupt
