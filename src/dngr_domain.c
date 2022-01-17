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

uintptr_t dngr_load(DngrDomain* dom, uintptr_t* prot_ptr) {
	const uintptr_t nullptr = 0;
	uintptr_t val;
	uintptr_t tmp;
	DngrPtr* node;

	while (1) {

		val = atomic_load(prot_ptr);
		node = __dngr_list_insert_or_append(&dom->pointers, val);

		if (atomic_load(prot_ptr) == val)
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

void dngr_drop(DngrDomain* dom, uintptr_t safe_val) {

	if (!__dngr_list_remove(&dom->pointers, safe_val))
		__builtin_unreachable();
}

static void __dngr_cleanup_ptr(DngrDomain* dom, uintptr_t ptr, int flags) {

	if (!__dngr_list_contains(&dom->pointers, ptr)) {

		/* We can deallocate straight away */
		dom->deallocator((void*)ptr);

	} else if (flags & DNGR_DEFER_DEALLOC) {

		/* Defer deallocation for later */
		__dngr_list_insert_or_append(&dom->retired, ptr);

	} else {

		/* Spin until all readers are done, then deallocate */
		while (__dngr_list_contains(&dom->pointers, ptr));
		dom->deallocator((void*)ptr);
	}
}

void dngr_swap(DngrDomain* dom, uintptr_t* prot_ptr, uintptr_t new_val, int flags) {
	uintptr_t old_obj;

	old_obj = atomic_exchange(prot_ptr, new_val);
	__dngr_cleanup_ptr(dom, old_obj, flags);
}

int dngr_compare_and_swap(DngrDomain* dom, uintptr_t* prot_ptr, uintptr_t expected_val, uintptr_t new_val, int flags) {

	if (!atomic_cas(prot_ptr, &expected_val, &new_val))
		return 0;

	__dngr_cleanup_ptr(dom, expected_val, flags);
	return 1;
}

void dngr_cleanup(DngrDomain* dom, int flags) {
	DngrPtr* node;
	uintptr_t ptr;

	DNGR_LIST_ITER(&dom->retired, node) {

		ptr = node->ptr;

		if (!__dngr_list_contains(&dom->pointers, ptr)) {

			/* We can deallocate straight away */
			if (__dngr_list_remove(&dom->retired, ptr))
				dom->deallocator((void*)ptr);

		} else if (!(flags & DNGR_DEFER_DEALLOC)) {

			/* Spin until all readers are done, then deallocate */
			while (__dngr_list_contains(&dom->pointers, ptr));
			if (__dngr_list_remove(&dom->retired, ptr))
				dom->deallocator((void*)ptr);
		}
		
	}
}
