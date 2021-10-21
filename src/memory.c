#include "memory.h"
#include "vga_text.h"

#include "common.h"

struct memory_list {
	struct memory_list *next;
	uint64_t n_entries;
	void *entries[510];
};

_Static_assert(sizeof(struct memory_list) == 4096, "Inalid size of memory_list.");

struct memory_list *head;
_Alignas(4096) struct memory_list last_elem;

void *memory_alloc(void) {
	if (head->n_entries == 0) {
		void *chunk = head;
		head = head->next;
		return chunk;
	} else {
		head->n_entries--;
		return head->entries[head->n_entries];
	}
}

void memory_free(void *ptr) {
	// Assume ptr is not present in the memory list.

	if (head->n_entries == 510) {
		struct memory_list *prev_head = head;
		head = ptr;
		head->next = prev_head;
		head->n_entries = 0;
	} else {
		head->entries[head->n_entries++] = ptr;
	}
}

struct pml4_entry {
	// TODO: Make this a bit cleaner.
	// Perhaps using bit-fields.
	uint64_t flags_and_address;
};

struct pml4 {
	struct pml4_entry entries[512];
};

struct pdpt_entry {
	uint64_t flags_and_address;
};

struct pdpt {
	struct pdpt_entry entries[512];
};

struct pd_entry {
	uint64_t flags_and_address;
};

struct pd {
	struct pd_entry entries[512];
};

struct pt_entry {
	uint64_t flags_and_address;
};

struct pt {
	struct pt_entry entries[512];
};

// Identity tables.
_Alignas(4096) static struct pml4 kernel_pml4_table;
_Alignas(4096) static struct pdpt kernel_pdpt_table_large; // Maps first 512GiB.
_Alignas(4096) static struct pdpt kernel_pdpt_table_small; // Maps first 1GiB.

void init_kernel_pml4(struct pml4 *pml4) {
	for (unsigned i = 0; i < 512; i++)
		pml4->entries[i].flags_and_address = 0;

	pml4->entries[511].flags_and_address =
		(0x1 | 0x2) + ((uint64_t)&kernel_pdpt_table_small - HIGHER_HALF_OFFSET);
	pml4->entries[256].flags_and_address =
		(0x1 | 0x2) + ((uint64_t)&kernel_pdpt_table_large - HIGHER_HALF_OFFSET);
}

void set_up_higher_identity_paging(void) {
	for (unsigned i = 0; i < 512; i++)
		kernel_pdpt_table_large.entries[i].flags_and_address = (0x1 | 0x2 | 0x80) + (i << 12);
	kernel_pdpt_table_small.entries[511].flags_and_address = 0x1 | 0x2 | 0x80;

	init_kernel_pml4(&kernel_pml4_table);

	load_cr3((void *)((uint64_t)&kernel_pml4_table - HIGHER_HALF_OFFSET));
	// We have a page table inherited from the bootstrap code.
}

extern void *_end;

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

// See section 4.8.3 in AMD64 Architecture Programmer's Manual, Volume 2. (Figure 4-22)
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

// See section 12.2.5 in AMD64 Architecture Programmer's Manual, Volume 2. (Figure 12-8)
struct tss {
	uint32_t reserved0;
	uint64_t rsp0, rsp1, rsp2;
	uint64_t reserved1;
	uint64_t ist[7];
	uint64_t reserved2;
	uint16_t reserved3;
	uint16_t io_map_base_address;
} __attribute__((packed));

_Alignas(4096) struct tss tss;

_Static_assert(sizeof (struct gdt_entry) == 8, "");
_Static_assert(sizeof (struct tss_descriptor) == 16, "");
_Static_assert(sizeof (struct tss) == 104, "");

static struct gdt {
	struct gdt_entry gdt_entries[5];
	struct tss_descriptor tss_descriptor;
} __attribute__((packed)) gdt = {
	.gdt_entries = {
		{0},
		{ .readable = 1, .code = 1, .code_or_data = 1, .dpl = 0, .present = 1, .long_mode = 1 },
		{ .readable = 1, .code = 0, .code_or_data = 1, .present = 1 },
		{ .readable = 1, .code = 1, .code_or_data = 1, .dpl = 3, .present = 1, .long_mode = 1 },
		{ .readable = 1, .code = 0, .dpl = 3, .code_or_data = 1, .present = 1 }
	},
	.tss_descriptor = { 0 } // Will be initailized at runtime, due to some bit-twiddling needed.
};

struct page_table_table {
	uint64_t n_tables;
	struct pml4 *pml4_pointers[511];
};

struct page_table_table *tables;
struct pml4 *current_pml4;

