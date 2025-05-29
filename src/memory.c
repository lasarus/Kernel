#include "memory.h"
#include "common.h"
#include "multiboot.h"
#include "page_allocator.h"

struct pml4 {
	struct {
		uint64_t flags_and_address;
	} entries[512];
};

struct pdpt {
	struct {
		uint64_t flags_and_address;
	} entries[512];
};

struct pd {
	struct {
		uint64_t flags_and_address;
	} entries[512];
};

struct pt {
	struct {
		uint64_t flags_and_address;
	} entries[512];
};

_Alignas(PAGE_SIZE) static struct pdpt kernel_pdpt_table_large; // Maps first 512GiB.

void set_up_higher_identity_paging(void) {
	for (unsigned i = 0; i < 512; i++)
		kernel_pdpt_table_large.entries[i].flags_and_address = (0x1 | 0x2 | 0x4 | 0x80) + ((uint64_t)i << 30);

	struct pml4 *pml4 = (void *)(get_cr3() + HIGHER_HALF_OFFSET);

	pml4->entries[256].flags_and_address =
		(0x1 | 0x2 | 0x4) + ((uint64_t)&kernel_pdpt_table_large - HIGHER_HALF_OFFSET);

	pml4->entries[0].flags_and_address = 0;
}

// See section 4.8.1 in AMD64 Architecture Programmer's Manual, Volume 2.
struct gdt_entry {
	uint64_t limit_low : 16;
	uint64_t base_low : 24;
	uint64_t accessed : 1;
	uint64_t readable : 1;
	uint64_t conforming : 1;
	uint64_t code : 1; // 1 if code, 0 if data.
	uint64_t code_or_data : 1; // 1 if code or data. 0 otherwise? For example TSS.
	uint64_t dpl : 2;
	uint64_t present : 1;
	uint64_t limit_high : 4;
	uint64_t available : 1;
	uint64_t long_mode : 1;
	uint64_t d : 1;
	uint64_t granularity : 1;
	uint64_t base_high : 8;
} __attribute__((packed));

// See section 4.8.3 in AMD64 Architecture Programmer's Manual, Volume 2.
// (Figure 4-22)
struct tss_descriptor {
	uint64_t limit_low : 16;
	uint64_t base_low : 24;
	uint64_t type : 4;
	uint64_t zero0 : 1;
	uint64_t dpl : 2;
	uint64_t present : 1;
	uint64_t limit_high : 4;
	uint64_t available : 1;
	uint64_t zero1 : 1;
	uint64_t zero2 : 1;
	uint64_t granularity : 1;
	uint64_t base_high : 40;
	uint64_t reserved0 : 8;
	uint64_t zero3 : 5;
	uint64_t reserved1 : 19;
} __attribute__((packed));

// See section 12.2.5 in AMD64 Architecture Programmer's Manual, Volume 2.
// (Figure 12-8)
struct tss {
	uint32_t reserved0;
	uint64_t rsp0, rsp1, rsp2;
	uint64_t reserved1;
	uint64_t ist[7];
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t io_map_base_address;
} __attribute__((packed));

_Alignas(PAGE_SIZE) struct tss tss;

_Static_assert(sizeof(struct gdt_entry) == 8, "");
_Static_assert(sizeof(struct tss_descriptor) == 16, "");
_Static_assert(sizeof(struct tss) == 104, "");

static struct gdt {
	struct gdt_entry gdt_entries[5];
	struct tss_descriptor tss_descriptor;
} __attribute__((packed)) gdt = {
	.gdt_entries = {
		{ 0 },
		{ .readable = 1, .code = 1, .code_or_data = 1, .dpl = 0, .present = 1, .long_mode = 1 },
		{ .readable = 1, .code = 0, .code_or_data = 1, .present = 1 },
		{ .readable = 1, .code = 0, .dpl = 3, .code_or_data = 1, .present = 1 },
		{ .readable = 1, .code = 1, .code_or_data = 1, .dpl = 3, .present = 1, .long_mode = 1 },
	},
	.tss_descriptor = { 0 }, // Will be initailized at runtime, due to some
	                         // bit-twiddling needed.
};

