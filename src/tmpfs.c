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

_Static_assert(N_DIR_ENTRIES > 14, "");

_Static_assert(sizeof(struct dir_data) <= 4096, "");

struct inode *tmpfs_dir_create_child(struct inode *inode, const char *name, int len) {
	struct dir_data *data = inode->data;
	struct file_entry *entry = &data->entries[data->n_entries++];
	for (int i = 0; i < len; i++)
		entry->name[i] = name[i];
	entry->name[len] = '\0';
	entry->inode = vfs_new_inode();
	return entry->inode;
}

struct inode *tmpfs_dir_find_child(struct inode *inode, const char *name, int len) {
	struct dir_data *data = inode->data;
	for (int i = 0; i < data->n_entries; i++) {
		if (strncmp(data->entries[i].name, name, len) == 0 &&
			data->entries[i].name[len] == '\0') {
			return data->entries[i].inode;
		}
	}
	return NULL;
}

void tmpfs_init_dir(struct inode *inode) {
	struct dir_data *data = memory_alloc();

	data->n_entries = 0;

	inode->type = INODE_DIRECTORY;
	inode->data = data;
	inode->create_child = tmpfs_dir_create_child;
	inode->find_child = tmpfs_dir_find_child;
}

struct file_header {
	void *data;
	uint64_t size;
	// Seems like a bit of waste...
};

_Static_assert(sizeof(struct file_header) <= 4096, "");

ssize_t tmpfs_file_read(struct inode *inode, size_t *offset, void *data, size_t count) {
	struct file_header *header = inode->data;
	uint8_t *user_data = data,
		*file_data = (uint8_t *)header->data + *offset;

	if (*offset + count > header->size)
		count = header->size - *offset;

	for (size_t i = 0; i < count; i++)
		*(user_data++) = *(file_data++);

	*offset += count;

	return count;
}

ssize_t tmpfs_file_write(struct inode *inode, size_t *offset, const void *data, size_t count) {
	ERROR("Read only");
	return 0;
}

ssize_t tmpfs_file_size(struct inode *inode) {
	struct file_header *header = inode->data;

	return header->size;
}

void tmpfs_init_file(struct inode *inode,
					 void *data, size_t size) {
	struct file_header *header = memory_alloc();

	header->data = data;
	header->size = size;

	inode->type = INODE_FILE;
	inode->data = header;
	inode->read = tmpfs_file_read;
	inode->write = tmpfs_file_write;
	inode->size = tmpfs_file_size;
}
