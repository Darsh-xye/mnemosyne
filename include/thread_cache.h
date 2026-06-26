#pragma once

#include "slab.h"
#include <cstddef>
#include <cstdint>

//Per thread cache for each size class.
struct ThreadCache {
    FreeSlot* free_list[NUM_SIZE_CLASSES];
    uint32_t  count[NUM_SIZE_CLASSES];

    ThreadCache() {
        for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
            free_list[i] = nullptr;
            count[i] = 0;
        }
    }
};

//Each thread gets its own cache instance
extern thread_local ThreadCache t_cache;

// Number of slots to pull from central pool at once
static constexpr int CACHE_REFILL_BATCH = 16;

// Max cached slots before flushing some back
static constexpr int CACHE_MAX_SIZE = 32;

// Pop/push a slot from/to the thread-local cache
void* tcache_pop(int size_class);
void  tcache_push(int size_class, void* ptr);