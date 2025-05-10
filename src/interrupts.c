#include "interrupts.h"
#include "keyboard.h"
#include "scheduler.h"
#include "vga_text.h"

#include "common.h"

struct interrupt_frame_s {
	uint64_t rip, cs, rflags, rsp, ss;
};

extern int n_tasks;
extern struct task tasks[16];
extern struct task *current_task;

__attribute__((interrupt)) void unhandled_interrupt(struct interrupt_frame_s *frame) {
	(void)frame;
	print("\nCurrent task: ");
	print_int(current_task - tasks);
	print("\nUnhandled interrupt.\n");
	hang_kernel();
}

__attribute__((interrupt)) void exception_double_fault(struct interrupt_frame_s *frame, uint64_t error_code) {
	(void)frame;
	print("Double fault.\nWith error code:");
	print_int((int)error_code);
	hang_kernel();
}

__attribute__((interrupt)) void exception_general_protection_fault(struct interrupt_frame_s *frame,
                                                                   uint64_t error_code) {
	(void)frame;
	print("General protection fault.\nWith error code: ");
	print_int((int)error_code);
	print("\n At instruction: ");
	print_hex(frame->rip);
	hang_kernel();
}

void disasm(uint8_t *addr, int len) {
	for (int i = 0; i < len; i++) {
		print_hex(addr[i]);
		print((i + 1) % 4 == 0 ? "\n" : " ");
	}
}

__attribute__((interrupt)) void exception_page_fault(struct interrupt_frame_s *frame, uint64_t error_code) {
	(void)frame;
	uint64_t cr2 = get_cr2();
	uint64_t cr3 = get_cr3();
	print("Page fault.\nWith error code: ");
	print_int((int)error_code);
	print("\n CRT2: ");
	print_hex(cr2);
	print("\n CRT3: ");
	print_hex(cr3);
	print("\n RIP: ");
	print_hex(frame->rip);
	print("\n");
	disasm((void *)frame->rip, 16);
	hang_kernel();
}

__attribute__((interrupt)) void irq_master_reset(struct interrupt_frame_s *frame) {
	(void)frame;
	print("IRQ master");
	outb(0x20, 0x20);
}

__attribute__((interrupt)) void irq_slave_reset(struct interrupt_frame_s *frame) {
	(void)frame;
	print("IRQ slave");
	outb(0xA0, 0x20);
	outb(0x20, 0x20);
}

uint64_t timer = 0;

__attribute__((interrupt)) void irq_timer(volatile struct interrupt_frame_s *frame) {
	(void)frame;

	timer++;

	outb(0xA0, 0x20);
	outb(0x20, 0x20);

	scheduler_update();
}

__attribute__((interrupt)) void irq_keyboard(struct interrupt_frame_s *frame) {
	(void)frame;
	unsigned char scancode = inb(0x60);
	keyboard_feed_scancode(scancode);

	outb(0x20, 0x20);
}

enum {
	EXCEPTION_DOUBLE_FAULT = 8,
	EXCEPTION_GENERAL_PROTECTION_FAULT = 13,
	EXCEPTION_PAGE_FAULT = 14,
	IRQ0 = 32,
};

typedef void (*func_ptr)();
func_ptr isr_table[] = {
	[EXCEPTION_DOUBLE_FAULT] = (func_ptr)exception_double_fault,
	[EXCEPTION_GENERAL_PROTECTION_FAULT] = (func_ptr)exception_general_protection_fault,
	[EXCEPTION_PAGE_FAULT] = (func_ptr)exception_page_fault,
	[IRQ0 + 0] = (func_ptr)irq_timer,
	[IRQ0 + 1] = (func_ptr)irq_keyboard,
	[IRQ0 + 2] = (func_ptr)irq_master_reset,
	[IRQ0 + 3] = (func_ptr)irq_master_reset,
	[IRQ0 + 4] = (func_ptr)irq_master_reset,
	[IRQ0 + 5] = (func_ptr)irq_master_reset,
	[IRQ0 + 6] = (func_ptr)irq_master_reset,
	[IRQ0 + 7] = (func_ptr)irq_master_reset,
	[IRQ0 + 8] = (func_ptr)irq_slave_reset,
	[IRQ0 + 9] = (func_ptr)irq_slave_reset,
	[IRQ0 + 10] = (func_ptr)irq_slave_reset,
	[IRQ0 + 11] = (func_ptr)irq_slave_reset,
	[IRQ0 + 12] = (func_ptr)irq_slave_reset,
	[IRQ0 + 13] = (func_ptr)irq_slave_reset,
	[IRQ0 + 14] = (func_ptr)irq_slave_reset,
	[IRQ0 + 15] = (func_ptr)irq_slave_reset,
};

_Alignas(0x10) struct idt_entry {
	uint16_t offset_low;
	uint16_t segment_selector;
	uint8_t ist; // Only 3 bits.
	uint8_t attributes; // type, 0, DPL, P
	uint16_t offset_mid;
	uint32_t offset_high;
	uint32_t reserved;
} __attribute((packed)) idt[COUNTOF(isr_table)];

void idt_set_descriptor(struct idt_entry *entry, func_ptr isr, uint8_t flags) {
	entry->offset_low = (uint64_t)isr & 0xFFFF;
	entry->segment_selector = 0x8;
	entry->ist = 0;
	entry->attributes = flags;
	entry->offset_mid = ((uint64_t)isr >> 16) & 0xFFFF;
	entry->offset_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
	entry->reserved = 0;
}

void interrupts_init(void) {
	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	outb(0x21, 0x20);
	outb(0xA1, 0x28);
	outb(0x21, 0x04);
	outb(0xA1, 0x02);
	outb(0x21, 0x01);
	outb(0xA1, 0x01);
	outb(0x21, 0x0);
	outb(0xA1, 0x0);

	for (unsigned i = 0; i < COUNTOF(isr_table); i++) {
		if (!isr_table[i])
			isr_table[i] = (func_ptr)unhandled_interrupt;
	}

	for (unsigned i = 0; i < COUNTOF(idt); i++) {
		if (i == 0x80)
			idt_set_descriptor(idt + i, isr_table[i], 0xEE);
		else
			idt_set_descriptor(idt + i, isr_table[i], 0x8E);
	}

	load_idt(sizeof idt, idt);
}