struct pml4 *memory_init(struct multiboot *mb) {
	set_up_higher_identity_paging();

	extern void *_end;
	uint64_t first_free = (uint64_t)&_end;
	struct multiboot_module *modules = (void *)(mb->mods_addr + HIGHER_HALF_OFFSET);
	for (unsigned i = 0; i < mb->mods_count; i++)
		first_free = MAX(first_free, modules[i].mod_end);

	struct multiboot_mmap *mmaps = (void *)(mb->mmap_addr + HIGHER_HALF_OFFSET);
	for (unsigned i = 0; i < mb->mmap_length; i++)
		if (mmaps[i].type == 1)
			page_allocator_add_range(MAX(mmaps[i].base_addr, first_free), mmaps[i].base_addr + mmaps[i].length);

	uint64_t tss_base = (uint64_t)&tss;
	uint32_t tss_limit = sizeof tss - 1;

	gdt.tss_descriptor.limit_low = tss_limit;
	gdt.tss_descriptor.limit_high = tss_limit >> 16;
	gdt.tss_descriptor.base_low = tss_base;
	gdt.tss_descriptor.base_high = tss_base >> 24;
	gdt.tss_descriptor.type = 0x9; // Table 4-6.
	gdt.tss_descriptor.present = 1;

	load_gdt(sizeof gdt - 1, (void *)&gdt);

	return PHYSICAL_TO_VIRTUAL(get_cr3());
}

struct pml4 *memory_new_page_table(void) {
	struct pml4 *new_table = allocate_page();
	for (unsigned i = 0; i < 512; i++)
		new_table->entries[i].flags_and_address = 0;

	return new_table;
}

#define GET_TABLE(FAA) PHYSICAL_TO_VIRTUAL((FAA) & 0x00Cffffffffff000)
#define MAKE_TABLE(ADDRESS) ((0x1 | 0x2 | 0x4) + VIRTUAL_TO_PHYSICAL(ADDRESS))

void copy_pt(struct pt *dest, struct pt *src) {
	for (int i = 0; i < 512; i++) {
		if (!src->entries[i].flags_and_address) {
			dest->entries[i] = src->entries[i];
			continue;
		}

		uint8_t *src_memory = GET_TABLE(src->entries[i].flags_and_address);
		uint8_t *dest_memory = allocate_page();

		for (int i = 0; i < PAGE_SIZE; i++) {
			dest_memory[i] = src_memory[i];
		}

		dest->entries[i].flags_and_address =
			(src->entries[i].flags_and_address & 0xfff) | VIRTUAL_TO_PHYSICAL(dest_memory);
	}
}

void copy_pd(struct pd *dest, struct pd *src) {
	for (int i = 0; i < 512; i++) {
		if (!src->entries[i].flags_and_address) {
			dest->entries[i] = src->entries[i];
			continue;
		}

		struct pt *src_pt = GET_TABLE(src->entries[i].flags_and_address);
		struct pt *dst_pt = allocate_page();

		copy_pt(dst_pt, src_pt);

		dest->entries[i].flags_and_address = (src->entries[i].flags_and_address & 0xfff) | VIRTUAL_TO_PHYSICAL(dst_pt);
	}
}

void copy_pdpt(struct pdpt *dest, struct pdpt *src) {
	for (int i = 0; i < 512; i++) {
		if (!src->entries[i].flags_and_address) {
			dest->entries[i] = src->entries[i];
			continue;
		}

		struct pd *src_pd = GET_TABLE(src->entries[i].flags_and_address);
		struct pd *dst_pd = allocate_page();

		copy_pd(dst_pd, src_pd);

		dest->entries[i].flags_and_address = (src->entries[i].flags_and_address & 0xfff) | VIRTUAL_TO_PHYSICAL(dst_pd);
	}
}

