# Documentation #

In order to share a pointer safely between threads, a domain needs to be created. A domain protects all of its users, and it is usually associated with an object to which access needs to be protected - if you have two independent objects, it would make sense to use two domains.

A domain is created with `dngr_domain_new`, and its contents deallocated with `dngr_domain_free`. Once this is done, safe pointers to objects can be obtained with `dngr_load`, and dropped with `dngr_drop`. The shared pointer can be made point to a different address with `dngr_swap`.

## API ##

The following documentation can be found in [domain.h](src/domain.h). See below for potential misuses.

```c
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
 * Just like `dngr_swap`, except it swaps the contents of the shared pointer if and only if the old value matches
 * `expected_val`. Returns 1 if the swap succeeded, 0 if it failed because the expected value did not match.
 */
int dngr_compare_and_swap(DngrDomain* dom, uintptr_t* prot_ptr, uintptr_t expected_val, uintptr_t new_val, int flags);

/*
 * Forces the cleanup of old objects that have not been deallocated yet. Just like `dngr_swap`,
 * if `flags` is 0, this function will wait until there are no more references to each object.
 * If `flags` is `DNGR_DEFER_DEALLOC`, only objects that already have no living references will be
 * deallocated.
 */
void dngr_cleanup(DngrDomain* dom, int flags);
```

## Code example ##

The following is a simplified version of the example included in this repository ([thread_prog.c](examples/thread_prog.c)).

In this program, there is a global shared configuration object that will be read and updated concurrently by two different threads. The configuration is allocated with `create_config`, and deallocated with  `delete_config`.

The reader thread dereferences the shared pointer in a loop, and does some stuff with the configuration contents (in the actual code, the configuration is printed). The writer thread creates new configuration objects, and updates the shared pointer to point to them.

Thanks to hazard pointers, we can be sure of two things:
 - `safe_config` will not be freed before the reader calls `dngr_drop`.
 - Old configuration objects swapped out by `dngr_swap` will be freed at some point, preventing memory leaks.

```c
Config* create_config() { ... }
void delete_config(Config*) { ... }

Config* shared_config;
DngrDomain* config_dom;

void init() {
    shared_config = create_config();
    DngrDomain* config_dom = dngr_domain_new(delete_config);
}

void deinit() {
    delete_config(shared_config);
    dngr_domain_free(dom);
}


/* Frequently reads the configuration */
void reader_thread() {
    Config* safe_config;

    while (...) {

        /* Load a safe pointer to the configuration */
        safe_config = dngr_load(config_dom, &shared_config);

        /* do stuff with the configuration */
        /* ... */

        /* We are done with this pointer */
        dngr_drop(config_dom, safe_config);
    }

}

/* Updates the configuration from time to time */
void writer_thread() {
    Config* new_config;

    while (...) {

        new_config = create_config();

        /* modify the new configuration */
        /* ... */

        /* Update with the new configuration */
        dngr_swap(config_dom, &shared_config, new_config, 0);
    }
}

int main() {

    init();

    /* Start threads */
    /* ... */

    deinit();

    return 0;
}
```

## Potential misuses ##

API users must be careful to drop all safe pointers once they are done using them; not doing this can lead to memory leaks or deadlocks. Consider the following example:

```c
    obj = dngr_load(dom, shared);
    new_obj = copy_and_modify_obj(obj);
    dngr_compare_and_swap(dom, shared, obj, new_obj, 0);
    return;
```

Here the current thread holds an undropped reference to `obj`, obtained with `dngr_load`, and since `dngr_compare_and_swap`'s `flags` have not been set to `DNGR_DEFER_DEALLOC`, if the CAS succeeds, the function will wait until all references are dropped; this will never happen, as the thread waiting also holds a reference. There are two possible solutions:
 - Drop `obj` via `dngr_drop` before calling `dngr_compare_and_swap`. Do this only if `ptr` is no longer going to be dereferenced.
 - Set `dngr_compare_and_swap`'s `flags` to `DNGR_DEFER_DEALLOC` and call `dngr_drop` afterwards.