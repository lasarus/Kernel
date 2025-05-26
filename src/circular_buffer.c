#include "circular_buffer.h"
#include "kmalloc.h"

static size_t buffer_data_size(const struct circular_buffer *buffer) {
	if (buffer->write_index >= buffer->read_index)
		return buffer->write_index - buffer->read_index;
	return buffer->size - buffer->read_index + buffer->write_index;
}

static size_t buffer_free_space(const struct circular_buffer *buffer) {
	return buffer->size - buffer_data_size(buffer);
}

size_t circular_buffer_size(struct circular_buffer *circular_buffer) {
	return buffer_data_size(circular_buffer);
}

ssize_t circular_buffer_write(struct circular_buffer *circular_buffer, const uint8_t *data, size_t size) {
	size_t free_space = buffer_free_space(circular_buffer);
	size_t to_write = size < free_space ? size : free_space;

	size_t first_chunk = to_write;
	if (circular_buffer->write_index + first_chunk > circular_buffer->size)
		first_chunk = circular_buffer->size - circular_buffer->write_index;

	memcpy(&circular_buffer->buffer[circular_buffer->write_index], data, first_chunk);

	size_t second_chunk = to_write - first_chunk;
	if (second_chunk > 0)
		memcpy(circular_buffer->buffer, data + first_chunk, second_chunk);

	// TODO: modulo is slow, perhaps set size to always be a power of 2?
	circular_buffer->write_index = (circular_buffer->write_index + to_write) % circular_buffer->size;
	return (ssize_t)to_write;
}

ssize_t circular_buffer_read(struct circular_buffer *circular_buffer, uint8_t *data, size_t max_size) {
	size_t data_available = buffer_data_size(circular_buffer);
	size_t to_read = max_size < data_available ? max_size : data_available;

	size_t first_chunk = to_read;
	if (circular_buffer->read_index + first_chunk > circular_buffer->size)
		first_chunk = circular_buffer->size - circular_buffer->read_index;

	memcpy(data, &circular_buffer->buffer[circular_buffer->read_index], first_chunk);

	size_t second_chunk = to_read - first_chunk;
	if (second_chunk > 0)
		memcpy(data + first_chunk, circular_buffer->buffer, second_chunk);

	circular_buffer->read_index = (circular_buffer->read_index + to_read) % circular_buffer->size;
	return (ssize_t)to_read;
}

void circular_buffer_init(struct circular_buffer *circular_buffer, size_t capacity) {
	*circular_buffer = (struct circular_buffer) {
		.buffer = kmalloc(capacity),
		.size = capacity,
		.read_index = 0,
		.write_index = 0,
	};
}