void copy_pml4(struct pml4 *dest, struct pml4 *src) {
	for (int i = 0; i < 512; i++) {
		if (!src->entries[i].flags_and_address || i == 511 || i == 256 || i == LARGE_ALLOC_INDEX) {
			dest->entries[i] = src->entries[i];
			continue;
		}

		struct pdpt *src_pdpt = GET_TABLE(src->entries[i].flags_and_address);
		struct pdpt *dst_pdpt = allocate_page();

		copy_pdpt(dst_pdpt, src_pdpt);

		dest->entries[i].flags_and_address =
			(src->entries[i].flags_and_address & 0xfff) | VIRTUAL_TO_PHYSICAL(dst_pdpt);
	}
}

struct pml4 *memory_page_table_copy(struct pml4 *old) {
	struct pml4 *new = memory_new_page_table();

	copy_pml4(new, old);

	return new;
}

void delete_pt(struct pt *table) {
	for (int i = 0; i < 512; i++) {
		if (!table->entries[i].flags_and_address)
			continue;

		void *memory = GET_TABLE(table->entries[i].flags_and_address);

		free_page(memory);
	}
}

void delete_pd(struct pd *table) {
	for (int i = 0; i < 512; i++) {
		if (!table->entries[i].flags_and_address)
			continue;

		struct pt *pt = GET_TABLE(table->entries[i].flags_and_address);

		delete_pt(pt);
		free_page(pt);

		table->entries[i].flags_and_address = 0;
	}
}

void delete_pdpt(struct pdpt *table) {
	for (int i = 0; i < 512; i++) {
		if (!table->entries[i].flags_and_address)
			continue;

		struct pd *pd = GET_TABLE(table->entries[i].flags_and_address);

		delete_pd(pd);
		free_page(pd);

		table->entries[i].flags_and_address = 0;
	}
}

void delete_pml4(struct pml4 *table, int only_user) {
	for (int i = 0; i < 512; i++) {
		if (!table->entries[i].flags_and_address || i == 511 || i == 256 || i == LARGE_ALLOC_INDEX) {
			continue;
		}

		if (only_user && i >= 256)
			continue;

		struct pdpt *pdpt = GET_TABLE(table->entries[i].flags_and_address);

		delete_pdpt(pdpt);
		free_page(pdpt);

		table->entries[i].flags_and_address = 0;
	}
	reload_cr3();
}

void memory_page_table_delete(struct pml4 *old, int only_user) {
	delete_pml4(old, only_user);
}

void memory_page_table_delete_pdpt(struct pml4 *old, int idx) {
	if (!old->entries[idx].flags_and_address)
		return;

	struct pdpt *pdpt = GET_TABLE(old->entries[idx].flags_and_address);

	delete_pdpt(pdpt);
	free_page(pdpt);

	old->entries[idx].flags_and_address = 0;

	reload_cr3();
}

void memory_page_table_move_pdpt(struct pml4 *old, int dest, int src) {
	old->entries[dest] = old->entries[src];
	old->entries[src].flags_and_address = 0;

	reload_cr3();
}

