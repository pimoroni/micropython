#include "py/runtime.h"

#define DEBUG_printf(...) // mp_printf(&mp_plat_print, __VA_ARGS__)

void *__wrap_calloc(size_t count, size_t size) {
    DEBUG_printf("calloc: %u\n", size * count);
    void* result = m_tracked_calloc(count, size);
    if(result == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("calloc failed"));
    }
    return result;
};

void *__wrap_malloc(size_t size) {
    DEBUG_printf("malloc: %u\n", size);
    void* result = m_tracked_calloc(1, size);
    if(result == NULL) {
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("malloc failed"));
    }
    return result;
};

void *__wrap_realloc(void *mem, size_t size) {
    return NULL;
};

void __wrap_free(void *mem) {
    DEBUG_printf("free: %lu\n", (uint32_t)mem);
    m_tracked_free(mem);
};