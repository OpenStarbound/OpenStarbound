#include "StarMemory.hpp"

#ifdef STAR_USE_JEMALLOC
#include "jemalloc/jemalloc.h"
#elif STAR_USE_MIMALLOC
#include "mimalloc.h"
#elif STAR_USE_RPMALLOC
#include "rpnew.h"

bool rpm_linker_ref() {
  rpmalloc_linker_reference();
  return true;
}

static bool _rpm_linker_ref = rpm_linker_ref();

#endif

namespace Star {

#ifdef STAR_USE_JEMALLOC
#ifdef STAR_JEMALLOC_IS_PREFIXED
  void* malloc(size_t size) {
    return je_malloc(size);
  }

  void* realloc(void* ptr, size_t size) {
    return je_realloc(ptr, size);
  }

  void free(void* ptr) {
    je_free(ptr);
  }

  void free(void* ptr, size_t size) {
    if (ptr)
      je_sdallocx(ptr, size, 0);
  }
#else
  void* malloc(size_t size) {
    return ::malloc(size);
  }

  void* realloc(void* ptr, size_t size) {
    return ::realloc(ptr, size);
  }

  void free(void* ptr) {
    ::free(ptr);
  }

  void free(void* ptr, size_t size) {
    ::free(ptr);
  }
#endif
#elif STAR_USE_MIMALLOC
  void* malloc(size_t size) {
  return mi_malloc(size);
  }

  void* realloc(void* ptr, size_t size) {
    return mi_realloc(ptr, size);
  }

  void free(void* ptr) {
    return mi_free(ptr);
  }

  void free(void* ptr, size_t size) {
    return mi_free_size(ptr, size);
  }
#elif STAR_USE_RPMALLOC
  void* malloc(size_t size) {
    return rpmalloc(size);
  }

  void* realloc(void* ptr, size_t size) {
    return rprealloc(ptr, size);
  }

  void free(void* ptr) {
    return rpfree(ptr);
  }

  void free(void* ptr, size_t) {
    return rpfree(ptr);
  }
#else
  void* malloc(size_t size) {
    return ::malloc(size);
  }

  void* realloc(void* ptr, size_t size) {
    return ::realloc(ptr, size);
  }

  void free(void* ptr) {
    return ::free(ptr);
  }

  void free(void* ptr, size_t) {
    return ::free(ptr);
  }
#endif

}

#ifndef  STAR_USE_RPMALLOC


void* operator new(std::size_t size) {
  auto ptr = Star::malloc(size);
  if (!ptr)
    throw std::bad_alloc();
  return ptr;
}

void* operator new[](std::size_t size) {
  auto ptr = Star::malloc(size);
  if (!ptr)
    throw std::bad_alloc();
  return ptr;
}

// Globally override new and delete.  As the per standard, new and delete must
// be defined in global scope, and must not be inline.

void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
  return Star::malloc(size);
}

void* operator new[](std::size_t size, std::nothrow_t const&) noexcept {
  return Star::malloc(size);
}

void operator delete(void* ptr) noexcept {
  Star::free(ptr);
}

void operator delete[](void* ptr) noexcept {
  Star::free(ptr);
}

void operator delete(void* ptr, std::nothrow_t const&) noexcept {
  Star::free(ptr);
}

void operator delete[](void* ptr, std::nothrow_t const&) noexcept {
  Star::free(ptr);
}

void operator delete(void* ptr, std::size_t size) noexcept {
  Star::free(ptr, size);
}

void operator delete[](void* ptr, std::size_t size) noexcept {
  Star::free(ptr, size);
}

#endif 