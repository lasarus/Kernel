OUTPUT_FORMAT(binary)

ENTRY(_start)

SECTIONS
{
	. = 1M;

	load_addr = .;
	.multiboot.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.bootstrap.text)
		*(.text)
	}

	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}

	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}
	_edata = .;

	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
	_end = .;
}
