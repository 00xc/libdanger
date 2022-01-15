#ifndef __DNGR_ATOMIC_H
#define __DNGR_ATOMIC_H

#define atomic_load(src)                   __atomic_load_n(src, __ATOMIC_SEQ_CST)
#define atomic_store(dst, val)             __atomic_store(dst, val, __ATOMIC_SEQ_CST)
#define atomic_exchange(ptr, val)          __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_cas(dst, expected, desired) __atomic_compare_exchange(dst, expected, desired, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#endif