#include "elf_loader.h"
#include "memory.h"
#include "vfs.h"
#include "vga_text.h"

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
int elf_loader_stage(struct pml4 *table, const char *path, uint64_t *rip) {
	struct inode *inode = vfs_resolve(NULL, path);

	if (!inode)
		return 1;

	if (inode->type != INODE_FILE)
		return 1;

	struct file *file = vfs_open_inode(inode, O_RDONLY);
	if (!file)
		return 1;

	// Read elf header
	struct header header;

	vfs_read_file(file, &header, sizeof header);

	if (!(header.e_ident[0] == 0x7f && header.e_ident[1] == 'E' && header.e_ident[2] == 'L' &&
	      header.e_ident[3] == 'F')) {
		return 1;
	}

	*rip = header.e_entry;

	if (header.e_phentsize != sizeof(struct program_header)) {
		print("Header entry size not as expected!\n");
		hang_kernel();
	}

	for (size_t i = 0; i < header.e_phnum; i++) {
		struct program_header ph;

		vfs_lseek(file, header.e_phoff + i * header.e_phentsize, SEEK_SET);
		vfs_read_file(file, &ph, header.e_phentsize);

		if (ph.p_type == 0 || ph.p_type > 16) {
		} else if (ph.p_type == 1) { // LOAD
			if (ph.p_filesz != ph.p_memsz) {
				print("Not implemented\n");
				hang_kernel();
			}

			vfs_lseek(file, ph.p_offset, SEEK_SET);
			memory_allocate_range(table, ph.p_vaddr + ELF_STAGE_OFFSET, NULL, ph.p_memsz, 1);
			vfs_read_file(file, (void *)(ph.p_vaddr + ELF_STAGE_OFFSET), ph.p_filesz);
		} else {
			print("Invalid program header type!\n");
			print_hex(ph.p_type);
			print("\n");
			hang_kernel();
		}
	}

	vfs_close_file(file);

	return 0;
}
