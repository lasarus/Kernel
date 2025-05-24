#include "kmalloc.h"
#include "common.h"
#include "interval.h"
#include "memory.h"
#include "vga_text.h"
#include <stdint.h>

struct free_slot {
	struct free_slot *next;
};

struct slab {
	struct slab *next, *prev;
	struct free_slot *first_slot;
	uint32_t used, index; // index into the `slab_caches` array.
};

#define OBJECT_SIZE(SLAB) (1u << (5 + (SLAB)->index))
#define OBJECT_COUNT(SLAB) (PAGE_SIZE / OBJECT_SIZE(SLAB) - 1)

_Static_assert(sizeof(struct slab) <= 32, "");
// 32 is the smallest allowed object size.

struct slab_cache {
	int index;
	struct slab *partial, *full, *empty;
};

static struct slab_cache slab_caches[7];
// 32, 64, 128, 256, 512, 1024, 2048

static struct slab *new_slab(struct slab_cache *cache) {
	struct slab *slab = memory_alloc();
	*slab = (struct slab) {
		.index = cache->index,
	};

	struct free_slot **slot = &slab->first_slot;
	for (unsigned i = 1; i < OBJECT_COUNT(slab) + 1; i++) {
		unsigned offset = OBJECT_SIZE(slab) * i;
		uint8_t *ptr = (uint8_t *)slab + offset;
		struct free_slot *next_slot = (struct free_slot *)ptr;
		*slot = next_slot;
		slot = &next_slot->next;
		*slot = NULL;
	}
	return slab;
}

static void slab_init(struct slab_cache *cache, int index) {
	cache->index = index;
	cache->partial = NULL;
	cache->full = NULL;
	cache->empty = new_slab(cache);
}

static void *slab_allocate(struct slab_cache *cache) {
	struct slab **head = &cache->partial;
	if (!*head) {
		if (!cache->empty) {
			cache->empty = new_slab(cache);
		}
		head = &cache->empty;
	}

	if (!*head)
		return NULL;

	struct slab *slab = *head;
	void *return_value = slab->first_slot;
	slab->first_slot = slab->first_slot->next;
	slab->used++;

	// Always assume the slab will be moved.
	// Set next slab to be the current head

	*head = slab->next;
	if (slab->next)
		slab->next->prev = NULL;

	struct slab **new_head = &cache->partial;

	if (slab->used == OBJECT_COUNT(slab)) {
		new_head = &cache->full;
	} else if (slab->used == 0) {
		new_head = &cache->empty;
	}

	slab->next = *new_head;
	if (*new_head)
		(*new_head)->prev = slab;
	*new_head = slab;

	return return_value;
}

static void slab_free(void *ptr) {
	struct slab *slab = (struct slab *)((intptr_t)ptr & ~(PAGE_SIZE - 1));
	struct free_slot **head = &slab->first_slot;
	struct free_slot *free_slot = ptr;
	free_slot->next = *head;
	*head = free_slot;
	slab->used--;

	// Remove the slab from the current list.
	if (slab->prev)
		slab->prev->next = slab->next;

	struct slab_cache *cache = &slab_caches[slab->index];
	// Add the slab to the correct list.
	if (slab->used == OBJECT_COUNT(slab)) {
		slab->next = cache->full;
		cache->full = slab;
	} else if (slab->used == 0) {
		slab->next = cache->empty;
		cache->empty = slab;
	} else {
		slab->next = cache->partial;
		cache->partial = slab;
	}
}

static size_t slab_get_size(void *ptr) {
	return OBJECT_SIZE((struct slab *)((intptr_t)ptr & ~(PAGE_SIZE - 1)));
}

static void *kmalloc_small(size_t size) {
	// Round up to nearest power of two.
	struct slab_cache *cache = NULL;
	for (unsigned i = 0; i < sizeof slab_caches / sizeof *slab_caches; i++) {
		if (size <= OBJECT_SIZE(&slab_caches[i])) {
			cache = &slab_caches[i];
			break;
		}
	}

	if (cache)
		return slab_allocate(cache);
	return NULL;
}

static uint64_t kmalloc_random(void) {
	// xorshift64star, from Wikipedia.
	static uint64_t x = 0x123456789;
	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	return x * 0x2545F4914F6CDD1DULL;
}

static void *kmalloc_large(size_t size, size_t alignment) {
	// Alignment must be at least a page.
	if (alignment < PAGE_SIZE)
		alignment = PAGE_SIZE;
	size_t alignment_mask = ~(alignment - 1);
	for (int i = 0; i < 32; i++) {
		uint64_t low = LARGE_ALLOC_OFFSET | (kmalloc_random() & 0xfffffffff & alignment_mask);

		if (interval_exists(low, size))
			continue;

		interval_add(low, size);

		memory_allocate_range(NULL, low, NULL, size, 0);

		return (void *)low;
	}

	ERROR("Probably out of virtual memory.\n");
	return NULL;
}

static size_t get_size_of_allocation(void *ptr) {
	if ((uintptr_t)ptr >= LARGE_ALLOC_OFFSET) {
		return interval_size((uint64_t)ptr);
	} else {
		return slab_get_size(ptr);
	}
}

static void kfree_large(void *ptr) {
	uint64_t size = interval_remove((uint64_t)ptr);
	memory_deallocate_range((uint64_t)ptr, size);
}

void kmalloc_init(void) {
	for (unsigned i = 0; i < sizeof slab_caches / sizeof *slab_caches; i++) {
		slab_init(&slab_caches[i], i);
	}
}

void *kmalloc(size_t size) {
	return size > 2048 ? kmalloc_large(size, PAGE_SIZE) : kmalloc_small(size);
}

void kfree(void *ptr) {
	if ((uintptr_t)ptr >= LARGE_ALLOC_OFFSET) {
		kfree_large(ptr);
	} else {
		slab_free(ptr);
	}
}

void *krealloc(void *ptr, size_t size) {
	void *new_area = kmalloc(size);
	size_t old_size = get_size_of_allocation(ptr);
	memcpy(new_area, ptr, old_size);
	kfree(ptr);
	return new_area;
}

void *kaligned_alloc(size_t alignment, size_t size) {
	size_t largest = alignment; // Largest of alignment and size.
	if (size > largest)
		largest = size;
	return alignment > 2048 ? kmalloc_large(size, alignment) : kmalloc_small(largest);
}
