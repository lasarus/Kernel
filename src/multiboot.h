#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include "common.h"

// Structure format taken from:
// https://www.gnu.org/software/grub/manual/multiboot/multiboot.html

struct multiboot {
	uint32_t flags;

	uint32_t mem_lower;
	uint32_t mem_upper;

	uint32_t boot_device;

	uint32_t cmdline;

	uint32_t mods_count;
	uint32_t mods_addr;

	uint8_t syms[16];

	uint32_t mmap_length;
	uint32_t mmap_addr;

	// ... more things exist in this table
};

struct multiboot_module {
	uint32_t mod_start;
	uint32_t mod_end;
	uint32_t string;
	uint32_t reserved;
};

struct multiboot_mmap {
	uint32_t size; // Should this have position -4?
	uint64_t base_addr;
	uint64_t length;
	uint32_t type;
} __attribute__((packed)); // Necessary since base_addr has 8 byte alignment.

#endif
