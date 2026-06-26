#include "slab_refill.h"
#include <mutex>

static std::mutex g_slab_mutex;

// Shared slab chains used when caches need fresh slots.
static SlabHeader* g_slab_chains[NUM_SIZE_CLASSES] = {
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
};

FreeSlot* slab_refill_locked(int size_class, int count) {
    std::lock_guard<std::mutex> lock(g_slab_mutex);

    size_t obj_size = SIZE_CLASSES[size_class];

    // Create the slab chain for this size class if needed.
    if (g_slab_chains[size_class] == nullptr) {
        g_slab_chains[size_class] = create_slab(obj_size);
        if (!g_slab_chains[size_class]) return nullptr;
    }

    FreeSlot* head = nullptr;
    FreeSlot* tail = nullptr;

    // Pull a batch of slots from the slab allocator.
    for (int i = 0; i < count; i++) {
        void* ptr = slab_alloc(g_slab_chains[size_class]);
        if (!ptr) break;

        FreeSlot* node = (FreeSlot*)ptr;
        if (!head) {
            head = node;
            tail = node;
        } else {
            tail->next = node;
            tail = node;
        }
    }

    if (tail) tail->next = nullptr;
    return head;
}