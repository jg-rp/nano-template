#include "micro_liquid/allocator.h"

/// @brief Allocate and initialize a new block of memory.
static ML_MemBlock *ML_Mem_new_block(ML_MemBlock *prev, Py_ssize_t capacity);

/// @brief Align `ptr` to the next integer divisible by `align`.
static uintptr_t align_forward(uintptr_t ptr, Py_ssize_t align);

/// @brief Reallocate objs.
static int ML_Mem_grow_refs(ML_Mem *self);

ML_Mem *ML_Mem_new(void)
{
    ML_Mem *mem = PyMem_Malloc(sizeof(ML_Mem));
    if (!mem)
    {
        return NULL;
    }

    if (ML_Mem_init(mem) < 0)
    {
        PyMem_Free(mem);
        return NULL;
    }

    return mem;
}

int ML_Mem_init(ML_Mem *self)
{
    ML_MemBlock *block = ML_Mem_new_block(NULL, DEFAULT_BLOCK_SIZE);
    if (!block)
    {
        return -1;
    }

    self->head = block;
    self->objs = NULL;
    self->obj_count = 0;
    self->obj_capacity = 0;
    return 0;
}

void *ML_Mem_alloc(ML_Mem *self, Py_ssize_t size)
{
    uintptr_t current_ptr = self->head->data + self->head->used;
    uintptr_t aligned_ptr = align_forward(current_ptr, sizeof(void *));
    Py_ssize_t required = size + aligned_ptr - current_ptr;

    if (self->head->used + required > self->head->capacity)
    {
        Py_ssize_t new_cap =
            (size > DEFAULT_BLOCK_SIZE) ? size : DEFAULT_BLOCK_SIZE;

        ML_MemBlock *new_block = ML_Mem_new_block(self->head, new_cap);
        if (!new_block)
        {
            return NULL;
        }

        self->head = new_block;
        current_ptr = self->head->data;
        aligned_ptr = align_forward(current_ptr, sizeof(void *));
    }

    Py_ssize_t padding = aligned_ptr - (self->head->data + self->head->used);
    self->head->used += size + padding;
    return (void *)aligned_ptr;
}

int ML_Mem_ref(ML_Mem *self, PyObject *obj)
{
    if (self->obj_count >= self->obj_capacity)
    {
        if (ML_Mem_grow_refs(self) < 0)
        {
            return -1;
        }
    }

    Py_INCREF(obj);
    self->objs[self->obj_count++] = obj;
    return 0;
}

int ML_Mem_steal_ref(ML_Mem *self, PyObject *obj)
{
    if (self->obj_count >= self->obj_capacity)
    {
        if (ML_Mem_grow_refs(self) < 0)
        {
            return -1;
        }
    }

    self->objs[self->obj_count++] = obj;
    return 0;
}

void ML_Mem_free(ML_Mem *self)
{
    if (!self)
    {
        return;
    }

    if (self->objs)
    {
        for (Py_ssize_t i = 0; i < self->obj_count; i++)
        {
            Py_XDECREF(self->objs[i]);
        }

        PyMem_Free(self->objs);
        self->objs = NULL;
        self->obj_count = 0;
        self->obj_capacity = 0;
    }

    ML_MemBlock *block = self->head;
    while (block)
    {
        ML_MemBlock *prev = block->prev;
        PyMem_Free(block);
        block = prev;
    }

    self->head = NULL;
    PyMem_Free(self);
}

static int ML_Mem_grow_refs(ML_Mem *self)
{
    Py_ssize_t new_cap = (self->obj_capacity < 8) ? 8 : self->obj_capacity * 2;
    PyObject **new_objs = NULL;

    if (!self->objs)
    {
        new_objs = PyMem_Malloc(sizeof(PyObject *) * new_cap);
    }
    else
    {
        new_objs = PyMem_Realloc(self->objs, sizeof(PyObject *) * new_cap);
    }

    if (!new_objs)
    {
        PyErr_NoMemory();
        return -1;
    }

    self->objs = new_objs;
    self->obj_capacity = new_cap;
    return 0;
}

static ML_MemBlock *ML_Mem_new_block(ML_MemBlock *prev, Py_ssize_t capacity)
{
    void *mem = PyMem_Malloc(sizeof(ML_MemBlock) + capacity);
    if (!mem)
    {
        PyErr_NoMemory();
        return NULL;
    }

    ML_MemBlock *block = (ML_MemBlock *)mem;
    block->prev = prev;
    block->capacity = capacity;
    block->used = 0;
    block->data = (uintptr_t)block + sizeof(ML_MemBlock);
    return block;
}

uintptr_t align_forward(uintptr_t ptr, Py_ssize_t align)
{
    Py_ssize_t modulo = ptr & (align - 1);

    if (modulo != 0)
    {
        ptr += align - modulo;
    }

    return ptr;
}