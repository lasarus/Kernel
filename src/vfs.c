#include "vfs.h"
#include "common.h"
#include "kmalloc.h"
#include "memory.h"
#include "vga_text.h"

static struct inode *fs_root;
static struct inode_operations *device_table[MAX_DRIVERS] = { 0 };

struct inode *vfs_new_inode(void) {
	static uint32_t inode_counter = 0;
	struct inode *inode = kmalloc(sizeof *inode);
	*inode = (struct inode) { 0 };
	return inode;
}

void vfs_init(void) {
	fs_root = vfs_new_inode();
	fs_root->type = INODE_DIRECTORY;
}

static const char *next_component(const char *path, char *component) {
	size_t i = 0;
	for (; path[i] && path[i] != '/'; i++) {
		component[i] = path[i];
	}
	component[i] = '\0';
	return path + i;
}

static const char *flush_slash(const char *path) {
	while (*path == '/')
		path++;
	return path;
}

struct inode *vfs_resolve(struct inode *root, const char *path) {
	const char *opath = path;
	if (*path == '/') {
		root = fs_root;
		path++;
	}

	if (!root)
		return NULL;

	path = flush_slash(path);

	while (*path) {
		if (root->type != INODE_DIRECTORY)
			return NULL;

		char component[FILENAME_MAX];
		path = next_component(path, component);
		root = root->operations->lookup(root, component);

		if (!root)
			return NULL;

		path = flush_slash(path);
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
	return file->inode->operations->read(file->inode, &file->file_offset, data, count);
}

ssize_t vfs_write_file(struct file *file, const void *data, size_t count) {
	return file->inode->operations->write(file->inode, &file->file_offset, data, count);
}

size_t vfs_lseek(struct file *file, size_t offset, int whence) {
	size_t size = file->inode->operations->size(file->inode);

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

void split_path(const char *path, char *parent_path, char *filename) {
	parent_path[0] = '.';
	parent_path[1] = '\0';
	filename[0] = '\0';

	if (path == NULL)
		return;

	size_t filename_start = 0;
	for (size_t i = 0; path[i]; i++) {
		if (path[i] == '/' && path[i + 1]) {
			filename_start = i + 1;
		}
	}

	// Copy the to the filename from index_of_last_slash and forward.
	size_t filename_index = 0;
	for (size_t i = filename_start; path[i] && path[i] != '/'; i++) {
		filename[filename_index++] = path[i];
	}
	filename[filename_index++] = '\0';

	// Find last index of parent_path, by starting at filename_start, and moving
	// backward until no slashes are found any more.
	size_t parent_path_end = filename_start;
	while (parent_path_end-- > 0)
		if (path[parent_path_end] != '/')
			break;

	parent_path_end++;

	if (parent_path_end == 0) {
		parent_path[0] = path[0] == '/' ? '/' : '.';
		parent_path[1] = '\0';
	} else {
		for (size_t i = 0; i < parent_path_end; i++) {
			parent_path[i] = path[i];
		}
		parent_path[parent_path_end] = '\0';
	}
}

struct file *vfs_open(const char *path, unsigned char access_mode) {
	struct inode *inode = vfs_resolve(NULL, path);
	if (!inode) {
		if (!(access_mode & O_CREAT))
			return NULL;

		char parent_path[PATH_MAX], filename[PATH_MAX];
		split_path(path, parent_path, filename);

		// TODO: Handle correct root?
		struct inode *parent = vfs_resolve(NULL, parent_path);
		if (parent->operations->create(parent, filename)) {
			kprintf("Failed to create child node\n");
			hang_kernel();
		}
		struct inode *new_inode = parent->operations->lookup(parent, filename);
		return vfs_open_inode(new_inode, access_mode);
	}
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

	int result = file->inode->operations->iterate(file->inode, &file->file_offset, vfs_filldir, dirent);

	if (result == 0)
		return 0;

	dirent->d_reclen = sizeof(struct dirent) + strlen(dirent->d_name) + 1;

	return dirent->d_reclen;
}

void vfs_register_driver(int major, struct inode_operations *operations) {
	device_table[major] = operations;
}

void vfs_mknod_helper(struct inode *inode, enum inode_type type, int major, int minor) {
	inode->type = type;
	inode->major = major;
	inode->minor = minor;

	inode->operations = device_table[major];
}
