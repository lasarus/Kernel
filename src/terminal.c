#include "terminal.h"
#include "driver_major_numbers.h"
#include "vfs.h"
#include "vga_text.h"

static ssize_t terminal_file_read(struct inode *inode, size_t *offset, void *data, size_t count) {
	return 0;
}

static ssize_t terminal_file_write(struct inode *inode, size_t *offset, const void *data, size_t count) {
	char *buf = (char *)data;

	unsigned char color = VGA_COLOR(VGA_WHITE, VGA_BLACK);
	switch (inode->minor) {
	case TERMINAL_MINOR_RED: color = VGA_COLOR(VGA_LIGHTRED, VGA_BLACK); break;
	case TERMINAL_MINOR_WHITE: color = VGA_COLOR(VGA_WHITE, VGA_BLACK); break;
	default: break;
	}

	for (uint64_t i = 0; i < count; i++) {
		print_char_color(buf[i], color);
	}

	return 0;
}

static struct inode_operations operations = {
	.read = terminal_file_read,
	.write = terminal_file_write,
};

void terminal_init(void) {
	vfs_register_driver(MAJOR_TERMINAL, &operations);
}
