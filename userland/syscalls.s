	.global read
	# void read(unsigned int fd, char *buf, size_t count)
	# (rdi, rsi, rdx, rcx, ...) -> (rdi, rsi, rdx, r10, r8, ...)
read:	
	movq $0, %rax
	syscall
	#int $0x80
	retq

	.global write
	# void wrtite(unsigned int fd, const char *buf, size_t count)
	# (rdi, rsi, rdx, rcx, ...) -> (rdi, rsi, rdx, r10, r8, ...)
write:	
	movq $1, %rax
	syscall
	#int $0x80
	retq

	.global fork
fork:	
	movq $57, %rax
	syscall
	retq

	.global nanosleep
nanosleep:	
	movq $0x23, %rax
	syscall
	retq

	.global _start
_start:	
	call main
	movq %rax, %rdi
	movq $0x3c, %rax
	syscall
