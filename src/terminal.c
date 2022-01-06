#include "terminal.h"
#include "vga_text.h"

static ssize_t terminal_file_read(struct inode *inode, void *data, size_t count) {
	print("Not implemented.");
	(void)inode;
	return 0;
}

static ssize_t terminal_file_write(struct inode *inode, const void *data, size_t count) {
	char *buf = (char *)data;

	for (uint64_t i = 0; i < count; i++) {
		print_char(buf[i]);
	}
	return 0;
}

void terminal_init_inode(struct inode *inode) {
	inode->type = INODE_FILE;
	inode->data = NULL;
	inode->read = terminal_file_read;
	inode->write = terminal_file_write;
}

static ssize_t terminal_red_file_read(struct inode *inode, void *data, size_t count) {
	print("Not implemented.");
	(void)inode;
	return 0;
}

static ssize_t terminal_red_file_write(struct inode *inode, const void *data, size_t count) {
	char *buf = (char *)data;

	for (uint64_t i = 0; i < count; i++) {
		print_char_color(buf[i], VGA_COLOR(VGA_LIGHTRED, VGA_BLACK));
	}
	return 0;
}

void terminal_red_init_inode(struct inode *inode) {
	inode->type = INODE_FILE;
	inode->data = NULL;
	inode->read = terminal_red_file_read;
	inode->write = terminal_red_file_write;
}
