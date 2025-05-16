#ifndef VFS_H
#define VFS_H

#include "common.h"

struct inode {
	enum {
		INODE_FILE,
		INODE_DIRECTORY,
		INODE_CHAR_DEVICE,
	} type;

	uint32_t id;

	union {
		struct {
			ssize_t (*read)(struct inode *inode, size_t *offset, void *data, size_t count);
			ssize_t (*write)(struct inode *inode, size_t *offset, const void *data, size_t count);
			void (*open)(struct inode *inode, int file);
			void (*close)(struct inode *inode, int file);
			ssize_t (*size)(struct inode *inode);
		};
		struct {
			struct inode *(*create_child)(struct inode *inode, const char *name, int len);
			struct inode *(*find_child)(struct inode *inode, const char *name, int len);
		};
	};

	void *data;
};

void vfs_init(void);

struct inode *vfs_new_inode(void);
struct inode *vfs_resolve(struct inode *root, const char *path);

// See https://en.wikipedia.org/wiki/File_descriptor for more information about
// the structure of the file descriptor table, file table, and inode table.
struct fd_table { // File descriptor table
	int n_entries;
	struct fd_table_entry {
		unsigned char close_on_exec; // Currently unused.
		int index;
	} entries[4088 / sizeof(struct fd_table_entry)];
};

_Static_assert(sizeof(struct fd_table) <= 4096, "");

struct fd_table *vfs_init_fd_table(void);
struct fd_table *vfs_copy_fd_table(struct fd_table *src);
int fd_table_get_file(struct fd_table *fd_table, int fd);
void fd_table_set_standard_streams(struct fd_table *fd_table, int stdin, int stdout, int stderr);

enum {
	O_RDONLY = 1 << 0,
	O_WRONLY = 1 << 1,
	O_RDWR = O_RDONLY | O_WRONLY,
};

struct file_table {
	int n_entries;
	struct file_table_entry {
		size_t file_offset;
		size_t status_flags;
		unsigned char access_mode; // Should be read/write/read-write.
		unsigned char dead; // Should be read/write/read-write.
		struct inode *inode;
	} entries[1000];
};

ssize_t vfs_read_file(int fd, void *data, size_t count);
ssize_t vfs_write_file(int fd, const void *data, size_t count);

int vfs_open_inode(struct inode *inode, unsigned char access_mode);
int vfs_open(const char *filename, unsigned char access_mode);
void vfs_close_file(int file);

enum {
	SEEK_SET,
	SEEK_CUR,
	SEEK_END,
};

size_t vfs_lseek(int fd, size_t offset, int whence);

#endif
