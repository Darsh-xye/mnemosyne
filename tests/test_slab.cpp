#include "allocator.h"
#include <iostream>
#include <cassert>
#include <vector>

// A simple test struct with a constructor and destructor
// so we can verify make<T>() and destroy<T>() actually
// call them correctly
struct TestObj {
    int value;
    static int constructor_calls;
    static int destructor_calls;

    TestObj(int v) : value(v) {
        constructor_calls++;
    }
    ~TestObj() {
        destructor_calls++;
    }
};
int TestObj::constructor_calls = 0;
int TestObj::destructor_calls  = 0;


void test_basic_alloc_free() {
    void* p = ma::alloc(20);       // rounds up to 32-byte class
    assert(p != nullptr);
    ma::free(p, 20);
    std::cout << "[PASS] basic alloc/free\n";
}

void test_alloc_zero() {
    void* p = ma::alloc(0);        // should not crash
    assert(p != nullptr);
    ma::free(p, 0);
    std::cout << "[PASS] alloc(0) handled gracefully\n";
}

void test_large_alloc() {
    void* p = ma::alloc(2048);     // bigger than 512, direct mmap
    assert(p != nullptr);
    ma::free(p, 2048);
    std::cout << "[PASS] large allocation (direct mmap)\n";
}

void test_free_list_reuse() {
    // Allocate, free, allocate again — should get the SAME
    // address back since slab free lists are LIFO
    void* p1 = ma::alloc(32);
    ma::free(p1, 32);
    void* p2 = ma::alloc(32);
    assert(p1 == p2);
    ma::free(p2, 32);
    std::cout << "[PASS] free list reuse (LIFO behavior)\n";
}

void test_many_allocations() {
    std::vector<void*> ptrs;
    for (int i = 0; i < 100000; i++) {
        void* p = ma::alloc(32);
        assert(p != nullptr);
        ptrs.push_back(p);
    }
    for (void* p : ptrs) {
        ma::free(p, 32);
    }
    std::cout << "[PASS] 100,000 allocations and frees, no crash\n";
}

void test_make_and_destroy() {
    TestObj::constructor_calls = 0;
    TestObj::destructor_calls  = 0;

    TestObj* obj = ma::make<TestObj>(42);
    assert(obj != nullptr);
    assert(obj->value == 42);
    assert(TestObj::constructor_calls == 1);

    ma::destroy(obj);
    assert(TestObj::destructor_calls == 1);

    std::cout << "[PASS] make<T>() and destroy<T>() call ctor/dtor correctly\n";
}

void test_mixed_sizes() {
    void* a = ma::alloc(8);
    void* b = ma::alloc(64);
    void* c = ma::alloc(256);
    assert(a && b && c);
    ma::free(a, 8);
    ma::free(b, 64);
    ma::free(c, 256);
    std::cout << "[PASS] mixed size class allocations\n";
}

int main() {
    ma::init();

    test_basic_alloc_free();
    test_alloc_zero();
    test_large_alloc();
    test_free_list_reuse();
    test_many_allocations();
    test_make_and_destroy();
    test_mixed_sizes();

    ma::shutdown();

    std::cout << "\nALL TESTS PASSED\n";
    return 0;
}