#include "tar.h"

#define BLOCK_SIZE 512

static size_t parse_octal(const char *str, size_t len) {
	size_t value = 0;
	for (size_t i = 0; i < len; i++) {
		if (str[i] >= '0' && str[i] <= '7') {
			value *= 8;
			value += str[i] - '0';
		}
	}
	return value;
}

void tar_extract(struct path_node *root, uint8_t *data, size_t size) {
	for (size_t offset = 0; offset + BLOCK_SIZE <= size;) {
		uint8_t *header = data + offset;

		if (header[0] == 0)
			break;

		char name[101];
		memcpy(name, header, 100);
		name[100] = '\0';

		size_t file_size = parse_octal((char *)(header + 124), 12);
		uint8_t typeflag = header[156];

		if (typeflag == '0' || typeflag == '\0') {
			struct file *file = vfs_open(root, name, O_WRONLY | O_CREAT);
			vfs_write(file, data + offset + BLOCK_SIZE, file_size);
			vfs_close_file(file);
		} else if (typeflag == '5') {
			vfs_mkdir(root, name);
		}

		offset += BLOCK_SIZE + ((file_size + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
	}
}
