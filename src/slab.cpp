#include "slab.h"
#include <cassert>
#include <cstdlib>  // nullptr

int get_size_class(size_t size) {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        if (size <= SIZE_CLASSES[i]) {
            return i;
        }
    }
    return -1; 
}
/*
Creates a new slab for a size class when no free slots remain.

Allocates SLAB_SIZE bytes with mmap, sets up the slab header,
splits the rest into slots, and links them into the free list.
*/
SlabHeader* create_slab(size_t obj_size) {
    // Asking OS for 2mb of anonymous memory
    // MAP_ANONYMOUS = not backed by any file, just raw memory
    // MAP_PRIVATE   = private to this process
    void* mem = mmap(
        nullptr,               
        SLAB_SIZE,            
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        -1,                    
        0                      
    );

    if (mem == MAP_FAILED) return nullptr;

    
    SlabHeader* slab = (SlabHeader*)mem;  // Place the SlabHeader at the very start of the 2MB block
    slab->obj_size   = obj_size;
    slab->next_slab  = nullptr;

    char* slots_start = (char*)mem + sizeof(SlabHeader);

    // How many slots fit in the remaining space?
    size_t usable_space = SLAB_SIZE - sizeof(SlabHeader);
    uint32_t num_slots  = (uint32_t)(usable_space / obj_size);

    slab->total_slots = num_slots;
    slab->free_slots  = num_slots;

    // Build the intrusive free list
    // Each free slot's first bytes point to the next free slot
    // Last slot points to nullptr
    slab->free_list = nullptr;
    for (uint32_t i = 0; i < num_slots; i++) {
        // Get pointer to this slot
        FreeSlot* slot = (FreeSlot*)(slots_start + i * obj_size);
        // Prepend to free list (build it in reverse, doesn't matter)
        slot->next      = slab->free_list;
        slab->free_list = slot;
    }

    return slab;
}

//  slab_alloc
//  Pops one slot from the free list of the
//  slab chain. If current slab is full,
//  creates a new slab and chains it.

void* slab_alloc(SlabHeader*& slab_chain) {

    SlabHeader* slab = slab_chain;
    while (slab && slab->free_slots == 0) {
        slab = slab->next_slab;
    }

    // All slabs full then create a new one
    if (!slab) {
        slab = create_slab(slab_chain->obj_size);
        if (!slab) return nullptr; // mmap failed

        // Prepend new slab to the chain
        slab->next_slab = slab_chain;
        slab_chain      = slab;
    }

    // Pop the head of the free list
    FreeSlot* slot  = slab->free_list;
    slab->free_list = slot->next;
    slab->free_slots--;

    return (void*)slot;
}

void slab_free(void* ptr, SlabHeader* slab_chain) {
    // Find which slab in the chain owns this pointer
    SlabHeader* slab = slab_chain;
    while (slab) {
        char* slab_start = (char*)slab;
        char* slab_end   = slab_start + SLAB_SIZE;
        if ((char*)ptr >= slab_start && (char*)ptr < slab_end) {
            // Found it — push ptr back onto free list
            FreeSlot* slot  = (FreeSlot*)ptr;
            slot->next      = slab->free_list;
            slab->free_list = slot;
            slab->free_slots++;
            return;
        }
        slab = slab->next_slab;
    }
    // If we get here, ptr doesn't belong to any slab
    // This means it was a large allocation (direct mmap)
    // handled separately in helios.cpp
}