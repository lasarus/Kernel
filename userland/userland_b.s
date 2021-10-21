	.section .text
_start:	
	movq $1, %rax
	movq $msg_len, %rdx
	movq $msg, %rsi
	int $0x80
	jmp _start
	retq
msg:	.ascii "Hello world from ring 3! BBB\n"
	.set msg_len, . - msg
	.global print_interrupt
