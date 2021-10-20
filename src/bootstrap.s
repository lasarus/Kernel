	.global kmain

	.section .multiboot, "a"

	.align  4096
multiboot_header:
	.set MAGIC, 0x1BADB002
	.set ALIGN, 1
	.set MEMINFO, 2
	.set MANUAL_MEMORY, 0x10000
	.set FLAGS, (ALIGN | MEMINFO | MANUAL_MEMORY)

	.long MAGIC
	.long FLAGS
	.long -(MAGIC + FLAGS)
	.long multiboot_header
	.long load_addr
	.long _edata
	.long _end
	.long _start

.section .bootstrap.bss
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
	.zero 8 * 512
	.align 4096
PML4_table:	
	# One entry for lower, one entry for higher. Both pointing to the same PDPL_table.
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
	.zero 8 * 512

GDT:
	.quad 0 # Null descriptor
	.quad 0 # code descriptor (not initialized)
	.quad 0 # data descriptor (not initialized)

GDT_ptr:	
	.word 0
	.long 0

.section .bootstrap.text

	.code32
	.global _start
_start:
	movl %eax, %edi # Move multiboot magic out of the way.
	movl %ebx, %esi # Move multiboot data structure out of the way.

	# Initialize page tables.
	movl $((0x1 | 0x2 | 0x4 | 0x80)), PDPT_table
	movl $((0x1 | 0x2 | 0x4 | 0x80)), PDPT_table + 8 * 511
	movl $((0x1 | 0x2 | 0x4) + PDPT_table), PML4_table
	movl $((0x1 | 0x2 | 0x4) + PDPT_table), PML4_table + 8 * 511

	# Initailize GDT.
	movl $0x00209A00, GDT + 8 + 4
	movl $0x00009200, GDT + 16 + 4

	# Initialize GDT_ptr.
	movw $(3 * 8 - 1), GDT_ptr
	movl $GDT, GDT_ptr + 2

	# Initialze IA-32e mode.
	# No error handling is done. I assume you are not stupid enough to run this code on a 32 bit
	# processor. Just like you wouldn't run it on an ARM processor.

	# 1. Start from protected mode, disable paging. (Already done by bootloader.)
	# 2. Enable PAE by setting CR4.PAE = 1
	movl %cr4, %eax
	orl $0x20, %eax
	# orl $0xA0, %eax
	movl %eax, %cr4
	# 3. Load CR3 with the physical base address of PML4.
	movl $PML4_table, %eax
	movl %eax, %cr3
	# 4. Enable IA-32e mode by setting IA32_EFER.LME = 1.
	movl $0xC0000080, %ecx
	rdmsr
	#movl $0x100, %eax
	#movl $0, %edx
	bts $8, %eax
	wrmsr
	# 5. Enable paging by setting CR0.PG = 1.
	movl %cr0, %eax
	orl $0x80000000, %eax
	movl %eax, %cr0

	lgdt GDT_ptr

	# We set up the GDT so that GDT+8 is the code segment.
	# Doing a long jump will put us into long mode.
	ljmp $8, $long_mode

	.code64
long_mode:	
	movq $stack_top, %rsp

	# Second argument to kmain is a pointer, turn it into higher half pointer.
	addq $HIGHER_HALF_OFFSET, %rsi

	xorq %rax, %rax # %rax holds number of SSE registers, according to Sys-V.
	call kmain

halt:
	hlt
	jmp halt

.section .bss
	.align 16
stack_bottom:
	.skip 16348
	.align 16
stack_top:	
