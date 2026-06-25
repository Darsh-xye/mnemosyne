# Mnemosyne
### A slab-based memory allocator with thread-local caching for multithreaded small-object allocation in C++

Mnemosyne is a custom **C++17 memory allocator** built for efficient **small-object allocation**. It combines a **size-class slab allocator** with **thread-local caches** to reduce allocation overhead, improve memory reuse, and scale better under **multithreaded / concurrent allocation workloads**.

The allocator currently manages allocations up to **512 bytes** using **mmap-backed slabs**, **intrusive free lists**, and **per-thread free lists**. Requests larger than 512 bytes bypass the slab layer and are handled directly through `mmap`.

---

## Highlights

- **Slab-based small-object allocator** in **C++17**
- **7 size classes**: `8, 16, 32, 64, 128, 256, 512`
- **2 MB slabs** acquired via `mmap`
- **Intrusive free-list slot management** for O(1) allocation/deallocation
- **Thread-local caches** for low-contention fast-path allocation
- **Shared central refill path** for multithreaded allocation workloads
- **Direct mmap path** for allocations larger than `512 B`
- **Typed allocation helpers** such as `make<T>()` / `destroy<T>()`

---

# Motivation

Mnemosyne was built to explore the allocator techniques commonly used to speed up **small, frequent allocations** in systems code:

- **size-class slab allocation**
- **thread-local fast paths**
- **shared refill paths for multithreaded workloads**
- **mmap-backed memory management**
- **reduced allocator contention under concurrency**

The project is inspired by design ideas used in allocators such as **jemalloc** and **tcmalloc**, while keeping the implementation compact and easy to study.

---

# Architecture

Mnemosyne is built around two core components:

## 1. Slab allocator
Small allocations are served through **fixed-size slabs** organized into **7 size classes**:

- `8 B`
- `16 B`
- `32 B`
- `64 B`
- `128 B`
- `256 B`
- `512 B`

Each size class is backed by a **2 MB slab** obtained through `mmap`.  
Free slots inside a slab are tracked using an **intrusive freelist**.

---

## 2. Thread-local cache layer
To improve performance under **multithreaded allocation workloads**, Mnemosyne maintains **per-thread caches** for slab-managed objects.

Typical allocation flow:

1. Round the request to the nearest size class.
2. Try to allocate from the calling thread’s local cache.
3. On a cache miss, refill from the **shared central allocator path**.
4. Return a slot from the local freelist.

This keeps the common allocation path thread-local and reduces contention on shared allocator state.

---

# Allocation Strategy

## Small allocations (`<= 512 B`)
Requests up to `512 B` are handled by the slab allocator:

- map size to nearest class
- allocate from thread-local cache if available
- otherwise refill from the central slab-backed allocator

## Large allocations (`> 512 B`)
Requests larger than `512 B` bypass the slab allocator and are served directly via `mmap`.

---

# Size Classes

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

# Implemented Features

Mnemosyne currently includes:

- **size-class slab allocation** for small objects
- **2 MB mmap-backed slabs**
- **intrusive free-list management**
- **thread-local caches / per-thread free lists**
- **shared central refill path**
- **large-object direct mmap path**
- **typed object allocation helpers**

---

# Example API

```cpp
#include "mnemosyne.h"

struct Node {
    int x, y;
    Node(int a, int b) : x(a), y(b) {}
};

int main() {
    void* p = mnemosyne::alloc(64);
    mnemosyne::free(p, 64);

    auto* node = mnemosyne::make<Node>(1, 2);
    mnemosyne::destroy(node);

    return 0;
}
