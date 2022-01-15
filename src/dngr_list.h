#ifndef __DNGR_LIST_H
#define __DNGR_LIST_H


#include "dngr_atomic.h"

#define DNGR_LIST_ITER(head, node) for (node = atomic_load(head); node; node = atomic_load(&node->next))

typedef struct __DngrPtr {
	void* ptr;
	struct __DngrPtr* next;
} DngrPtr;

DngrPtr* __dngr_list_insert_or_append(DngrPtr** head, void* ptr);
int __dngr_list_remove(DngrPtr** head, void* cur);
int __dngr_list_contains(DngrPtr** head, void* ptr);
void __dngr_list_free(DngrPtr** head);

#endif
