	.section .text
_start:	
	movq $1, %rax
	movq $msg_len, %rdx
	movq $msg, %rsi
	int $0x80
	retq
msg:	.ascii "Hello world from userland (although still ring 0)!\n"
	.set msg_len, . - msg
	.global print_interrupt
