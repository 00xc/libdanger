#include <stdlib.h>
#include <string.h>

#include "dngr_domain.h"

DngrDomain* dngr_domain_new(void (*deallocator)(void*)) {
	DngrDomain* dom;

	dom = calloc(1, sizeof(DngrDomain));
	if (dom == NULL)
		return NULL;

	dom->deallocator = deallocator;

	return dom;
}

void dngr_domain_free(DngrDomain* dom) {

	if (dom == NULL)
		return;

	if (dom->pointers != NULL)
		__dngr_list_free(&dom->pointers);

	if (dom->retired != NULL)
		__dngr_list_free(&dom->retired);

	free(dom);
}

void* dngr_load(DngrDomain* dom, void* prot_ptr) {
	const void* const nullptr = NULL;
	void* val;
	void* tmp;
	DngrPtr* node;

	while (1) {

		val = atomic_load((void**)prot_ptr);
		node = __dngr_list_insert_or_append(&dom->pointers, val);

		if (atomic_load((void**)prot_ptr) == val)
			break;

		/*
		 * This pointer is being retired by another thread - remove this hazard pointer
		 * and try again. We first try to remove the hazard pointer we just used. If someone
		 * else used it to drop the same pointer, we walk the list.
		 */
		tmp = val;
		if (!atomic_cas(&node->ptr, &tmp, &nullptr))
			__dngr_list_remove(&dom->pointers, val);
	}

	return val;
}

void dngr_drop(DngrDomain* dom, void* safe_val) {

	if (!__dngr_list_remove(&dom->pointers, safe_val))
		__builtin_unreachable();
}

void dngr_swap(DngrDomain* dom, void* prot_ptr, void* new_obj, int flags) {
	void* old_obj;
	int protected;

	old_obj = atomic_exchange((void**)prot_ptr, new_obj);
	protected = __dngr_list_contains(&dom->pointers, old_obj);

	if (!protected) {

		/* We can deallocate straight away */
		dom->deallocator(old_obj);

	} else if (flags & DNGR_DEFER_DEALLOC) {

		/* Defer deallocation for later */
		__dngr_list_insert_or_append(&dom->retired, old_obj);

	} else {

		/* Spin until all readers are done, then deallocate */
		while (__dngr_list_contains(&dom->pointers, old_obj));
		dom->deallocator(old_obj);

	}
}

void dngr_cleanup(DngrDomain* dom, int flags) {
	DngrPtr* node;
	void* ptr;

	DNGR_LIST_ITER(&dom->retired, node) {

		ptr = node->ptr;

		if (!__dngr_list_contains(&dom->pointers, ptr)) {

			/* We can deallocate straight away */
			if (__dngr_list_remove(&dom->retired, ptr))
				dom->deallocator(ptr);

		} else if (!(flags & DNGR_DEFER_DEALLOC)) {

			/* Spin until all readers are done, then deallocate */
			while (__dngr_list_contains(&dom->pointers, ptr));
			if (__dngr_list_remove(&dom->retired, ptr))
				dom->deallocator(ptr);
		}
		
	}
}
