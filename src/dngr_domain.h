#ifndef __DNGR_DOMAIN_H
#define __DNGR_DOMAIN_H

#include "dngr_list.h"

#define DNGR_DEFER_DEALLOC 1

typedef struct {
	DngrPtr* pointers;
	DngrPtr* retired;
	void (*deallocator)(void*);
} DngrDomain;

/* Create a new domain on the heap */
DngrDomain* dngr_domain_new(void (*deallocator)(void*));

/* Free a previously allocated domain */
void dngr_domain_free(DngrDomain* dom);

/*
 * Load a safe pointer to a shared object. This pointer must be passed to `dngr_drop` once it is
 * no longer needed
 */
void* dngr_load(DngrDomain* dom, void* prot_ptr);

/*
 * Drop a safe pointer to a shared object. This pointer (`safe_val`) must have come from
 * `dngr_load`
 */
void dngr_drop(DngrDomain* dom, void* safe_val);

/*
 * Swaps the contents of a shared pointer with a new pointer. The old value will be deallocated
 * by calling the `deallocator` function for the domain, provided when `dngr_domain_new` was
 * called. If `flags` is 0, this function will wait until no more references to the old object are
 * held in order to deallocate it. If flags is `DNGR_DEFER_DEALLOC`, the old object will only be
 * deallocated if there are already no references to it; otherwise the cleanup will be done the
 * next time `dngr_cleanup` is called.
 */
void dngr_swap(DngrDomain* dom, void* prot_ptr, void* new_val, int flags);

/*
 * Forces the cleanup of old objects that have not been deallocated yet. Just like `dngr_swap`,
 * if `flags` is 0, this function will wait until there are no more references to each object.
 * If `flags` is `DNGR_DEFER_DEALLOC`, only objects that already have no living references will be
 * deallocated.
 */
void dngr_cleanup(DngrDomain* dom, int flags);

#endif
