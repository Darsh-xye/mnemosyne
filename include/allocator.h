#pragma once

#include "slab.h"
#include <cstddef>
#include <utility>
#include <new>

namespace ma {

    //Reset allocator state / counters.
    void init();
    void shutdown();

    //Raw allocation API.
    void* alloc(size_t size);
    void  free(void* ptr, size_t size);

    //Allocate memory for T and construct it in place.
    template<typename T, typename... Args>
    T* make(Args&&... args) {
        void* mem = alloc(sizeof(T));
        if (!mem) return nullptr;
        return new (mem) T(std::forward<Args>(args)...);
    }

    //Destroy T and return its memory to the allocator.
    template<typename T>
    void destroy(T* ptr) {
        if (!ptr) return;
        ptr->~T();
        free((void*)ptr, sizeof(T));
    }

    //Print allocator stats.
    void dump_stats();

} 