#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include "common.h"

struct circular_buffer {
	size_t size, write_index, read_index;
	uint8_t *buffer;
};

void circular_buffer_init(struct circular_buffer *circular_buffer, size_t capacity);
size_t circular_buffer_size(struct circular_buffer *circular_buffer);
ssize_t circular_buffer_write(struct circular_buffer *circular_buffer, const uint8_t *data, size_t size);
ssize_t circular_buffer_read(struct circular_buffer *circular_buffer, uint8_t *data, size_t max_size);

#endif