page_table_t memory_init(struct multiboot *mb) {
	head = &last_elem;

	uint64_t first_free = (uint64_t)&_end;
	for (unsigned i = 0; i < mb->mods_count; i++) {
		struct multiboot_module *modules = (void *)(uint64_t)mb->mods_addr;
		if (modules[i].mod_end > first_free)
			first_free = modules[i].mod_end;
	}

	set_up_higher_identity_paging();
	uint64_t total_mem = 0;
	uint64_t mmap_addr = mb->mmap_addr + HIGHER_HALF_OFFSET;
	for (unsigned i = 0; i < mb->mmap_length; i++) {
		struct multiboot_mmap *mmap = (void *)mmap_addr;
		mmap += i;
		if (mmap->type != 1)
			continue;

		uint64_t start = round_up_4096(mmap->base_addr);
		for (; start < mmap->length + mmap->base_addr - 4096; start += 4096) {
			if (start < first_free)
				continue;
			memory_free((void *)(start + HIGHER_HALF_IDENTITY));
			total_mem += 4096;
		}
	}

	uint64_t tss_base = (uint64_t)&tss;
	uint32_t tss_limit = sizeof tss - 1;

	gdt.tss_descriptor.limit_low = tss_limit;
	gdt.tss_descriptor.limit_high = tss_limit >> 16;
	gdt.tss_descriptor.base_low = tss_base;
	gdt.tss_descriptor.base_high = tss_base >> 24;
	gdt.tss_descriptor.type = 0x9; // Table 4-6.
	gdt.tss_descriptor.present = 1;

	load_gdt(sizeof gdt - 1, (void *)&gdt);

	tables = memory_alloc();
	tables->n_tables = 1;
	tables->pml4_pointers[0] = (void *)((uint64_t)&kernel_pml4_table - HIGHER_HALF_OFFSET + HIGHER_HALF_IDENTITY);

	return 0;
}

page_table_t memory_new_page_table(void) {
	struct pml4 *new_table = memory_alloc();
	for (unsigned i = 0; i < 512; i++)
		new_table->entries[i].flags_and_address = 0;

	init_kernel_pml4(new_table);
	int idx = tables->n_tables++;

	tables->pml4_pointers[idx] = new_table;
	current_pml4 = new_table;

	return idx;
}

void memory_switch_page_table(page_table_t table) {
	load_cr3((void *)((uint64_t)tables->pml4_pointers[table] - HIGHER_HALF_IDENTITY));
	current_pml4 = tables->pml4_pointers[table];
}

void memory_page_add(uint64_t virtual_addr, void *high_addr, int user) {
	uint16_t a = (virtual_addr >> (12 + 9 * 0)) & 0x1FF;
	uint16_t b = (virtual_addr >> (12 + 9 * 1)) & 0x1FF;
	uint16_t c = (virtual_addr >> (12 + 9 * 2)) & 0x1FF;
	uint16_t d = (virtual_addr >> (12 + 9 * 3)) & 0x1FF;

	struct pml4 *pml4 = current_pml4;

	struct pdpt *pdpt = 0;
	if (pml4->entries[d].flags_and_address & 1) {
		pdpt = (void *)((pml4->entries[d].flags_and_address & 0x00Cffffffffff000) + HIGHER_HALF_IDENTITY);
	} else {
		pdpt = memory_alloc();
		for (unsigned i = 0; i < 512; i++)
			pdpt->entries[i].flags_and_address = 0;
		pml4->entries[d].flags_and_address = (0x1 | 0x2 | 0x4) + (uint64_t)pdpt - HIGHER_HALF_IDENTITY;
	}

	struct pdpt *pd = 0;
	if (pdpt->entries[c].flags_and_address & 1) {
		pd = (void *)((pdpt->entries[c].flags_and_address & 0x00Cffffffffff000) + HIGHER_HALF_IDENTITY);
	} else {
		pd = memory_alloc();
		for (unsigned i = 0; i < 512; i++)
			pd->entries[i].flags_and_address = 0;
		pdpt->entries[c].flags_and_address = (0x1 | 0x2 | 0x4) + (uint64_t)pd - HIGHER_HALF_IDENTITY;
	}

	struct pdpt *pt = 0;
	if (pd->entries[b].flags_and_address & 1) {
		pt = (void *)((pd->entries[b].flags_and_address & 0x00Cffffffffff000) + HIGHER_HALF_IDENTITY);
	} else {
		pt = memory_alloc();
		for (unsigned i = 0; i < 512; i++)
			pt->entries[i].flags_and_address = 0;
		pd->entries[b].flags_and_address = (0x1 | 0x2 | 0x4) + (uint64_t)pt - HIGHER_HALF_IDENTITY;
	}

	uint64_t flags = 0x1 | 0x2;
	if (user)
		flags |= 0x4;
	uint64_t physical_addr = (uint64_t)high_addr - HIGHER_HALF_IDENTITY;
	pt->entries[a].flags_and_address = flags | (uint64_t)physical_addr;
}

void memory_allocate_range(uint64_t base, uint64_t size, int user) {
	// base must be aligned to 4KiB page.

	for (uint64_t page = 0; page < size; page += 4096)
		memory_page_add(base + page, memory_alloc(), user);
}

void set_kernel_stack(uint64_t stack) {
	tss.rsp0 = stack;
	gdt.tss_descriptor.type = 9;
	load_tss(40);
}
