#include "allocator.h"
#include "thread_cache.h"

#include <atomic>
#include <iostream>
#include <sys/mman.h>

using namespace std;

namespace ma {

// Global statistics
static atomic<size_t> g_total_allocs{0};
static atomic<size_t> g_total_frees{0};
static atomic<size_t> g_large_allocs{0};

// Per-thread counters
struct ThreadStats {
    size_t allocs = 0;
    size_t frees = 0;

    ~ThreadStats() {
        g_total_allocs.fetch_add(allocs, memory_order_relaxed);
        g_total_frees.fetch_add(frees, memory_order_relaxed);
    }
};

thread_local ThreadStats t_stats;

void init() {
    g_total_allocs = 0;
    g_total_frees = 0;
    g_large_allocs = 0;
}

void shutdown() {
    dump_stats();
}

void* alloc(size_t size) {
    if (size == 0) size = 1;

    int cls = get_size_class(size);

    // Large allocation
    if (cls == -1) {
        void* mem = mmap(nullptr, size,
                         PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS | MAP_PRIVATE,
                         -1, 0);

        if (mem == MAP_FAILED)
            return nullptr;

        g_large_allocs.fetch_add(1, memory_order_relaxed);
        return mem;
    }

    // Small allocation
    void* ptr = tcache_pop(cls);
    if (ptr)
        t_stats.allocs++;

    return ptr;
}

void free(void* ptr, size_t size) {
    if (!ptr)
        return;

    int cls = get_size_class(size);

    // Large free
    if (cls == -1) {
        munmap(ptr, size);
        return;
    }

    // Small free
    tcache_push(cls, ptr);
    t_stats.frees++;
}

void dump_stats() {
    cout << "===== Memory Allocator Stats =====\n";
    cout << "Total small allocs : " << g_total_allocs.load() << '\n';
    cout << "Total frees        : " << g_total_frees.load() << '\n';
    cout << "Total large allocs : " << g_large_allocs.load() << '\n';
}

} // namespace ma