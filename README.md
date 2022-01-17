# libdanger #
A sane and simple implementation of hazard pointers in C with GCC atomic builtins. No overengineering, no classes, no C++.

## What are hazard pointers? ##

Hazard pointers are an approach to lock-free memory management for shared data structres in multithreaded programs. They were initially presented by Andrei Alexandrescu and Maged Michael in their 2004 paper [Lock Free Data Structures with Hazard Pointers](https://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.112.6406&rep=rep1&type=pdf). They solve similar problems to reference counting, RCU (Read-Copy-Update) and plain locks. 

Suppose you have a shared pointer to a heap object. Threads dereference this pointer to access the information contained in the object. Suppose now that some thread wants to update this pointer, so that it points to a new, updated object. There are two things that this thread needs to do: swap the old pointer with the new one, and free the old pointer to prevent memory leaks. Swapping is easy - once the pointer is updated, subsequent readers will see the new pointer and dereference it transparently. However, the old object cannot be freed straight away, since some other threads might still hold a reference to it. If we freed it, these threads would access deallocated memory.

We can solve this issue by using hazard pointers - the shared pointer will be updated to point to the new object, but the old one will not be freed until nobody else holds a reference to it.

## Usage ##

Refer to the [documentation file](DOC.md) for information about the API, code examples and potential misuses.

## Compiling ##

Simply build the library with `make static` and compile your applcation with:

 `cc <your_program>.c libdanger.a -I<path_to_libdanger>/src/`
