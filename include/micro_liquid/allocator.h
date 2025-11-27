#ifndef ML_ALLOCATOR_H
#define ML_ALLOCATOR_H

#include "micro_liquid/common.h"

#define DEFAULT_BLOCK_SIZE (1024 * 4)

/// @brief Allocator block header.
typedef struct ML_MemBlock
{
    struct ML_MemBlock *prev;
    Py_ssize_t capacity;
    Py_ssize_t used;
    uintptr_t data;
} ML_MemBlock;

/// @brief Arena allocator with PyObject reference management.
typedef struct ML_Mem
{
    ML_MemBlock *head;
    PyObject **objs;
    Py_ssize_t obj_count;
    Py_ssize_t obj_capacity;
} ML_Mem;

/// @brief Allocate and initialize a new allocator with a single block.
/// @return A pointer to the new allocator, or NULL on failure with an
/// exception set.
ML_Mem *ML_Mem_new(void);

/// @brief Initialize allocator with a single empty block.
/// @return 0 on success, -1 on failure.
int ML_Mem_init(ML_Mem *self);

/// @brief Allocate `size` bytes.
/// @return A pointer to the start of the allocated bytes, or NULL on failure.
void *ML_Mem_alloc(ML_Mem *self, Py_ssize_t size);

/// @brief Register `obj` as a new reference.
/// Increments `obj`s reference count.
/// @return 0 on success, -1 on failure.
int ML_Mem_ref(ML_Mem *self, PyObject *obj);

/// @brief Steal a reference to `obj`.
/// Does not increment `obj`s reference count.
/// @return 0 on success, -1 on failure.
int ML_Mem_steal_ref(ML_Mem *self, PyObject *obj);

/// @brief Free all allocated blocks and decrement any owned PyObjects.
void ML_Mem_free(ML_Mem *self);

#endif
