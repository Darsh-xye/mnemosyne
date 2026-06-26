#include "allocator.h"
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>
#include <atomic>

void worker_basic(int thread_id, int iterations) {
    for (int i = 0; i < iterations; i++) {
        void* p = ma::alloc(32);
        assert(p != nullptr);
        ma::free(p, 32);
    }
}

void test_concurrent_alloc_free() {
    const int NUM_THREADS = 8;
    const int ITERATIONS  = 50000;

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker_basic, i, ITERATIONS);
    }
    for (auto& t : threads) t.join();

    std::cout << "[PASS] " << NUM_THREADS << " threads x "
              << ITERATIONS << " alloc/free each, no crash\n";
}

void worker_hold_then_free(std::vector<void*>& out, int count) {
    for (int i = 0; i < count; i++) {
        void* p = ma::alloc(64);
        assert(p != nullptr);
        out.push_back(p);
    }
}

void test_hold_then_free() {
    const int NUM_THREADS = 8;
    const int COUNT       = 10000;

    std::vector<std::vector<void*>> all_ptrs(NUM_THREADS);
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker_hold_then_free, std::ref(all_ptrs[i]), COUNT);
    }
    for (auto& t : threads) t.join();

    for (auto& ptrs : all_ptrs) {
        for (void* p : ptrs) {
            ma::free(p, 64);
        }
    }

    std::cout << "[PASS] hold-then-free across threads\n";
}

void worker_mixed_sizes(int iterations) {
    size_t sizes[] = {8, 32, 64, 128, 256, 512};
    for (int i = 0; i < iterations; i++) {
        size_t s = sizes[i % 6];
        void* p = ma::alloc(s);
        assert(p != nullptr);
        ma::free(p, s);
    }
}

void test_mixed_sizes_concurrent() {
    const int NUM_THREADS = 8;
    const int ITERATIONS  = 20000;

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker_mixed_sizes, ITERATIONS);
    }
    for (auto& t : threads) t.join();

    std::cout << "[PASS] mixed size classes across " << NUM_THREADS << " threads\n";
}

void worker_stress_refill(int iterations) {
    for (int i = 0; i < iterations; i++) {
        void* p = ma::alloc(128);
        assert(p != nullptr);
        if (i % 10 == 0) ma::free(p, 128);
    }
}

void test_stress_slab_refill() {
    const int NUM_THREADS = 16;
    const int ITERATIONS  = 5000;

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker_stress_refill, ITERATIONS);
    }
    for (auto& t : threads) t.join();

    std::cout << "[PASS] stress test on mutex-protected slab refill path\n";
}

void worker_large_alloc(int iterations) {
    for (int i = 0; i < iterations; i++) {
        void* p = ma::alloc(4096);
        assert(p != nullptr);
        ma::free(p, 4096);
    }
}

void test_concurrent_large_alloc() {
    const int NUM_THREADS = 8;
    const int ITERATIONS  = 2000;

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker_large_alloc, ITERATIONS);
    }
    for (auto& t : threads) t.join();

    std::cout << "[PASS] concurrent large (mmap-direct) allocations\n";
}

int main() {
    ma::init();

    test_concurrent_alloc_free();
    test_hold_then_free();
    test_mixed_sizes_concurrent();
    test_stress_slab_refill();
    test_concurrent_large_alloc();

    ma::shutdown();

    std::cout << "\nALL THREAD TESTS PASSED\n";
    return 0;
}