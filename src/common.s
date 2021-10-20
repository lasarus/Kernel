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
	.global load_idt

load_idt: # void load_idt(uint16_t limit, void *base)
	movw %di, -16(%rsp) # Red-zone is not really needed since interrupts are not enabled.
	movq %rsi, -14(%rsp)
	lidtq -16(%rsp)
	sti
	retq
