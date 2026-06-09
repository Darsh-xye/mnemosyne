#pragma once

#include <cstddef>   //size_t
#include <cstdint>   //uint32_t
#include <sys/mman.h> //mmap

//SIZE CLASSES
//These 7 sizes and anything above 512 bytes goes directly to mmap

static constexpr int NUM_SIZE_CLASSES = 7;

//The actual byte sizes
static constexpr size_t SIZE_CLASSES[NUM_SIZE_CLASSES]={
    8, 16, 32, 64, 128, 256, 512
};

static constexpr size_t SLAB_SIZE = 2*1024*1024; 

//Free slots are linked together.
//When a slot is allocated, the user overwrites this memory.
struct FreeSlot {
    FreeSlot* next;  //points to next free slot, null if last
};

// Metadata for a single slab.
// Lives at the start of the 2 MB region.
struct SlabHeader {
    size_t      obj_size;    //size of each slot in this slab
    uint32_t    total_slots; //how many slots were carved at creation
    uint32_t    free_slots;  //how many slots are currently free
    FreeSlot*   free_list;   //head of the free slot linked list
    SlabHeader* next_slab;   //next slab of the same size class
                             //(when this slab fills up, we chain a new one here)
};

//Find the first size class that can fit size
//Returns -1 if the request is too large for slabs
int  get_size_class(size_t size);

//Create a new slab and build its free list
SlabHeader* create_slab(size_t obj_size);

//Allocate one slot from the slab chain
void* slab_alloc(SlabHeader*& slab_chain);

//Free a slot back into the correct slab
void  slab_free(void* ptr, size_t obj_size);