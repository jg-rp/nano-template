// SPDX-License-Identifier: MIT

#ifndef NT_ALLOCATOR_H
#define NT_ALLOCATOR_H

#include "nano_template/common.h"

#define DEFAULT_BLOCK_SIZE (size_t)(1024 * 4)

/// @brief Allocator block header.
typedef struct NT_MemBlock
{
    struct NT_MemBlock *prev;
    size_t capacity;
    size_t used;
    uintptr_t data;
} NT_MemBlock;

/// @brief Arena allocator with PyObject reference ownership.
typedef struct NT_Mem
{
    NT_MemBlock *head;
    PyObject **objs;
    size_t obj_count;
    size_t obj_capacity;
} NT_Mem;

/// @brief Allocate and initialize a new allocator with a single block.
/// @return A pointer to the new allocator, or NULL on failure with an
/// exception set.
NT_Mem *NT_Mem_new(void);

/// @brief Initialize allocator with a single empty block.
/// @return 0 on success, -1 on failure.
int NT_Mem_init(NT_Mem *mem);

/// @brief Allocate `size` bytes.
/// @return A pointer to the start of the allocated bytes, or NULL on failure
/// with an exception set.
void *NT_Mem_alloc(NT_Mem *mem, size_t size);

/// @brief Register `obj` as a new reference.
/// Increments `obj`s reference count.
/// @return 0 on success, -1 on failure.
int NT_Mem_ref(NT_Mem *mem, PyObject *obj);

/// @brief Steal a reference to `obj`.
/// Does not increment `obj`s reference count.
/// @return 0 on success, -1 on failure.
int NT_Mem_steal_ref(NT_Mem *mem, PyObject *obj);

/// @brief Free all allocated blocks and decrement any owned PyObjects.
void NT_Mem_free(NT_Mem *mem);

#endif
