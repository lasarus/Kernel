#include "tmpfs.h"
#include "kmalloc.h"
#include "memory.h"
#include "vfs.h"
#include "vga_text.h"

struct dir_entry {
	char name[FILENAME_MAX];
	struct inode *inode;
};

#define N_DIR_ENTRIES (4096 / sizeof(struct dir_entry))

struct dir_header {
	int n_entries;
	struct dir_entry entries[N_DIR_ENTRIES];
};

struct file_header {
	void *data;
	uint64_t size;
};

static ssize_t tmpfs_read(struct inode *inode, size_t *offset, void *data, size_t count);
static ssize_t tmpfs_write(struct inode *inode, size_t *offset, const void *data, size_t count);
static ssize_t tmpfs_size(struct inode *inode);

static struct inode *tmpfs_lookup(struct inode *dir, const char *name);
static int tmpfs_iterate(struct inode *dir, size_t *offset, filldir_t filldir, void *context);
static int tmpfs_create(struct inode *dir, const char *name);
static int tmpfs_mkdir(struct inode *dir, const char *name);
static int tmpfs_mknod(struct inode *dir, const char *name, enum inode_type type, int major, int minor);

static struct inode_operations operations = {
	.read = tmpfs_read,
	.write = tmpfs_write,
	.size = tmpfs_size,
	.lookup = tmpfs_lookup,
	.iterate = tmpfs_iterate,
	.create = tmpfs_create,
	.mkdir = tmpfs_mkdir,
	.mknod = tmpfs_mknod,
};

_Static_assert(N_DIR_ENTRIES > 14, "");
_Static_assert(sizeof(struct dir_header) <= 4096, "");

static struct inode *new_inode(struct inode *dir, const char *name) {
	struct dir_header *data = dir->data;
	struct dir_entry *entry = &data->entries[data->n_entries++];
	strcpy(entry->name, name);
	entry->inode = vfs_new_inode();
	return entry->inode;
}

static ssize_t tmpfs_read(struct inode *inode, size_t *offset, void *data, size_t count) {
	struct file_header *header = inode->data;
	uint8_t *user_data = data, *file_data = (uint8_t *)header->data + *offset;

	if (*offset + count > header->size)
		count = header->size - *offset;

	for (size_t i = 0; i < count; i++)
		*(user_data++) = *(file_data++);

	*offset += count;

	return (ssize_t)count;
}

static ssize_t tmpfs_write(struct inode *inode, size_t *offset, const void *data, size_t count) {
	if (inode->type != INODE_FILE)
		return 0;
	struct file_header *header = inode->data;
	if (header->size != 0)
		return 0;
	header->data = (void *)data; // TODO: THIS IS BAD.
	header->size = count;
	return (ssize_t)count;
}

static ssize_t tmpfs_size(struct inode *inode) {
	if (inode->type != INODE_FILE)
		return 0;
	struct file_header *header = inode->data;
	return (ssize_t)header->size;
}

static struct inode *tmpfs_lookup(struct inode *dir, const char *name) {
	if (dir->type != INODE_DIRECTORY)
		return NULL;
	struct dir_header *data = dir->data;
	for (int i = 0; i < data->n_entries; i++) {
		if (strcmp(data->entries[i].name, name) == 0)
			return data->entries[i].inode;
	}
	return NULL;
}

static int tmpfs_iterate(struct inode *dir, size_t *offset, filldir_t filldir, void *context) {
	if (dir->type != INODE_DIRECTORY)
		return 1;
	struct dir_header *data = dir->data;
	if (*offset == 0) {
		filldir(context, ".", 1, 4);
	} else if (*offset == 1) {
		filldir(context, "..", 2, 4);
	} else if (*offset < (size_t)data->n_entries + 2) {
		struct dir_entry *entry = &data->entries[*offset - 2];
		size_t length = strlen(entry->name);
		unsigned char type = entry->inode->type;
		filldir(context, entry->name, length, type);
	} else {
		return 0;
	}

	(*offset)++;
	return 1;
}

static int tmpfs_create(struct inode *dir, const char *name) {
	if (dir->type != INODE_DIRECTORY)
		return 1;
	struct inode *inode = new_inode(dir, name);
	if (!inode)
		return 1;

	inode->type = INODE_FILE;
	inode->operations = &operations;
	struct file_header *header = kmalloc(sizeof *header);
	*header = (struct file_header) {
		.data = NULL,
		.size = 0,
	};
	inode->data = header;

	return 0;
}

static int tmpfs_mkdir(struct inode *dir, const char *name) {
	if (dir->type != INODE_DIRECTORY)
		return -1;
	// Check path already exists
	if (tmpfs_lookup(dir, name))
		return -1;

	struct inode *inode = new_inode(dir, name);
	if (!inode)
		return -1;

	inode->type = INODE_DIRECTORY;
	inode->operations = &operations;
	struct dir_header *header = memory_alloc();
	*header = (struct dir_header) {
		.n_entries = 0,
	};
	inode->data = header;

	return 0;
}

static int tmpfs_mknod(struct inode *dir, const char *name, enum inode_type type, int major, int minor) {
	if (dir->type != INODE_DIRECTORY)
		return 1;
	struct inode *inode = new_inode(dir, name);
	if (!inode)
		return 1;

	vfs_mknod_helper(inode, type, major, minor);

	return 0;
}

void tmpfs_init(struct inode *root) {
	root->type = INODE_DIRECTORY;
	root->operations = &operations;
	struct dir_header *header = memory_alloc();
	*header = (struct dir_header) {
		.n_entries = 0,
	};
	root->data = header;
}
