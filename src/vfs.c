#include "vfs.h"
#include "common.h"
#include "kmalloc.h"
#include "vga_text.h"

static struct path_node *fs_root;
static struct inode_operations *device_table[MAX_DRIVERS] = { 0 };

struct inode *vfs_new_inode(void) {
	static uint32_t inode_counter = 0;
	struct inode *inode = kmalloc(sizeof *inode);
	*inode = (struct inode) { 0 };
	return inode;
}

static struct path_node *vfs_new_path_node(struct path_node *parent, struct inode *inode, const char *name) {
	struct path_node *path_node = kmalloc(sizeof *path_node);
	path_node->inode = inode;
	path_node->parent = parent;
	strcpy(path_node->name, name);
	return path_node;
}

void vfs_init(void) {
	fs_root = vfs_new_path_node(NULL, vfs_new_inode(), "");
}

static const char *flush_slash(const char *path) {
	while (*path == '/')
		path++;
	return path;
}

const char *next_component(const char *path, char *component) {
	size_t i = 0;
	for (; path[i] && path[i] != '/'; i++) {
		component[i] = path[i];
	}
	component[i] = '\0';
	path = path + i;
	path = flush_slash(path);
	return path;
}

static struct path_node *split_path(struct path_node *root, const char *path, char *filename) {
	char parent_path[PATH_MAX];
	parent_path[0] = '.';
	parent_path[1] = '\0';
	filename[0] = '\0';

	if (path == NULL)
		return NULL;

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

	return vfs_resolve(root, parent_path);
}

struct fd_table *vfs_init_fd_table(void) {
	struct fd_table *table = kmalloc(sizeof *table);
	*table = (struct fd_table) { .n_entries = 0 };
	return table;
}

struct fd_table *vfs_copy_fd_table(struct fd_table *src) {
	struct fd_table *table = kmalloc(sizeof *table);
	*table = *src;

	for (int i = 0; i < table->n_entries; i++) {
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

size_t vfs_lseek(struct file *file, size_t offset, int whence) {
	struct inode *inode = file->path_node->inode;
	size_t size = inode->operations->size(inode);

	switch (whence) {
	case SEEK_SET:
		if (offset > size)
			offset = size;
		return file->file_offset = offset;

	default: ERROR("Not implemented\n");
	}
	return 0;
}

struct file *vfs_open(struct path_node *root, const char *path, unsigned char access_mode) {
	struct path_node *path_node = vfs_resolve(root, path);
	if (!path_node) {
		if (!(access_mode & O_CREAT))
			return NULL;

		char filename[PATH_MAX];
		struct path_node *parent = split_path(root, path, filename);
		if (vfs_create(parent, filename)) {
			kprintf("Failed to create child node\n");
			hang_kernel();
		}
		struct path_node *new_path_node = vfs_lookup(parent, filename);
		return vfs_open_path_node(new_path_node, access_mode);
	}
	return vfs_open_path_node(path_node, access_mode);
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
	if (file->path_node->inode->type != INODE_DIRECTORY)
		return -1;

	int result = vfs_iterate(file->path_node, &file->file_offset, vfs_filldir, dirent);

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

struct path_node *vfs_resolve(struct path_node *root, const char *path) {
	const char *opath = path;
	if (*path == '/') {
		root = fs_root;
		path++;
	}

	if (!root)
		return NULL;

	path = flush_slash(path);

	while (*path) {
		if (root->inode->type != INODE_DIRECTORY)
			return NULL;

		char component[FILENAME_MAX];
		path = next_component(path, component);

		if (strcmp(component, "..") == 0) {
			if (root->parent)
				root = root->parent;
		} else if (strcmp(component, ".") == 0) {
			// Do nothing.
		} else {
			root = vfs_lookup(root, component);
		}

		if (!root)
			return NULL;
	}

	return root;
}

struct file *vfs_open_path_node(struct path_node *path_node, unsigned char access_mode) {
	struct file *file = kmalloc(sizeof *file);
	*file = (struct file) { .ref_count = 1, .path_node = path_node, .access_mode = access_mode };
	return file;
}

ssize_t vfs_read(struct file *file, void *data, size_t count) {
	struct inode *inode = file->path_node->inode;
	return inode->operations->read(inode, &file->file_offset, data, count);
}

ssize_t vfs_write(struct file *file, const void *data, size_t count) {
	struct inode *inode = file->path_node->inode;
	return inode->operations->write(inode, &file->file_offset, data, count);
}

ssize_t vfs_size(struct file *file) {
	struct inode *inode = file->path_node->inode;
	return inode->operations->size(inode);
}

int vfs_iterate(struct path_node *path_node, size_t *offset, filldir_t filldir, void *context) {
	struct inode *inode = path_node->inode;
	return inode->operations->iterate(inode, offset, filldir, context);
}

int vfs_create(struct path_node *path_node, const char *name) {
	struct inode *inode = path_node->inode;
	return inode->operations->create(inode, name);
}

int vfs_mkdir(struct path_node *root, const char *name) {
	char filename[FILENAME_MAX];
	struct path_node *path_node = split_path(root, name, filename);
	struct inode *inode = path_node->inode;
	return inode->operations->mkdir(inode, filename);
}

int vfs_mknod(struct path_node *root, const char *name, enum inode_type type, int major, int minor) {
	char filename[FILENAME_MAX];
	struct path_node *path_node = split_path(root, name, filename);
	struct inode *inode = path_node->inode;
	return inode->operations->mknod(inode, filename, type, major, minor);
}

struct path_node *vfs_lookup(struct path_node *dir, const char *name) {
	struct inode *inode = dir->inode;
	struct inode *child = inode->operations->lookup(inode, name);
	if (!child)
		return NULL;
	return vfs_new_path_node(dir, child, name);
}

size_t get_path(struct path_node *path_node, char *buffer, size_t size) {
#define MAX_NEST 256
	size_t nodes_size = 0;
	static struct path_node *nodes[MAX_NEST];

	while (path_node) {
		nodes[nodes_size++] = path_node;
		if (nodes_size >= MAX_NEST) {
			return 0;
		}

		path_node = path_node->parent;
	}

	size_t written = 0;

	for (size_t i = nodes_size; i-- > 0;) {
		const char *name = nodes[i]->name;
		size_t len = strlen(name);
		for (size_t j = 0; j < len; j++) {
			buffer[written++] = name[j];
			if (written >= size)
				return 0;
		}

		if (nodes[i]->parent == NULL || i != 0) {
			buffer[written++] = '/';
			if (written >= size)
				return 0;
		}
	}
	buffer[written++] = '\0';
	if (written >= size)
		return 0;

	return written;
}