void memory_page_add(struct pml4 *table, uint64_t virtual_addr, void *high_addr, int user) {
	uint16_t a = (virtual_addr >> (12 + 9 * 0)) & 0x1FF;
	uint16_t b = (virtual_addr >> (12 + 9 * 1)) & 0x1FF;
	uint16_t c = (virtual_addr >> (12 + 9 * 2)) & 0x1FF;
	uint16_t d = (virtual_addr >> (12 + 9 * 3)) & 0x1FF;

	struct pml4 *pml4 = table;

	struct pdpt *pdpt = 0;
	if (pml4->entries[d].flags_and_address & 1) {
		pdpt = PHYSICAL_TO_VIRTUAL(pml4->entries[d].flags_and_address & 0x00Cffffffffff000);
	} else {
		pdpt = allocate_page();
		for (unsigned i = 0; i < 512; i++)
			pdpt->entries[i].flags_and_address = 0;
		pml4->entries[d].flags_and_address = (0x1 | 0x2 | 0x4) + VIRTUAL_TO_PHYSICAL(pdpt);
	}

	struct pdpt *pd = 0;
	if (pdpt->entries[c].flags_and_address & 1) {
		pd = PHYSICAL_TO_VIRTUAL(pdpt->entries[c].flags_and_address & 0x00Cffffffffff000);
	} else {
		pd = allocate_page();
		for (unsigned i = 0; i < 512; i++)
			pd->entries[i].flags_and_address = 0;
		pdpt->entries[c].flags_and_address = (0x1 | 0x2 | 0x4) + VIRTUAL_TO_PHYSICAL(pd);
	}

	struct pdpt *pt = 0;
	if (pd->entries[b].flags_and_address & 1) {
		pt = PHYSICAL_TO_VIRTUAL(pd->entries[b].flags_and_address & 0x00Cffffffffff000);
	} else {
		pt = allocate_page();
		for (unsigned i = 0; i < 512; i++)
			pt->entries[i].flags_and_address = 0;
		pd->entries[b].flags_and_address = (0x1 | 0x2 | 0x4) + VIRTUAL_TO_PHYSICAL(pt);
	}

	uint64_t flags = 0x1 | 0x2;
	if (user)
		flags |= 0x4;
	uint64_t physical_addr = VIRTUAL_TO_PHYSICAL(high_addr);
	pt->entries[a].flags_and_address = flags | physical_addr;
}

void memory_allocate_range(struct pml4 *table, uint64_t base, uint8_t *data, uint64_t size, int user) {
	// TODO: Remove this when memory subsystem is more sane.
	if (!table)
		table = PHYSICAL_TO_VIRTUAL(get_cr3());

	// base must be aligned to 4KiB page.
	for (uint64_t page = 0; page < size; page += PAGE_SIZE) {
		uint8_t *new_page = allocate_page();

		if (data) {
			for (unsigned i = 0; i < PAGE_SIZE; i++) {
				if (page + i >= size)
					break;
				new_page[i] = data[page + i];
			}
		}

		memory_page_add(table, base + page, new_page, user);
	}

	reload_cr3();
}

static void delete_page(struct pml4 *table, uint64_t address) {
	uint16_t pml4_index = (address >> 39) & 0x1FF;
	uint16_t pdpt_index = (address >> 30) & 0x1FF;
	uint16_t pd_index = (address >> 21) & 0x1FF;
	uint16_t pt_index = (address >> 12) & 0x1FF;

	struct pdpt *pdpt = GET_TABLE(table->entries[pml4_index].flags_and_address);
	struct pdpt *pd = GET_TABLE(pdpt->entries[pdpt_index].flags_and_address);
	struct pdpt *pt = GET_TABLE(pd->entries[pd_index].flags_and_address);

	uint8_t *src_memory = GET_TABLE(pt->entries[pt_index].flags_and_address);
	free_page(src_memory);

	pt->entries[pt_index].flags_and_address = 0;
	int delete_pt = 1;
	for (int i = 0; i < 512; i++) {
		if (pt->entries[i].flags_and_address) {
			delete_pt = 0;
			break;
		}
	}

	if (!delete_pt)
		return;

	free_page(pt);
	pd->entries[pd_index].flags_and_address = 0;
	int delete_pd = 1;
	for (int i = 0; i < 512; i++) {
		if (pd->entries[i].flags_and_address) {
			delete_pd = 0;
			break;
		}
	}

	if (!delete_pd)
		return;

	free_page(pd);
	pdpt->entries[pdpt_index].flags_and_address = 0;

	// For now, keep pdpt.
}

void memory_deallocate_range(uint64_t base, uint64_t size) {
	// TODO: Refactor memory subsystem to be more sane.
	struct pml4 *current_pml4 = PHYSICAL_TO_VIRTUAL(get_cr3());
	for (uint64_t page = 0; page < size; page += PAGE_SIZE) {
		delete_page(current_pml4, base + page);
	}
}

void set_kernel_stack(uint64_t stack) {
	tss.rsp0 = stack;
	gdt.tss_descriptor.type = 9;
	load_tss(offsetof(struct gdt, tss_descriptor));
}
