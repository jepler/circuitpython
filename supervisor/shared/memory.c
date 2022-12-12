/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Scott Shawcroft for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "supervisor/memory.h"
#include "supervisor/port.h"

#include <string.h>

#include "py/gc.h"
#include "py/misc.h"
#include "supervisor/shared/display.h"

supervisor_allocation supervisor_root;

void free_memory(supervisor_allocation *allocation) {
    void *ptr = allocation->ptr;
    supervisor_allocation *next = allocation->next;

    for (supervisor_allocation *walk = &supervisor_root; walk; walk = walk->next) {
        if (walk->next == allocation) {
            walk->next = next;
            break;
        }
    }

    memset(allocation, 0, sizeof(*allocation));
    if (gc_alloc_possible()) {
        gc_free(ptr);
    }
}

supervisor_allocation *allocation_from_ptr(void *ptr) {
    for (supervisor_allocation *walk = &supervisor_root; walk; walk = walk->next) {
        if (walk->ptr == ptr) {
            return walk;
        }
    }
    return NULL;
}

void supervisor_gc_deinit(void) {
    gc_deinit();
}

static bool find_next_lowest_movable_allocation(supervisor_memory_t **low_addr, supervisor_allocation **lowest) {
    bool result = false;
    for (supervisor_allocation *walk = supervisor_root.next; walk; walk = walk->next) {
        gc_reserve(walk->ptr, walk->n_bytes);
        if (walk->ptr && walk->move && walk->ptr < *low_addr) {
            result = true;
            *low_addr = walk->ptr;
            *lowest = walk;
        }
    }
    return result;
}

void supervisor_gc_init(void) {
    assert(!gc_alloc_possible());
    gc_init(port_heap_get_bottom(), port_heap_get_top());

    // Reserve the memory for all non-movable allocations
    for (supervisor_allocation *walk = supervisor_root.next; walk; walk = walk->next) {
        if (walk->ptr && !walk->move) {
            gc_reserve(walk->ptr, walk->n_bytes);
        }
    }

    // iteratively move the lowest relocatable allocation left to the next available address
    // this way the new allocation from gc_alloc is guaranteed not to overlap any other allocation
    // EXCEPT the one being moved. (proof left as exercise)
    supervisor_memory_t *low_addr = (supervisor_memory_t *)~(uintptr_t)0;
    supervisor_allocation *lowest = NULL;
    while (find_next_lowest_movable_allocation(&low_addr, &lowest)) {
        supervisor_memory_t *ptr = lowest->ptr;
        lowest->ptr = gc_alloc(lowest->n_bytes, 0, false);
        lowest->move(lowest, ptr);
    }

}

STATIC void supervisor_ensure_gc(void) {
    if (!gc_alloc_possible()) {
        supervisor_gc_init();
    }
}

bool allocate_memory(supervisor_allocation *allocation, size_t n_bytes, supervisor_move_f move) {
    supervisor_ensure_gc();
    allocation->ptr = gc_alloc(n_bytes, 0, move == SUPERVISOR_STACK_ALLOCATION);
    if (!allocation->ptr) {
        return false;
    }
    allocation->move = move == SUPERVISOR_STACK_ALLOCATION ? move : 0;
    allocation->n_bytes = n_bytes;
    allocation->next = supervisor_root.next;
    supervisor_root.next = allocation;
    return true;
}

void allocate_memory_throw(supervisor_allocation *allocation, size_t n_bytes, supervisor_move_f move) {
    if (!allocate_memory(allocation, n_bytes, move)) {
        m_malloc_fail(n_bytes);
    }
}

void supervisor_allocator_collect_ptrs(void) {
    for (supervisor_allocation *walk = supervisor_root.next; walk; walk = walk->next) {
        gc_collect_ptr(walk->ptr);
    }
}

void supervisor_simple_move(supervisor_allocation *allocation, supervisor_memory_t *old_allocation) {
    memmove(allocation->ptr, old_allocation, allocation->n_bytes);
}

void supervisor_allocation_promote(supervisor_allocation *allocation, void *ptr, supervisor_move_f move) {
    allocation->ptr = ptr;
    allocation->n_bytes = gc_nbytes(ptr);
    allocation->next = supervisor_root.next;
    allocation->move = move;
    supervisor_root.next = allocation;
}
