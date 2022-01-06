	.section .text
_start:	
	movq $1, %rax
	movq $1, %rdi
	movq $10, %r15
	movq $msg_len, %rdx
	movq $msg, %rsi
	int $0x80
	movq $35, %rax
	movq $20, %rdi
	int $0x80
	jmp _start
	retq
msg:	.ascii "A"
	.set msg_len, . - msg
	.global print_interrupt
