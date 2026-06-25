# Mnemosyne
### A multithreaded slab allocator with thread-local caching for small-object allocation in C++

Mnemosyne is a custom **C++17 memory allocator** designed for efficient **small-object allocation** under multithreaded workloads. It combines a **size-class slab allocator** with **per-thread caches** to reduce allocation latency, improve reuse locality, and minimize contention on the fast path.

The allocator currently targets allocations up to **512 bytes** using **mmap-backed slabs**, **intrusive free lists**, and **thread-local freelists**. Requests larger than 512 bytes bypass the slab layer and are served directly through `mmap`.

---

## Highlights

- **Size-class slab allocator** for small objects
- **7 fixed size classes**: `8, 16, 32, 64, 128, 256, 512`
- **2 MB slabs** acquired via `mmap`
- **Intrusive free-list** slot management for O(1) allocation/deallocation
- **Thread-local caches** to avoid shared allocator traffic on the hot path
- **Central refill path** for replenishing local caches
- **Large-object mmap bypass** for requests `> 512 B`
- **Typed allocation helpers** such as `make<T>()` / `destroy<T>()`

---

## Motivation

General-purpose allocators are built for broad workloads. Mnemosyne explores the allocator techniques that make **small-object allocation** fast and scalable in practice:

- **segregated size classes**
- **slab-based allocation**
- **thread-local fast paths**
- **central refill / shared backing allocator**
- **mmap-backed memory acquisition**

The project is inspired by allocator design ideas used in systems such as **jemalloc** and **tcmalloc**, while keeping the implementation compact and easy to reason about.

---

# Architecture

Mnemosyne is built around two core layers:

## 1. Slab allocator
Small allocations are handled using **fixed-size slab classes**. Each slab stores objects of a single size class and maintains its free slots using an **intrusive freelist**.

### Current size classes
- `8 B`
- `16 B`
- `32 B`
- `64 B`
- `128 B`
- `256 B`
- `512 B`

Each class is backed by a **2 MB slab** obtained from the OS via `mmap`.

---

## 2. Thread-local cache layer
To reduce allocator contention in multithreaded workloads, Mnemosyne maintains **per-thread caches** for slab-managed size classes.

Typical allocation flow:
1. Round request size to the nearest size class.
2. Try to allocate from the calling thread’s local cache.
3. If the cache is empty, refill from the **central slab-backed allocator**.
4. Return a slot from the local freelist.

This makes the common allocation path **thread-local**, reducing synchronization overhead and improving temporal locality.

---

# Allocation Strategy

## Small allocations (`<= 512 B`)
Requests up to 512 bytes are served from the slab system:

- map request to nearest size class
- allocate from thread-local cache if possible
- refill from the central allocator on cache miss
- return a free slot from the corresponding slab

## Large allocations (`> 512 B`)
Requests larger than 512 bytes bypass the slab allocator and are handled directly through `mmap`.

This keeps the slab layer specialized for small objects and avoids forcing large allocations into fixed-size classes.

---

# Size-Class Table

| Requested Size | Rounded Class | Allocation Path |
|----------------|---------------|-----------------|
| `1 - 8`        | `8 B`         | slab / thread cache |
| `9 - 16`       | `16 B`        | slab / thread cache |
| `17 - 32`      | `32 B`        | slab / thread cache |
| `33 - 64`      | `64 B`        | slab / thread cache |
| `65 - 128`     | `128 B`       | slab / thread cache |
| `129 - 256`    | `256 B`       | slab / thread cache |
| `257 - 512`    | `512 B`       | slab / thread cache |
| `> 512`        | direct        | `mmap` |

---

# Core Components

## Slabs
Each slab stores metadata such as:
- object size
- total slot count
- free slot count
- freelist head
- slab linkage / allocator bookkeeping

## Free slots
Free blocks are linked using an **intrusive next-pointer layout**, allowing constant-time push/pop on the freelist.

## Thread caches
Each thread maintains local free storage for slab-managed classes, allowing most small allocations and frees to avoid the shared path.

## Central allocator / refill path
When a thread cache is empty, it refills from a central slab-backed source responsible for slab creation and object distribution.

---

# Implemented Features

Mnemosyne currently includes:

- **slab allocation** for fixed-size small objects
- **7 size classes** from `8 B` to `512 B`
- **2 MB mmap-backed slabs**
- **intrusive free-list slot management**
- **thread-local caches** for small allocations
- **central refill path** for local cache replenishment
- **large allocation bypass** using direct `mmap`
- **typed object allocation helpers** on top of the allocator

---

# Example API

> The exact API may vary slightly depending on the current branch / file names, but the intended usage looks like this:

```cpp
#include "mnemosyne.h"

int main() {
    void* p = mnemosyne::alloc(64);
    mnemosyne::free(p, 64);

    auto* obj = mnemosyne::make<MyType>(42, "hello");
    mnemosyne::destroy(obj);

    return 0;
}
