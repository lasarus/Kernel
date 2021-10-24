	.section .text
_start:	
	movq $1, %rax
	movq $msg_len, %rdx
	movq $msg, %rsi
	int $0x80

	movq $35, %rax
	movq $2, %rdi
	int $0x80
	jmp _start
	retq
msg:	.ascii "B"
	.set msg_len, . - msg
	.global print_interrupt
