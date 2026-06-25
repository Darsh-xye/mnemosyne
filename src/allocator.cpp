#include "allocator.h"
#include "slab.h"
#include <iostream>
#include <sys/mman.h>

namespace ma {

// One slab chain per size class.
// nullptr means no slab has been created yet for this class.
static SlabHeader* g_slab_chains[NUM_SIZE_CLASSES] = {
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

// Stats for dump_stats()
static size_t g_total_allocs   = 0;
static size_t g_total_frees    = 0;
static size_t g_large_allocs   = 0; // allocations bigger than 512 bytes

void init() {
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        g_slab_chains[i] = nullptr;
    }
    g_total_allocs = 0;
    g_total_frees  = 0;
    g_large_allocs = 0;
}

void shutdown() {
    dump_stats();

    // OS reclaims all process memory on exit anyway.
    // Real allocators DO clean up — left as a future improvement.
}

void* alloc(size_t size) {
    if (size == 0) size = 1; // handle alloc(0) gracefully

    int cls = get_size_class(size);


    if (cls == -1) {
        void* mem = mmap(
            nullptr, size,
            PROT_READ | PROT_WRITE,
            MAP_ANONYMOUS | MAP_PRIVATE,
            -1, 0
        );
        if (mem == MAP_FAILED) return nullptr;
        g_large_allocs++;
        return mem;
    }

    size_t obj_size = SIZE_CLASSES[cls];

    // First ever allocation for this size class z create the very first slab manually (slab_alloc assumes a chain
    // if already exists so we bootstrap it here)
    if (g_slab_chains[cls] == nullptr) {
        g_slab_chains[cls] = create_slab(obj_size);
        if (!g_slab_chains[cls]) return nullptr;
    }

    void* ptr = slab_alloc(g_slab_chains[cls]);
    if (ptr) g_total_allocs++;
    return ptr;
}

void free(void* ptr, size_t size) {
    if (!ptr) return;

    int cls = get_size_class(size);

    //Large allocation , was direct mmap, must munmap 
    if (cls == -1) {
        munmap(ptr, size);
        return;
    }

    //Small allocation then return to its slab 
    slab_free(ptr, g_slab_chains[cls]);
    g_total_frees++;
}

void dump_stats() {
    std::cout << "Memory Allocator Stats \n";
    std::cout << "Total small allocs : " << g_total_allocs << "\n";
    std::cout << "Total frees        : " << g_total_frees  << "\n";
    std::cout << "Total large allocs : " << g_large_allocs << "\n";
    for (int i = 0; i < NUM_SIZE_CLASSES; i++) {
        if (g_slab_chains[i]) {
            std::cout << "Size class " << SIZE_CLASSES[i]
                      << " bytes -> slabs allocated\n";
        }
    }
    std::cout << "===================================\n";
}

} 