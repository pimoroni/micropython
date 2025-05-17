#include "py/runtime.h"
#include <string.h>

void *__wrap_malloc(size_t size) {
    // Allocate an extra sizeof(size_t) bytes
    size_t *addr = (size_t *)m_tracked_calloc(sizeof(uint8_t), size + sizeof(size_t));
    // Tag our memory with its allocated size
    *addr = size;
    // Skip past the size_t size
    addr++;

    mp_printf(&mp_plat_print, "malloc %lu %p : %p\n", size, addr, (uint8_t *)addr + size);

    return (void *)addr;
}

void __wrap_free(void *p) {
    size_t *pp = (size_t *)p;   // Convert our void pointer to size_t* so we can read the size marker
    pp--;                       // Skip back to get our real start
    size_t size = *pp;
    m_tracked_free((void *)pp);

    mp_printf(&mp_plat_print, "free %p (size %d)\n", p, size);
}

void *__wrap_realloc(void *p, size_t size) {
    void *addr = __wrap_malloc(size);
    size_t old_size = *((size_t *)p - 1);
    memcpy(addr, p, old_size < size ? old_size : size);
    __wrap_free(p);

    mp_printf(&mp_plat_print, "realloc %lu -> %lu, %p -> %p : %p\n", old_size, size, p, addr, (uint8_t *)addr + size);

    return addr;
}

void *__wrap_calloc(size_t nitems, size_t size) {
    return __wrap_malloc(size * nitems);
}