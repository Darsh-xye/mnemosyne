#include "central_pool.h"

using namespace std;

PaddedAtomicTaggedPtr g_central_pool[NUM_SIZE_CLASSES];

// Push one node
void central_push(int size_class, FreeSlot* node) {
    TaggedPtr old_head = g_central_pool[size_class].value.load(memory_order_relaxed);
    TaggedPtr new_head;

    do {
        node->next = old_head.ptr;
        new_head = {node, old_head.tag + 1};
    } while (!g_central_pool[size_class].value.compare_exchange_weak(
        old_head, new_head,
        memory_order_release,
        memory_order_relaxed));
}

// Pop one node
FreeSlot* central_pop(int size_class) {
    TaggedPtr old_head = g_central_pool[size_class].value.load(memory_order_acquire);
    TaggedPtr new_head;

    do {
        if (!old_head.ptr) return nullptr;
        new_head = {old_head.ptr->next, old_head.tag + 1};
    } while (!g_central_pool[size_class].value.compare_exchange_weak(
        old_head, new_head,
        memory_order_acquire,
        memory_order_relaxed));

    return old_head.ptr;
}

// Push batch
void central_push_batch(int size_class, FreeSlot* head, int count) {
    FreeSlot* tail = head;
    for (int i = 0; i < count - 1; i++)
        tail = tail->next;

    TaggedPtr old_head = g_central_pool[size_class].value.load(memory_order_relaxed);
    TaggedPtr new_head;

    do {
        tail->next = old_head.ptr;
        new_head = {head, old_head.tag + 1};
    } while (!g_central_pool[size_class].value.compare_exchange_weak(
        old_head, new_head,
        memory_order_release,
        memory_order_relaxed));
}

// Pop batch
FreeSlot* central_pop_batch(int size_class, int count) {
    FreeSlot* head = nullptr;
    FreeSlot* tail = nullptr;

    for (int i = 0; i < count; i++) {
        FreeSlot* node = central_pop(size_class);
        if (!node) break;

        if (!head)
            head = tail = node;
        else {
            tail->next = node;
            tail = node;
        }
    }

    if (tail) tail->next = nullptr;
    return head;
}