#ifndef PIPE_H
#define PIPE_H

#include "vfs.h"

int create_pipe(struct file **read, struct file **write);

#endif
