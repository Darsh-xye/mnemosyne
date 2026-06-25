#pragma once

#include "slab.h"
#include <cstddef>
#include <utility>
#include <new>

namespace ma {  // "ma" = memory allocator, change later if you rename project

    void init();
    void shutdown();

    // Raw allocation
    void* alloc(size_t size);
    void  free(void* ptr, size_t size);

    // Typed allocation, placement new wrapper
    // Size is deduced automatically from T, so the user never has to pass it
    template<typename T, typename... Args>
    T* make(Args&&... args) {
        void* mem = alloc(sizeof(T));
        if (!mem) return nullptr;
        return new (mem) T(std::forward<Args>(args)...);
    }

    template<typename T>
    void destroy(T* ptr) {
        if (!ptr) return;
        ptr->~T();
        free((void*)ptr, sizeof(T));
    }

    //  Debug stats
    void dump_stats();

} 