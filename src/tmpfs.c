#include "tmpfs.h"
#include "memory.h"
#include "vga_text.h"

struct file_entry {
	char name[256];
	struct inode *inode;
};

#define N_DIR_ENTRIES (4096 / sizeof(struct file_entry))

struct dir_data {
	int n_entries;
	struct file_entry entries[N_DIR_ENTRIES];
};

_Static_assert(sizeof(struct dir_data) <= 4096, "");

struct inode *tmpfs_dir_create_child(struct inode *inode, const char *name, int len) {
	struct dir_data *data = inode->data;
	struct file_entry *entry = &data->entries[data->n_entries++];
	for (int i = 0; i < len; i++)
		entry->name[i] = name[i];
	entry->inode = vfs_new_inode();
	return entry->inode;
}

struct inode *tmpfs_dir_find_child(struct inode *inode, const char *name, int len) {
	hang_kernel();
	return NULL;
}

void tmpfs_init_dir(struct inode *inode) {
	struct dir_data *data = memory_alloc();

	data->n_entries = 0;

	inode->type = INODE_DIRECTORY;
	inode->data = data;
	inode->create_child = tmpfs_dir_create_child;
	inode->create_child = tmpfs_dir_create_child;
}

struct file_block {
	uint8_t data[4096];
};

struct file_header {
	struct file_header *next;
	uint64_t n_blocks;
	struct file_block *blocks[510];
};

_Static_assert(sizeof(struct file_header) <= 4096, "");

// Each process has its own file-descriptors?
// A table. What do the table entries represent?

ssize_t tmpfs_file_read(struct inode *inode, void *data, size_t count) {
	print("Reading from file");
	(void)inode;
	return 0;
}

ssize_t tmpfs_file_write(struct inode *inode, const void *data, size_t count) {
	char *buf = (char *)data;

	for (uint64_t i = 0; i < count; i++) {
		print_char(buf[i]);
	}
	return 0;
}

void tmpfs_init_file(struct inode *inode) {
	struct file_header *header = memory_alloc();

	header->next = NULL;
	header->n_blocks = 0;

	inode->type = INODE_FILE;
	inode->data = header;
	inode->read = tmpfs_file_read;
	inode->write = tmpfs_file_write;
}
