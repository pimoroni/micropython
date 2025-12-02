#include "micropython/mp_tracked_allocator.hpp"

#ifdef __cplusplus
extern "C" {
#endif
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
  void *m_malloc(size_t num_bytes);
  void *m_realloc(void *ptr, size_t old_num_bytes, size_t new_num_bytes);
  void m_free(void *ptr, size_t num_bytes);
#else
  void *m_malloc(size_t num_bytes);
  void *m_realloc(void *ptr, size_t new_num_bytes);
  void m_free(void *ptr);
#endif
#ifdef __cplusplus
}
#endif

#define PV_STD_ALLOCATOR MPAllocator
#define PV_MALLOC m_malloc
#define PV_FREE m_free
#define PV_REALLOC m_realloc