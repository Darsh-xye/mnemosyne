# Memory Allocator

A memory allocator written from scratch in C++. Built to understand how 
allocators like jemalloc and tcmalloc work under the hood.

## What it does
- Slab allocation with size classes for O(1) alloc and free
- Lock-free thread-local caches so multiple threads don't block each other
- Generational garbage collector for automatic memory reclamation

## Build
```bash
mkdir build && cd build
cmake ..
make
```

## Run tests
```bash
./test_slab
./test_threads
```

## Status
Work in progress.
