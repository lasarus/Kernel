/* This is a temporary link script for userland binaries. */
OUTPUT_FORMAT("elf64-x86-64")

ENTRY(_start)

SECTIONS {
	. = 1M;

	.text : {
		*(.text)
	}

	.rodata : {
		*(.rodata)
	}

	.data : {
		*(.data)
	}

	.bss : {
		*(.bss)
	}

	/DISCARD/ : {
		*(*)
	}
}
