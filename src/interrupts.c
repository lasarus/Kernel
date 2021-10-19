#include "interrupts.h"
#include "vga_text.h"

#include "common.h"

struct interrupt_frame_s;

__attribute__((interrupt))
void unhandled_interrupt(struct interrupt_frame_s *frame) {
	print("Unhandled interrupt.\n");
	hang_kernel();
}

__attribute__((interrupt))
void exception_double_fault(struct interrupt_frame_s *frame, uint64_t error_code) {
	print("Double fault.\n");
	hang_kernel();
}

__attribute__((interrupt))
void exception_general_protection_fault(struct interrupt_frame_s *frame, uint64_t error_code) {
	print("General protection fault.\nWith error code: ");
	print_int((int)error_code);
	hang_kernel();
}

__attribute__((interrupt))
void irq_master_reset(struct interrupt_frame_s *frame) {
	print("IRQ master");
	outb(0x20, 0x20);
}

__attribute__((interrupt))
void irq_slave_reset(struct interrupt_frame_s *frame) {
	print("IRQ slave");
	outb(0xA0, 0x20);
	outb(0x20, 0x20);
}

__attribute__((interrupt))
void irq_timer(struct interrupt_frame_s *frame) {
	/* print("Timer "); */
	/* static int tick = 0; */
	/* print_int(tick++); */
	/* print("\n"); */

	outb(0xA0, 0x20);
	outb(0x20, 0x20);
}

__attribute__((interrupt))
void irq_keyboard(struct interrupt_frame_s *frame) {
	unsigned char scan_code = inb(0x60);
	print_int(scan_code);

	outb(0x20, 0x20);
}

enum {
	EXCEPTION_DOUBLE_FAULT = 8,
	EXCEPTION_GENERAL_PROTECTION_FAULT = 13,
	IRQ0 = 32,
};

void* isr_table[] = {
	[EXCEPTION_DOUBLE_FAULT] = exception_double_fault,
	[EXCEPTION_GENERAL_PROTECTION_FAULT] = exception_general_protection_fault,
	[IRQ0+0] = irq_timer,
	[IRQ0+1] = irq_keyboard,
	[IRQ0+2] = irq_master_reset,
	[IRQ0+3] = irq_master_reset,
	[IRQ0+4] = irq_master_reset,
	[IRQ0+5] = irq_master_reset,
	[IRQ0+6] = irq_master_reset,
	[IRQ0+7] = irq_master_reset,
	[IRQ0+8] = irq_slave_reset,
	[IRQ0+9] = irq_slave_reset,
	[IRQ0+10] = irq_slave_reset,
	[IRQ0+11] = irq_slave_reset,
	[IRQ0+12] = irq_slave_reset,
	[IRQ0+13] = irq_slave_reset,
	[IRQ0+14] = irq_slave_reset,
	[IRQ0+15] = irq_slave_reset,
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

void idt_set_descriptor(struct idt_entry *entry, void* isr, uint8_t flags) {
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
			isr_table[i] = unhandled_interrupt;
	}

	for (unsigned i = 0; i < COUNTOF(idt); i++) {
		idt_set_descriptor(idt + i, isr_table[i], 0x8E);
    }

	load_idt(sizeof idt, idt);
}
