#include <stdlib.h>
#include <string.h>

#include "dngr_list.h"

/* Allocate a new node with specified value and append to list */
static DngrPtr* __dngr_list_append(DngrPtr** head, void* ptr) {
	DngrPtr* new;
	DngrPtr* old;

	new = calloc(1, sizeof(DngrPtr));
	if (new == NULL)
		return NULL;

	new->ptr = ptr;

	do {
		old = atomic_load(head);
		new->next = old;
	} while (!atomic_cas(head, &old, &new));

	return new;
}

/*
 * Attempt to find an empty node to store value, otherwise append a new node.
 * Returns the node containing the newly added value.
 */
DngrPtr* __dngr_list_insert_or_append(DngrPtr** head, void* val) {
	DngrPtr* node;
	void* expected;
	int need_alloc = 1;

	DNGR_LIST_ITER(head, node) {
		expected = atomic_load(&node->ptr);
		if (expected == NULL && atomic_cas(&node->ptr, &expected, &val)) {
			need_alloc = 0;
			break;
		}
	}

	if (need_alloc)
		node = __dngr_list_append(head, val);

	return node;
}

/* Remove a node from the list with the specified value */
int __dngr_list_remove(DngrPtr** head, void* val) {
	DngrPtr* node;
	void* expected;
	void* new = NULL;

	DNGR_LIST_ITER(head, node) {
		expected = atomic_load(&node->ptr);
		if (expected == val && atomic_cas(&node->ptr, &expected, &new))
			return 1;
	}

	return 0;
}

/* Returns 1 if the list currently contains an node with the specified value */
int __dngr_list_contains(DngrPtr** head, void* val) {
	DngrPtr* node;

	DNGR_LIST_ITER(head, node) {
		if (atomic_load(&node->ptr) == val)
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
