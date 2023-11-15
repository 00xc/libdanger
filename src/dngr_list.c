#include <stdlib.h>
#include <string.h>

#include "dngr_list.h"

/* Allocate a new node with specified value and append to list */
static DngrPtr* __dngr_list_append(DngrPtr** head, uintptr_t ptr) {
	DngrPtr* new;
	DngrPtr* old;

	new = calloc(1, sizeof(DngrPtr));
	if (new == NULL)
		return NULL;

	new->ptr = ptr;
	old = atomic_load(head);

	do {
		new->next = old;
	} while (!atomic_cas_weak(head, &old, &new));

	return new;
}

/*
 * Attempt to find an empty node to store value, otherwise append a new node.
 * Returns the node containing the newly added value.
 */
DngrPtr* __dngr_list_insert_or_append(DngrPtr** head, uintptr_t ptr) {
	DngrPtr* node;
	uintptr_t expected;
	int need_alloc = 1;

	DNGR_LIST_ITER(head, node) {
		expected = atomic_load(&node->ptr);
		if (expected == 0 && atomic_cas(&node->ptr, &expected, &ptr)) {
			need_alloc = 0;
			break;
		}
	}

	if (need_alloc)
		node = __dngr_list_append(head, ptr);

	return node;
}

/* Remove a node from the list with the specified value */
int __dngr_list_remove(DngrPtr** head, uintptr_t ptr) {
	DngrPtr* node;
	uintptr_t expected;
	const uintptr_t nullptr = 0;

	DNGR_LIST_ITER(head, node) {
		expected = atomic_load(&node->ptr);
		if (expected == ptr && atomic_cas(&node->ptr, &expected, &nullptr))
			return 1;
	}

	return 0;
}

/* Returns 1 if the list currently contains an node with the specified value */
int __dngr_list_contains(DngrPtr** head, uintptr_t ptr) {
	DngrPtr* node;

	DNGR_LIST_ITER(head, node) {
		if (atomic_load(&node->ptr) == ptr)
			return 1;
	}

	return 0;
}

/* Frees all the nodes in a list - NOT THREAD SAFE */
void __dngr_list_free(DngrPtr** head) {
	DngrPtr* cur;
	DngrPtr* old;

	cur = *head;
	while (cur) {
		old = cur;
		cur = cur->next;
		free(old);
	}
}
