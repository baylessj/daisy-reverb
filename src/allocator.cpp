#include "allocator.hpp"
#include "daisy_petal.h"

// This is used in the modified CloudSeed code for allocating
// 61MB delay line memory to SDRAM (64MB available on Daisy)
#define CUSTOM_POOL_SIZE (61 * 1024 * 1024)
DSY_SDRAM_BSS char custom_pool[CUSTOM_POOL_SIZE];
size_t pool_index = 0;
int allocation_count = 0;
void* customPoolAllocate(size_t size) {
  if (pool_index + size >= CUSTOM_POOL_SIZE) {
    return 0;
  }
  void* ptr = &custom_pool[pool_index];
  pool_index += size;
  return ptr;
}