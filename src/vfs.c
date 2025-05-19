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

void vfs_init(void) {
	fs_root = vfs_new_inode();
	fs_root->type = INODE_DIRECTORY;
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
	for (int i = 0; i < table->n_entries; i++) {
		table->entries[i] = src->entries[i];
		if (table->entries[i].file)
			table->entries[i].file->ref_count++;
	}

	return table;
}

struct file *fd_table_get_file(struct fd_table *fd_table, int fd) {
	if (fd > fd_table->n_entries) {
		print("Error: invalid fd: ");
		print_int(fd);
		print("\n");
		return NULL;
	}
	return fd_table->entries[fd].file;
}

void fd_table_set_standard_streams(struct fd_table *fd_table, struct file *stdin, struct file *stdout,
                                   struct file *stderr) {
	if (fd_table->n_entries < 3)
		fd_table->n_entries = 3;
	fd_table->entries[0].file = stdin;
	fd_table->entries[1].file = stdout;
	fd_table->entries[2].file = stderr;
}

ssize_t vfs_read_file(struct file *file, void *data, size_t count) {
	return file->inode->read(file->inode, &file->file_offset, data, count);
}

ssize_t vfs_write_file(struct file *file, const void *data, size_t count) {
	return file->inode->write(file->inode, &file->file_offset, data, count);
}

size_t vfs_lseek(struct file *file, size_t offset, int whence) {
	size_t size = file->inode->size(file->inode);

	switch (whence) {
	case SEEK_SET:
		if (offset > size)
			offset = size;
		return file->file_offset = offset;

	default: ERROR("Not implemented\n");
	}
	return 0;
}

struct file *vfs_open_inode(struct inode *inode, unsigned char access_mode) {
	struct file *file = kmalloc(sizeof *file);
	*file = (struct file) { .ref_count = 1, .inode = inode, .access_mode = access_mode };
	return file;
}

struct file *vfs_open(const char *filename, unsigned char access_mode) {
	struct inode *inode = vfs_resolve(NULL, filename);
	if (!inode)
		return NULL;
	return vfs_open_inode(inode, access_mode);
}

void vfs_close_file(struct file *file) {
	file->ref_count--;
	if (file->ref_count <= 0)
		kfree(file);
}

int fd_table_assign_open_file(struct fd_table *fd_table, struct file *file) {
	// Iterate through fd_table and find something open.
	for (int i = 0; i < fd_table->n_entries; i++) {
		if (!fd_table->entries[i].file) {
			fd_table->entries[i].file = file;
			return i;
		}
	}

	int new_entry = fd_table->n_entries++;
	fd_table->entries[new_entry].file = file;
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

ssize_t vfs_fill_dirent(struct file *file, struct dirent *dirent) {
	if (file->inode->type != INODE_DIRECTORY)
		return -1;

	int result = file->inode->iterate(file->inode, &file->file_offset, vfs_filldir, dirent);

	if (result == 0)
		return 0;

	dirent->d_reclen = sizeof(struct dirent) + strlen(dirent->d_name) + 1;

	return dirent->d_reclen;
}
