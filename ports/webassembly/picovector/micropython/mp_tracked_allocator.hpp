#include <cstdlib>
#include <limits>
#include <new>
#include <iostream>
#include <cstdint>

#ifndef MPTALLOCATOR_H
#define MPTALLOCATOR_H

extern "C" {
#include "py/runtime.h"
}

template<class T>
struct MPAllocator
{
    typedef T value_type;

    MPAllocator() = default;

    template<class U>
    constexpr MPAllocator(const MPAllocator <U>&) noexcept {}

    [[nodiscard]] T* allocate(std::size_t n)
    {
        if (auto p = static_cast<T*>(m_malloc(n * sizeof(T))))
        {
            return p;
        }
        mp_raise_msg_varg(&mp_type_RuntimeError, MP_ERROR_TEXT("Failed to allocate %lu bytes!"), n);
        return NULL;
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
        m_free(p, n);
#else
        m_free(p);
#endif
    }
};

template<class T, class U>
bool operator==(const MPAllocator <T>&, const MPAllocator <U>&) { return true; }

template<class T, class U>
bool operator!=(const MPAllocator <T>&, const MPAllocator <U>&) { return false; }

#endif // MPTALLOCATOR_H
