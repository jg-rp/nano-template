// SPDX-License-Identifier: MIT

#include "nano_template/allocator.h"

/// @brief Allocate and initialize a new block of memory.
static NT_MemBlock *NT_Mem_new_block(NT_MemBlock *prev, size_t capacity);

/// @brief Align `ptr` to the next integer divisible by `align`.
static uintptr_t align_forward(uintptr_t ptr, size_t align);

/// @brief Reallocate objs.
static int NT_Mem_grow_refs(NT_Mem *mem);

NT_Mem *NT_Mem_new(void)
{
    NT_Mem *mem = PyMem_Malloc(sizeof(NT_Mem));
    if (!mem)
    {
        PyErr_NoMemory();
        return NULL;
    }

    if (NT_Mem_init(mem) < 0)
    {
        PyMem_Free(mem);
        return NULL;
    }

    return mem;
}

int NT_Mem_init(NT_Mem *mem)
{
    NT_MemBlock *block = NT_Mem_new_block(NULL, DEFAULT_BLOCK_SIZE);
    if (!block)
    {
        return -1;
    }

    mem->head = block;
    mem->objs = NULL;
    mem->obj_count = 0;
    mem->obj_capacity = 0;
    return 0;
}

void *NT_Mem_alloc(NT_Mem *mem, size_t size)
{
    uintptr_t current_ptr = mem->head->data + mem->head->used;
    uintptr_t aligned_ptr = align_forward(current_ptr, sizeof(void *));
    size_t required = size + aligned_ptr - current_ptr;

    if (mem->head->used + required > mem->head->capacity)
    {
        size_t new_cap =
            (size > DEFAULT_BLOCK_SIZE) ? size : DEFAULT_BLOCK_SIZE;

        NT_MemBlock *new_block = NT_Mem_new_block(mem->head, new_cap);
        if (!new_block)
        {
            return NULL;
        }

        mem->head = new_block;
        current_ptr = mem->head->data;
        aligned_ptr = align_forward(current_ptr, sizeof(void *));
    }

    uintptr_t padding = aligned_ptr - (mem->head->data + mem->head->used);
    mem->head->used += size + padding;
    // NOLINTNEXTLINE(performance-no-int-to-ptr)
    return (void *)aligned_ptr;
}

int NT_Mem_ref(NT_Mem *mem, PyObject *obj)
{
    if (mem->obj_count >= mem->obj_capacity)
    {
        if (NT_Mem_grow_refs(mem) < 0)
        {
            return -1;
        }
    }

    Py_INCREF(obj);
    mem->objs[mem->obj_count++] = obj;
    return 0;
}

int NT_Mem_steal_ref(NT_Mem *mem, PyObject *obj)
{
    if (mem->obj_count >= mem->obj_capacity)
    {
        if (NT_Mem_grow_refs(mem) < 0)
        {
            return -1;
        }
    }

    mem->objs[mem->obj_count++] = obj;
    return 0;
}

void NT_Mem_free(NT_Mem *mem)
{
    if (!mem)
    {
        return;
    }

    if (mem->objs)
    {
        for (size_t i = 0; i < mem->obj_count; i++)
        {
            Py_XDECREF(mem->objs[i]);
        }

        PyMem_Free(mem->objs);
        mem->objs = NULL;
        mem->obj_count = 0;
        mem->obj_capacity = 0;
    }

    NT_MemBlock *block = mem->head;
    while (block)
    {
        NT_MemBlock *prev = block->prev;
        PyMem_Free(block);
        block = prev;
    }

    mem->head = NULL;
    PyMem_Free(mem);
}

static int NT_Mem_grow_refs(NT_Mem *mem)
{
    // NOLINTNEXTLINE(readability-magic-numbers)
    size_t new_cap = (mem->obj_capacity < 8) ? 8 : mem->obj_capacity * 2;
    PyObject **new_objs = NULL;

    if (!mem->objs)
    {
        new_objs = PyMem_Malloc(sizeof(PyObject *) * new_cap);
    }
    else
    {
        new_objs = PyMem_Realloc(mem->objs, sizeof(PyObject *) * new_cap);
    }

    if (!new_objs)
    {
        PyErr_NoMemory();
        return -1;
    }

    mem->objs = new_objs;
    mem->obj_capacity = new_cap;
    return 0;
}

static NT_MemBlock *NT_Mem_new_block(NT_MemBlock *prev, size_t capacity)
{
    void *mem = PyMem_Malloc(sizeof(NT_MemBlock) + capacity);
    if (!mem)
    {
        PyErr_NoMemory();
        return NULL;
    }

    NT_MemBlock *block = (NT_MemBlock *)mem;
    block->prev = prev;
    block->capacity = capacity;
    block->used = 0;
    block->data = (uintptr_t)block + sizeof(NT_MemBlock);
    return block;
}

uintptr_t align_forward(uintptr_t ptr, size_t align)
{
    size_t modulo = ptr & (align - 1);

    if (modulo != 0)
    {
        ptr += align - modulo;
    }

    return ptr;
}