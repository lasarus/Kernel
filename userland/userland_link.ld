/* This is a temporary link script for userland binaries. */
OUTPUT_FORMAT("binary")

SECTIONS {
	. = 1M;

	.text : {
		*(.text)
	}
	/DISCARD/ : {
		*(*)
	}
}