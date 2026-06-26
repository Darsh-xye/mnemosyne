#pragma once

#include <cstddef>    // size_t
#include <cstdint>    // uint32_t
#include <sys/mman.h> // mmap

// 7 slab size classes.
// Requests larger than 512 bytes go directly to mmap.
static constexpr int NUM_SIZE_CLASSES = 7;

static constexpr size_t SIZE_CLASSES[NUM_SIZE_CLASSES] = {
    8, 16, 32, 64, 128, 256, 512
};

static constexpr size_t SLAB_SIZE = 2 * 1024 * 1024; // 2 MB

// Node used to link free slots inside a slab.
struct FreeSlot {
    FreeSlot* next;
};

// Metadata stored at the beginning of each slab.
struct SlabHeader {
    size_t      obj_size;     // size of each slot in this slab
    uint32_t    total_slots;  // total number of slots in the slab
    uint32_t    free_slots;   // currently available slots
    FreeSlot*   free_list;    // head of free-list
    SlabHeader* next_slab;    // next slab in the same size class
};

// Returns -1 if the request is too large for slabs.
int get_size_class(size_t size);

// Creates a new slab for a given object size and builds its free list.
SlabHeader* create_slab(size_t obj_size);

void* slab_alloc(SlabHeader*& slab_chain); // Allocates one slot from the slab chain.

void slab_free(void* ptr, SlabHeader* slab_chain); // Frees a slot back to the correct slab in the chain.