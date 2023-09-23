/**
 * A custom memory allocator for using a section of contiguous SDRAM space.
 */
#pragma once

#include <cstddef>

void* customPoolAllocate(size_t size);

template <typename T> T* sdramAllocate(std::size_t n) {
  if (auto p = static_cast<T*>(customPoolAllocate(sizeof(T) * n))) {
    return p;
  }
  return nullptr;
}