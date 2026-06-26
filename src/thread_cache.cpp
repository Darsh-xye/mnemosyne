#include "thread_cache.h"
#include "central_pool.h"
#include "slab_refill.h"

thread_local ThreadCache t_cache;

void* tcache_pop(int size_class) {
    // Refill from central pool or slab refill if cache is empty.
    if (t_cache.free_list[size_class] == nullptr) {
        FreeSlot* batch = central_pop_batch(size_class, CACHE_REFILL_BATCH);

        if (!batch) {
            batch = slab_refill_locked(size_class, CACHE_REFILL_BATCH);
        }

        if (!batch) return nullptr;

        t_cache.free_list[size_class] = batch;

        int n = 0;
        FreeSlot* cur = batch;
        while (cur) { n++; cur = cur->next; }
        t_cache.count[size_class] = n;
    }

    // Pop one slot from the local cache.
    FreeSlot* slot = t_cache.free_list[size_class];
    t_cache.free_list[size_class] = slot->next;
    t_cache.count[size_class]--;

    return (void*)slot;
}

void tcache_push(int size_class, void* ptr) {
    // Push slot into the local cache.
    FreeSlot* slot = (FreeSlot*)ptr;
    slot->next = t_cache.free_list[size_class];
    t_cache.free_list[size_class] = slot;
    t_cache.count[size_class]++;

    // Flush half back if the cache gets too large.
    if (t_cache.count[size_class] > CACHE_MAX_SIZE) {
        int flush_count = t_cache.count[size_class] / 2;

        FreeSlot* head = t_cache.free_list[size_class];
        FreeSlot* tail = head;
        for (int i = 0; i < flush_count - 1; i++) {
            tail = tail->next;
        }

        t_cache.free_list[size_class] = tail->next;
        tail->next = nullptr;

        central_push_batch(size_class, head, flush_count);
        t_cache.count[size_class] -= flush_count;
    }
}