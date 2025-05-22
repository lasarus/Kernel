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

	.global execve
execve:	
	movq $59, %rax
	syscall
	retq

	.global nanosleep
nanosleep:	
	movq $0x23, %rax
	syscall
	retq

	.global exit
exit:	
	movq $0x3c, %rax
	syscall
	retq

	.global _start
_start:
	mov (%rsp), %rdi
	lea 8(%rsp), %rsi
	call main
	movq %rax, %rdi
	movq $0x3c, %rax
	syscall

	.global open
open:	#int open(const char *pathname, int flags);
	movq $2, %rax
	syscall
	retq

	.global getdents64
getdents64:	
# ssize_t getdents64(int fd, void *dirp, size_t count);
	movq $217, %rax
	syscall
	retq

	.global close
close:	#int close(int fd);
	movq $3, %rax
	syscall
	retq

	.global wait4
wait4: #int wait4(int pid, int *wstatus, int options, struct rusage *rusage);
	movq $61, %rax
	syscall
	retq

	.global getcwd
getcwd: #int getcwd(char *buffer, size_t size) ;
	movq $79, %rax
	syscall
	retq


	.global chdir
chdir:	#int chdir(const char *path);
	movq $80, %rax
	syscall
	retq
