#ifndef VFS_H
#define VFS_H

#include "common.h"

#define PATH_MAX 1024
#define FILENAME_MAX 256
#define MAX_DRIVERS 256

struct inode;

typedef int (*filldir_t)(void *context, const char *name, size_t name_length, unsigned char type);

enum inode_type {
	INODE_FILE,
	INODE_DIRECTORY,
	INODE_CHAR_DEVICE,
};

struct inode_operations {
	ssize_t (*read)(struct inode *inode, size_t *offset, void *data, size_t count);
	ssize_t (*write)(struct inode *inode, size_t *offset, const void *data, size_t count);
	ssize_t (*size)(struct inode *inode);

	struct inode *(*lookup)(struct inode *dir, const char *name);
	int (*iterate)(struct inode *dir, size_t *offset, filldir_t filldir, void *context);
	int (*create)(struct inode *dir, const char *name);
	int (*mkdir)(struct inode *dir, const char *name);
	int (*mknod)(struct inode *dir, const char *name, enum inode_type type, int major, int minor);
};

struct inode {
	enum inode_type type;
	struct inode_operations *operations;
	void *data;
	int major, minor;
};

struct path_node {
	struct inode *inode;
	char name[FILENAME_MAX];
	struct path_node *parent;
};

struct file {
	int ref_count;
	size_t file_offset;
	size_t status_flags;
	unsigned char access_mode; // Should be read/write/read-write.
	unsigned char dead; // Should be read/write/read-write.
	struct path_node *path_node;
};

void vfs_init(void);

struct inode *vfs_new_inode(void);

// See https://en.wikipedia.org/wiki/File_descriptor for more information about
// the structure of the file descriptor table, file table, and inode table.
struct fd_table { // File descriptor table
	int n_entries;
	struct fd_table_entry {
		unsigned char close_on_exec; // Currently unused.
		struct file *file;
	} entries[4088 / sizeof(struct fd_table_entry)];
};

struct fd_table *vfs_init_fd_table(void);
struct fd_table *vfs_copy_fd_table(struct fd_table *src);
struct file *fd_table_get_file(struct fd_table *fd_table, int fd);
void fd_table_set_standard_streams(struct fd_table *fd_table, struct file *stdin, struct file *stdout,
                                   struct file *stderr);
int fd_table_assign_open_file(struct fd_table *fd_table, struct file *file);

enum {
	O_RDONLY = 1 << 0,
	O_WRONLY = 1 << 1,
	O_CREAT = 1 << 2,
	O_RDWR = O_RDONLY | O_WRONLY,
};

struct file *vfs_open_path_node(struct path_node *path_node, unsigned char access_mode);
struct file *vfs_open(struct path_node *root, const char *path, unsigned char access_mode);
void vfs_close_file(struct file *file);

struct dirent;
ssize_t vfs_fill_dirent(struct file *file, struct dirent *dirent);

void vfs_register_driver(int major, struct inode_operations *operations);
void vfs_mknod_helper(struct inode *inode, enum inode_type type, int major, int minor);

enum {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};

size_t vfs_lseek(struct file *file, size_t offset, int whence);

struct path_node *vfs_resolve(struct path_node *root, const char *path);
ssize_t vfs_read(struct file *file, void *data, size_t count);
ssize_t vfs_write(struct file *file, const void *data, size_t count);
ssize_t vfs_size(struct file *file);

struct path_node *vfs_lookup(struct path_node *dir, const char *name);
int vfs_iterate(struct path_node *path_node, size_t *offset, filldir_t filldir, void *context);
int vfs_create(struct path_node *path_node, const char *name);
int vfs_mkdir(struct path_node *root, const char *name);
int vfs_mknod(struct path_node *root, const char *name, enum inode_type type, int major, int minor);

size_t get_path(struct path_node *path_node, char *buffer, size_t size);

// Help function for parsing paths.
const char *next_component(const char *path, char *component);

#endif
