OUTPUT_FORMAT("binary")

HIGHER_HALF_OFFSET = 0xFFFFFFFFC0000000;

SECTIONS {
	. = 1M;

	load_addr = .;

	.bootstrap.text : {
		*(.multiboot)
		*(.bootstrap.text)
	}

	. += HIGHER_HALF_OFFSET;

	.text : AT(ADDR(.text) - HIGHER_HALF_OFFSET) {
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*) /* ?? */
	}

	.data : AT(ADDR(.data) - HIGHER_HALF_OFFSET) {
		*(.data)
		*(.data.*) /* I don't know why this is needed, somehow gcc is creating extra .data.* sections that should be placed here. */
	}

	_edata = . - HIGHER_HALF_OFFSET;

	.bss : AT(ADDR(.data) - HIGHER_HALF_OFFSET) {
		*(COMMON)
		*(.bss)
	}

	/* We are now high in the address space, and we must land on the ground
	Before storing _end, which is used in the multiboot header to tell
	the bootloader how the image should be loaded. */
	. -= HIGHER_HALF_OFFSET;

	.bootstrap.bss : {
		*(.bootstrap.bss)
	}

	_end = .;

	/DISCARD/ : {
		*(.note*)
	}
}
