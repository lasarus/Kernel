	.section .bss
stack_tmp:
	.quad 0

	.section .text
.global syscall
.global syscall_handler
syscall_handler:
	.set KERNEL_STACK_POS, 0xffffff0000000000
	.set KERNEL_STACK_SIZE, 0x0000000000001000
	.set KERNEL_STACK_END, KERNEL_STACK_POS + KERNEL_STACK_SIZE

	# Store stack in stack_tmp temporarily.
	# This is not a good way of doing things in case of SMP.

	movq %rsp, stack_tmp
	movabsq $KERNEL_STACK_END, %rsp

    pushq stack_tmp # Push old stack
	pushq %r11 # Old rflags
    pushq %rcx # Place to return
    pushq %r15
    pushq %r14
    pushq %r13
    pushq %r12
    pushq %r11
    pushq %r10
    pushq %r9
    pushq %r8
    pushq %rax
    pushq %rcx
    pushq %rdx
    pushq %rbx
    pushq %rbp
    pushq %rsi
    pushq %rdi
	movq %rax, %r9

	sti # Interrupts are disabled by the SF_MASK.

	call syscall

    popq %rdi
    popq %rsi
    popq %rbp
    popq %rbx
    popq %rdx
    popq %rcx
    popq %rax
    popq %r8
    popq %r9
    popq %r10
    popq %r11
    popq %r12
    popq %r13
    popq %r14
    popq %r15
    popq %rcx # RIP after sysretq.
	popq %r11 # Restore rflags
    popq %rsp

	sysretq

.global syscall_init
syscall_init:	
	# See Appendix A of the AMD64 Volume 2 manual for numerical values
	# to the different MSRs.

	# IA32_EFER/EFER.SCE = 1. This enables the syscall instruction.
	movl $0xC0000080, %ecx
	rdmsr
	bts $0, %eax
	wrmsr

	# IA32_LSTAR/LSTAR = syscall_handler
	movl $0xC0000082, %ecx # See page 709 of the AMD64 Volume 2 manual.
	# Note that wrmsr expects the value to be in edx:eax.
	# This means that a 64 bit address needs to be split into these registers.
	movq $syscall_handler, %rax
	movq $syscall_handler, %rdx
	shrq $32, %rdx
	wrmsr

	# IA32_FMASK/SF_MASK.IF = 1 # Clear interrupt flag in RFLAGS on syscall.
	movl $0xC0000084, %ecx # See page 709 of the AMD64 Volume 2 manual.
	movl $0x00000200, %eax # Set bit 9 to 1.
	xorq %rdx, %rdx
	wrmsr

	# IA32_STAR/STAR = SYSRET CS SS : SYSCALL CS SS : ... # No need to modify EFLAGS on syscall.
	movl $0xC0000081, %ecx # See page 709 of the AMD64 Volume 2 manual.
	movl $0x001B0008, %edx
	wrmsr

	ret
