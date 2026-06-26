# Mnemosyne
### A slab-based memory allocator with thread-local caching for multithreaded small-object allocation in C++

Mnemosyne is a custom **C++17 memory allocator** for efficient **small-object allocation**. It combines a **size-class slab allocator** with **thread-local caches** to reduce allocation overhead, improve memory reuse, and scale under concurrent workloads.

Allocations up to **512 bytes** are served from **2 MB mmap-backed slabs** using intrusive free lists and per-thread caches. Larger allocations bypass the slab allocator and are handled directly with `mmap`.

---

## Highlights

- Slab-based allocator in **C++17**
- **7 size classes:** `8, 16, 32, 64, 128, 256, 512`
- **2 MB mmap-backed slabs**
- O(1) allocation/deallocation using intrusive free lists
- Thread-local caches for fast-path allocation
- Shared central refill path
- Direct `mmap` for allocations larger than `512 B`
- Typed allocation helpers (`make<T>()` / `destroy<T>()`)

---

# Motivation

Mnemosyne explores techniques used by modern memory allocators to optimize frequent small allocations:

- Size-class slab allocation
- Thread-local fast paths
- Shared refill paths
- `mmap`-backed memory management
- Reduced contention under concurrency

The implementation is intentionally compact while demonstrating techniques used in allocators such as **jemalloc** and **tcmalloc**.

---

# Architecture

## Slab Allocator

Small objects are allocated from **2 MB slabs** divided into seven size classes:

`8 B • 16 B • 32 B • 64 B • 128 B • 256 B • 512 B`

Each slab maintains an intrusive free list, enabling constant-time allocation and deallocation.

---

## Thread-Local Cache

Each thread maintains a local cache to reduce synchronization overhead.

Allocation flow:

1. Round the request to the nearest size class.
2. Allocate from the thread-local cache.
3. Refill from the shared central allocator on a cache miss.
4. Return the allocated object.

This keeps the common allocation path thread-local and minimizes contention.

---

# Allocation Strategy

### Small Allocations (`≤512 B`)

- Round to the nearest size class
- Allocate from the thread-local cache
- Refill from the central allocator when needed

### Large Allocations (`>512 B`)

Large objects bypass the slab allocator and are allocated directly using `mmap`.

---

# Size Classes

| Requested Size | Rounded Class | Allocation Path |
|----------------|---------------|-----------------|
| `1–8 B` | `8 B` | Slab / Thread Cache |
| `9–16 B` | `16 B` | Slab / Thread Cache |
| `17–32 B` | `32 B` | Slab / Thread Cache |
| `33–64 B` | `64 B` | Slab / Thread Cache |
| `65–128 B` | `128 B` | Slab / Thread Cache |
| `129–256 B` | `256 B` | Slab / Thread Cache |
| `257–512 B` | `512 B` | Slab / Thread Cache |
| `>512 B` | Direct | `mmap` |

---

# Features

- Size-class slab allocation
- 2 MB `mmap`-backed slabs
- Intrusive free-list management
- Thread-local caches
- Shared central refill path
- Direct `mmap` for large allocations
- Typed allocation helpers

---

# Build

```bash
mkdir build
cd build
cmake ..
make
```

---

# Run

```bash
./test_slab      # Correctness tests
./test_threads   # Multithreaded tests
./bench          # Benchmark against glibc malloc
```

---

# Benchmark

Mnemosyne was benchmarked against the system **glibc `malloc/free`** allocator across multiple allocation patterns.

### Best Result

| Workload | glibc | Mnemosyne |
|----------|------:|----------:|
| Allocate 500,000 objects, hold, then free | 56.236 ms | **23.334 ms** |

**Result:** **2.41× faster** than glibc for this workload.

The improvement comes from slab allocation, constant-time free-list operations, and efficient object reuse.

Across the remaining benchmark scenarios—including fixed-size, varied-size, multithreaded, and large allocations—**glibc outperformed Mnemosyne**. These results reflect the maturity of highly optimized production allocators and the current scope of this educational implementation.

---

# Example

```cpp
#include "allocator.h"

struct Node {
    int x, y;

    Node(int a, int b) : x(a), y(b) {}
};

int main() {
    ma::init();

    void* p = ma::alloc(64);
    ma::free(p, 64);

    auto* node = ma::make<Node>(1, 2);
    ma::destroy(node);

    ma::shutdown();
    return 0;
}
```

---

# References

This project was developed with guidance from the following papers and documentation:

- **Bonwick (1994)** — *The Slab Allocator*  
  https://www.usenix.org/conference/usenix-summer-1994-technical-conference/slab-allocator-object-caching-kernel

- **Bonwick & Adams (2001)** — *Magazines and Vmem*  
  https://www.usenix.org/legacy/event/usenix01/full_papers/bonwick/bonwick.pdf

- **Google** — *TCMalloc Design Documentation*  
  https://google.github.io/tcmalloc/design.html

- **Evans (2006)** — *A Scalable Concurrent malloc(3) Implementation for FreeBSD (jemalloc)*  
  https://people.freebsd.org/~jasone/jemalloc/bsdcan2006/jemalloc.pdf

- **Michael & Scott (1996)** — *Non-Blocking Concurrent Queue Algorithms*  
  https://www.cs.rochester.edu/u/scott/papers/1996_PODC_queues.pdf

- **glibc Wiki** — *Malloc Internals*  
  https://sourceware.org/glibc/wiki/MallocInternals
