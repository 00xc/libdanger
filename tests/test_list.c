#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "dngr_list.h"

#define W1 (void*)0xdeadbeef
#define W2 (void*)0xdeafbabe
#define W3 (void*)0xbadf00d
#define W4 (void*)0xba5eba11

#define ASSERT_OR(exp, s) assert(exp); printf("[+] %s\n", s);

int main() {
	DngrPtr* head;
	DngrPtr* node;
	unsigned int i = 0;

	head = calloc(1, sizeof(DngrPtr));
	ASSERT_OR(head != NULL, "allocation")

	__dngr_list_insert_or_append(&head, W1);
	__dngr_list_insert_or_append(&head, W2);
	__dngr_list_insert_or_append(&head, W3);

	DNGR_LIST_ITER(&head, node) {
		switch (i++) {
			case 0: ASSERT_OR(node->ptr == W3, "list_iter 1"); break;
			case 1: ASSERT_OR(node->ptr == W2, "list_iter 2"); break;
			case 2: ASSERT_OR(node->ptr == W1, "list_iter 3"); break;
			default: break;
		}
	}

	ASSERT_OR(__dngr_list_contains(&head, W3), "list_contains 1")
	ASSERT_OR(!__dngr_list_contains(&head, W4), "list_contains 2")

	ASSERT_OR(__dngr_list_remove(&head, W3), "list_remove 1")
	ASSERT_OR(!__dngr_list_contains(&head, W3), "list_remove 2")
	ASSERT_OR(head->ptr == NULL, "list_remove 3")
	ASSERT_OR(__dngr_list_contains(&head, W1), "list_remove 4")

	__dngr_list_insert_or_append(&head, W3);
	ASSERT_OR(head->ptr == W3, "list_insert");

	__dngr_list_free(&head);

	return EXIT_SUCCESS;
}
