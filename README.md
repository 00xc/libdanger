# libdanger #
A sane and simple implementation of hazard pointers in C with GCC atomic builtins. No overengineering, no classes, no C++.

## What are hazard pointers? ##

Hazard pointers are an approach to lock-free memory management for shared data structres in multithreaded programs. They were initially presented by Andrei Alexandrescu and Maged Michael in their 2004 paper [Lock Free Data Structures with Hazard Pointers](https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.112.6406&rep=rep1&type=pdf). They solve similar problems to reference counting, RCU (Read-Copy-Update) and plain locks. 

Suppose you have a shared pointer to a heap object. Threads dereference this pointer to access the information contained in the object. Suppose now that some thread wants to update this pointer, so that it points to a new, updated object. There are two things that this thread needs to do: swap the old pointer with the new one, and free the old pointer to prevent memory leaks. Swapping is easy - once the pointer is updated, subsequent readers will see the new pointer and dereference it transparently. However, the old object cannot be freed straight away, since some other threads might still hold a reference to it. If we freed it, these threads would access deallocated memory.

We can solve this issue by using hazard pointers - the shared pointer will be updated to point to the new object, but the old one will not be freed until nobody else holds a reference to it.

## Compiling ##

Simply build the library with `make static` and compile your applcation with:

 `cc <your_program>.c libdanger.a -I<path_to_libdanger>/src/`

## Usage ##

In order to share a pointer safely, a domain needs to be created. A domain protects all of its users, and it is usually associated with an object to which access needs to be protected - if you have two independent objects, it would make sense to use two domains.

A domain is created with `dngr_domain_new`, and its contents deallocated with `dngr_domain_free`. Once this is done, safe pointers to objects can be obtained with `dngr_load`, and dropped with `dngr_drop`. The shared pointer can be made point to a different address with `dngr_swap`.

### API ###

The following documentation can be found in [domain.h](src/domain.h)

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
 - `safe_config` will not be freed before the reader calls `dngr_load`.
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