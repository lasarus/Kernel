#ifndef TAR_H
#define TAR_H

#include "vfs.h"

void tar_extract(struct path_node *root, uint8_t *data, size_t size);

#endif
