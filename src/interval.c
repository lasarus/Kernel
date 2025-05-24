#include "interval.h"
#include "kmalloc.h"

struct node {
	uint64_t low;
	uint64_t size;
	struct node *next;
};

static struct node *head = NULL;

int interval_exists(uint64_t low, uint64_t size) {
	for (struct node *node = head; node; node = node->next)
		if (low + size > node->low && low < node->low + node->size)
			return 1;
	return 0;
}

void interval_add(uint64_t low, uint64_t size) {
	struct node *node = kmalloc(sizeof *node);

	*node = (struct node) {
		.low = low,
		.size = size,
	};

	if (!head || low < head->low) {
		node->next = head;
		head = node;
		return;
	}

	struct node *search = head;
	while (search->next && search->next->low < low)
		search = search->next;

	node->next = search->next;
	search->next = node;
}

uint64_t interval_remove(uint64_t low) {
	struct node *previous = NULL, *node = head;
	while (node && node->low != low) {
		previous = node;
		node = node->next;
	}

	if (previous)
		previous->next = node->next;
	else
		head = node->next;

	uint64_t sz = node->size;
	kfree(node);
	return sz;
}

uint64_t interval_size(uint64_t low) {
	struct node *node = head;
	while (node && node->low != low) {
		node = node->next;
	}

	return node->size;
}
