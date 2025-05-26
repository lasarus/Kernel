#include "pipe.h"
#include "circular_buffer.h"
#include "kmalloc.h"
#include "scheduler.h"
#include "vfs.h"
#include "vga_text.h"

struct pipe {
	int write_count, read_count;
	struct circular_buffer circular_buffer;
	struct task_wait read_wait;
	struct task_wait write_wait;
};

static ssize_t pipe_read(struct inode *inode, size_t *offset, void *data, size_t count) {
	struct pipe *pipe = inode->data;

	while (circular_buffer_size(&pipe->circular_buffer) == 0 && pipe->write_count > 0)
		scheduler_wait(&pipe->read_wait);

	if (circular_buffer_size(&pipe->circular_buffer) == 0 && pipe->write_count == 0)
		return 0;

	ssize_t ret = circular_buffer_read(&pipe->circular_buffer, data, count);
	scheduler_unwait(&pipe->write_wait);
	return ret;
}

static ssize_t pipe_write(struct inode *inode, size_t *offset, const void *data, size_t count) {
	struct pipe *pipe = inode->data;
	while (circular_buffer_size(&pipe->circular_buffer) == pipe->circular_buffer.size)
		scheduler_wait(&pipe->write_wait);

	ssize_t ret = circular_buffer_write(&pipe->circular_buffer, data, count);
	scheduler_unwait(&pipe->read_wait);
	return ret;
}

static int pipe_open(struct file *file, struct inode *inode) {
	struct pipe *pipe = inode->data;

	if (file->access_mode & O_RDONLY) {
		pipe->read_count++;
	}

	if (file->access_mode & O_WRONLY) {
		pipe->write_count++;
		scheduler_unwait(&pipe->read_wait);
	}

	return 0;
}

static int pipe_close(struct file *file, struct inode *inode) {
	struct pipe *pipe = inode->data;

	if (file->access_mode & O_RDONLY) {
		pipe->read_count--;
	}

	if (file->access_mode & O_WRONLY) {
		pipe->write_count--;

		if (pipe->write_count == 0) {
			scheduler_unwait(&pipe->read_wait);
		}
	}

	if (pipe->read_count == 0 && pipe->write_count == 0) {
		inode->data = NULL;
		inode->operations = NULL;
		inode->file_operations = NULL;
		kfree(pipe);
	}

	return 0;
}

static struct inode_operations operations = { .read = pipe_read, .write = pipe_write };
static struct file_operations file_operations = {
	.open = pipe_open,
	.close = pipe_close,
};

int create_pipe(struct file **read, struct file **write) {
	struct inode *pipe_inode = vfs_new_inode();
	struct pipe *pipe = kmalloc(sizeof *pipe);
	*pipe = (struct pipe) { 0 };

	circular_buffer_init(&pipe->circular_buffer, 4096);

	pipe_inode->operations = &operations;
	pipe_inode->file_operations = &file_operations;
	pipe_inode->data = pipe;

	struct path_node *path_node = vfs_create_path_node(pipe_inode, "pipe");

	*read = vfs_open_path_node(path_node, O_RDONLY);
	*write = vfs_open_path_node(path_node, O_WRONLY);

	return 0;
}
