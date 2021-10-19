.section .multiboot
	.align 4
	.long 0x1BADB002
	.long 3
	.long -(0x1BADB002 + 3)

.section .bss
	.align 16
stack_bottom:
	.skip 16348
	.align 16
stack_top:	

.section .data
	.align 4096
PDPT_table:	
	# Only one entry.
	# 0x1 Present
	# 0x2 R/W
	# 0x4 U/S
	# 0x8 PWT
	# 0x10 PCD
	# 0x80 PS (1 if GB large pages)
	# 11:8 Ignored
	# (M-1):12 Physical address of 4KByte aligned page-directory-pointer table referenced by this entry.
	# 51:M Reserved
	# 62:52 Ignored
	# 63 XD
	.quad 0x1 | 0x2 | 0x4 | 0x80
	.align 4096
PML4_table:	
	# Only one entry.
	# 0x1 Present
	# 0x2 R/W
	# 0x4 U/S
	# 0x8 PWT
	# 0x10 PCD
	# 11:8 Ignored
	# (M-1):12 Physical address of 4KByte aligned page-directory-pointer table referenced by this entry.
	# 51:M Reserved
	# 62:52 Ignored
	# 63 XD
	.quad (0x1 | 0x2 | 0x4) # Has to be initialized at runtime with the correct address to PDPT_table

	.align 4096
GDT:
	.quad 0x0000000000000000 # Null descriptor
	.quad 0x00209A0000000000 # code descriptor
	.quad 0x0000920000000000 # data descriptor

	.align 4
GDT_ptr:	
	.word . - GDT - 1
	.long GDT

.section .text

	.code32
	.global _start
_start:
	# Initialze IA-32e mode.
	# No error handling is done. I assume you are not stupid enough to run this code on a x86
	# processor. Just like you wouldn't run it on an ARM processor.
	movl $PDPT_table, %eax
	orl PML4_table, %eax
	movl %eax, PML4_table

	# 1. Start from protected mode, disable paging....
	# 2. Enable PAE by setting CR4.PAE = 1
	movl %cr4, %eax
	orl $0x20, %eax
	# orl $0xA0, %eax
	movl %eax, %cr4
	# 3. Load CR3 with the physical base address of PML4
	movl $PML4_table, %eax
	movl %eax, %cr3
	# 4. Enable IA-32e mode by setting IA32_EFER.LME = 1
	movl $0xC0000080, %ecx
	rdmsr
	#movl $0x100, %eax
	#movl $0, %edx
	bts $8, %eax
	wrmsr
	# 5. Enable paging by setting CR0.PG = 1
	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0

	lgdt GDT_ptr
	ljmp $0x8, $long_mode

.code64
	.global kmain
long_mode:	
	# Now we are in long mode for real.
	movq $stack_top, %rsp
	xorq %rax, %rax
	call kmain
halt:	
	hlt
	jmp halt

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
