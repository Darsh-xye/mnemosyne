#pragma once

#include "slab.h"
#include <atomic>
#include <cstdint>

struct TaggedPtr {
    FreeSlot* ptr;
    uint64_t  tag;

    bool operator==(const TaggedPtr& other) const {
        return ptr == other.ptr && tag == other.tag;
    }
};

struct alignas(64) PaddedAtomicTaggedPtr {
    std::atomic<TaggedPtr> value;
};

extern PaddedAtomicTaggedPtr g_central_pool[NUM_SIZE_CLASSES];

void central_push(int size_class, FreeSlot* node);
FreeSlot* central_pop(int size_class);
void central_push_batch(int size_class, FreeSlot* head, int count);
FreeSlot* central_pop_batch(int size_class, int count);
