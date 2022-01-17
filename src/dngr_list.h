#ifndef __DNGR_LIST_H
#define __DNGR_LIST_H

#include <stdint.h>

#include "dngr_atomic.h"

#define DNGR_LIST_ITER(head, node) for (node = atomic_load(head); node; node = atomic_load(&node->next))

typedef struct __DngrPtr {
	uintptr_t ptr;
	struct __DngrPtr* next;
} DngrPtr;

DngrPtr* __dngr_list_insert_or_append(DngrPtr** head, uintptr_t ptr);
int __dngr_list_remove(DngrPtr** head, uintptr_t ptr);
int __dngr_list_contains(DngrPtr** head, uintptr_t ptr);
void __dngr_list_free(DngrPtr** head);

#endif
