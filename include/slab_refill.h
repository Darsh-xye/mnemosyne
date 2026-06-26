#pragma once

#include "slab.h"

FreeSlot* slab_refill_locked(int size_class, int count);