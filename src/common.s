	.section .text
	.global outb
outb: # void outb(uint16_t port, uint8_t val)
	movl %esi, %eax
	movl %edi, %edx
	outb %al, %dx
	retq

	.global inb
inb: # uint8_t inb(uint16_t port)
	movl %edi, %edx
	inb %dx, %al
	retq

	.global hang_kernel
hang_kernel:
	cli
	hlt
	jmp hang_kernel

	.global sleep_kernel
sleep_kernel:	
	hlt
	retq

	.global load_idt
load_idt: # void load_idt(uint16_t limit, void *base)
	movw %di, -16(%rsp) # Red-zone is not really needed since interrupts are not enabled.
	movq %rsi, -14(%rsp)
	lidtq -16(%rsp)
	sti
	retq

	.global load_gdt
load_gdt: # void load_gdt(uint16_t limit, void *base)
	movw %di, -16(%rsp)
	movq %rsi, -14(%rsp)
	lgdt -16(%rsp)
	sti
	retq

	.global load_tss
load_tss:	 # void load_tss(uint16_t gdt_entry)
	ltr %di
	retq

	.global load_cr3
load_cr3: # void load_cr3(void *pml4_ptr);
	movq %rdi, %cr3
	retq

	.global get_cr2
get_cr2: # uint64_t get_cr2(void);
	movq %cr2, %rax
	retq

msg:	.ascii "Hello World!\n"
	.set msg_len, . - msg
	.global print_interrupt
print_interrupt:	
	movq $1, %rax
	movq $msg_len, %rdx
	movq $msg, %rsi
	int $0x80
	retq
