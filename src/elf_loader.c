#include "elf_loader.h"
#include "vga_text.h"
#include "memory.h"

struct header {
	unsigned char e_ident[16];
	uint16_t e_type; // Not verified
	uint16_t e_machine; // Not verified
	uint32_t e_version; // Not verified
	uint64_t e_entry;
	uint64_t e_phoff;
	uint64_t e_shoff;
	uint32_t e_flags;
	uint16_t e_ehsize;
	uint16_t e_phentsize;
	uint16_t e_phnum;
	uint16_t e_shentsize;
	uint16_t e_shnum;
	uint16_t e_shstrndx;
};

struct program_header {
	uint32_t p_type;
	uint32_t p_flags;
	uint64_t p_offset;
	uint64_t p_vaddr;
	uint64_t p_paddr;
	uint64_t p_filesz;
	uint64_t p_memsz;
	uint64_t p_align;
};

// It would be fun to make this a part of userspace some time.
void elf_loader_load(page_table_t table, uint8_t *data, uint64_t size, uint64_t *rip) {
	// Read elf header
	struct header *header = (struct header *)data;

	if (!(header->e_ident[0] == 0x7f &&
		  header->e_ident[1] == 'E' &&
		  header->e_ident[2] == 'L' &&
		  header->e_ident[3] == 'F')) {
		print("Not elf!\n");
		hang_kernel();
	}

	*rip = header->e_entry;

	if (header->e_phentsize != sizeof (struct program_header)) {
		print("Header entry size not as expected!\n");
		hang_kernel();
	}

	struct program_header *program_headers = (struct program_header *)(data + header->e_phoff);
	for (int i = 0; i < header->e_phnum; i++) {
		struct program_header *ph = program_headers + i;
		if (ph->p_type == 0) {
			continue;
		} else if (ph->p_type == 1) { // LOAD
			if (ph->p_filesz != ph->p_memsz) {
				print("Not implemented\n");
				hang_kernel();
			}
			memory_allocate_range(table, ph->p_vaddr, data + ph->p_offset, ph->p_filesz, 1);
		} else {
			print("Invalid program header type!\n");
			hang_kernel();
		}
	}
}
