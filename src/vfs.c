#include "vfs.h"
#include "kmalloc.h"
#include "memory.h"
#include "vga_text.h"

struct inode *vfs_new_inode(void) {
	static uint32_t inode_counter = 0;
	struct inode *inode = kmalloc(sizeof *inode);
	*inode = (struct inode) { .id = inode_counter++ };
	return inode;
}

static struct inode *fs_root;
static struct file_table file_table;

void vfs_init(void) {
	fs_root = vfs_new_inode();
	fs_root->type = INODE_DIRECTORY;

	file_table.n_entries = 0;
}

struct inode *vfs_resolve(struct inode *root, const char *path) {
	const char *opath = path;
	if (*path == '/') {
		root = fs_root;
		path++;
	}

	if (!root)
		return NULL;

	while (*path) {
		if (root->type != INODE_DIRECTORY)
			return NULL;

		// Find next path.
		const char *start = path;
		for (; *path && *path != '/'; path++)
			;
		int len = path - start;

		if (len == 0) { // Special case: /dir//name.txt
			path++;
			continue;
		}

		root = root->find_child(root, start, len);

		if (!root)
			return NULL;
	}

	return root;
}

struct fd_table *vfs_init_fd_table(void) {
	struct fd_table *table = memory_alloc();

	table->n_entries = 0;

	return table;
}

struct fd_table *vfs_copy_fd_table(struct fd_table *src) {
	struct fd_table *table = memory_alloc();

	table->n_entries = src->n_entries;
	for (int i = 0; i < table->n_entries; i++)
		table->entries[i] = src->entries[i];

	return table;
}

int fd_table_get_file(struct fd_table *fd_table, int fd) {
	if (fd > fd_table->n_entries) {
		print("Error: invalid fd: ");
		print_int(fd);
		print("\n");
	}
	return fd_table->entries[fd].index;
}

void fd_table_set_standard_streams(struct fd_table *fd_table, int stdin, int stdout, int stderr) {
	if (fd_table->n_entries < 3)
		fd_table->n_entries = 3;
	fd_table->entries[0].index = stdin;
	fd_table->entries[1].index = stdout;
	fd_table->entries[2].index = stderr;
}

ssize_t vfs_read_file(int fd, void *data, size_t count) {
	if (fd == -1)
		ERROR("Invalid file handle\n");
	struct file_table_entry *entry = &file_table.entries[fd];
	struct inode *inode = entry->inode;

	return inode->read(inode, &entry->file_offset, data, count);
}

ssize_t vfs_write_file(int fd, const void *data, size_t count) {
	struct file_table_entry *entry = file_table.entries + fd;
	struct inode *inode = entry->inode;

	return inode->write(inode, &entry->file_offset, data, count);
}

size_t vfs_lseek(int fd, size_t offset, int whence) {
	struct file_table_entry *entry = file_table.entries + fd;
	struct inode *inode = entry->inode;

	size_t size = inode->size(inode);

	switch (whence) {
	case SEEK_SET:
		if (offset > size)
			offset = size;
		return entry->file_offset = offset;

	default: ERROR("Not implemented\n");
	}
	return 0;
}

int vfs_open_inode(struct inode *inode, unsigned char access_mode) {
	int new_idx = -1;
	for (int i = 0; i < file_table.n_entries; i++) {
		if (file_table.entries[i].dead) {
			new_idx = i;
			break;
		}
	}

	if (new_idx == -1)
		new_idx = file_table.n_entries++;

	struct file_table_entry *entry = file_table.entries + new_idx;

	*entry = (struct file_table_entry) { .inode = inode, .access_mode = access_mode };

	return new_idx;
}

int vfs_open(const char *filename, unsigned char access_mode) {
	struct inode *inode = vfs_resolve(NULL, filename);
	if (!inode)
		return -1;
	return vfs_open_inode(inode, access_mode);
}

void vfs_close_file(int file) {
	file_table.entries[file].dead = 1;
}

int fd_table_assign_open_file(struct fd_table *fd_table, int fd) {
	// Iterate through fd_table and find something open.
	for (int i = 0; i < fd_table->n_entries; i++) {
		if (fd_table->entries[i].index == -1) {
			fd_table->entries[i].index = fd;
			return i;
		}
	}

	int new_entry = fd_table->n_entries++;
	fd_table->entries[new_entry].index = fd;
	fd_table->entries[new_entry].close_on_exec = 0;

	return new_entry;
}

struct dirent {
	unsigned long d_ino; // Not used
	long d_off; // Not used
	unsigned short d_reclen;
	unsigned char d_type;
	char d_name[];
};

struct vfs_filldir_context {
	struct dirent *dirent;
};

int vfs_filldir(void *context, const char *name, size_t name_length, unsigned char type) {
	struct dirent *dirent = context;

	memcpy(dirent->d_name, name, name_length);
	dirent->d_type = type;
	dirent->d_name[name_length] = '\0';

	return 0;
}

ssize_t vfs_fill_dirent(int fd, struct dirent *dirent) {
	struct file_table_entry *entry = file_table.entries + fd;
	struct inode *inode = entry->inode;

	if (inode->type != INODE_DIRECTORY)
		return -1;

	int result = inode->iterate(inode, &entry->file_offset, vfs_filldir, dirent);

	if (result == 0)
		return 0;

	dirent->d_reclen = sizeof(struct dirent) + strlen(dirent->d_name) + 1;

	return dirent->d_reclen;
}
