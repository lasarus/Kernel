#ifndef TMPFS_H
#define TMPFS_H

#include "vfs.h"

void tmpfs_init_dir(struct inode *inode);
void tmpfs_init_file(struct inode *inode, void *data, size_t size);

#endif
