// SPDX-License-Identifier: MIT

#include "nano_template/compiler.h"

/// @brief Enter a new local/block scope.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Compiler_enter_scope(NT_Compiler *c);

/// Leave the current local/block scope.
static void NT_Compiler_leave_scope(NT_Compiler *c);

/// @brief Append `const` to the constant pool and set `out_index` to its index
/// in the pool.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Compiler_add_constant(NT_Compiler *c, PyObject *const,
                                    size_t *out_index);

/// Emit an instruction with no operands.
static int NT_Compiler_emit(NT_Compiler *c, NT_Op op, size_t *out_pos);

/// Emit an instruction with one operand.
static int NT_Compiler_emit1(NT_Compiler *c, NT_Op op, int operand,
                             size_t *out_pos);

/// Emit an instruction with two operands.
static int NT_Compiler_emit2(NT_Compiler *c, NT_Op op, int op1, int op2,
                             size_t *out_pos);

/// @brief Change the instruction at `pos` to use `new_operand`.
/// Only works for instructions with exactly one operand.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Compiler_change_operand(NT_Compiler *c, size_t pos,
                                      int new_operand);

/// @brief Add `name` to the symbol table for the current scope.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Compiler_define(NT_Compiler *c, PyObject *name, int *out_offset);

/// @brief Resolve `name` in the current scope.
/// If found, `out_depth` and `out_offset` will be set.
/// @return 1 if `name` was found, 0 otherwise.
static int NT_Compiler_resolve(NT_Compiler *c, PyObject *name, int *out_depth,
                               int *out_offset);

NT_Compiler *NT_Compiler_new(void)
{
    NT_Compiler *c = NULL;
    NT_Ins *ins = NULL;
    PyObject *scope = NULL;
    size_t initial_scope_capacity = 8;

    NT_Compiler *c = PyMem_Malloc(sizeof(NT_Compiler));
    if (!c)
    {
        PyErr_NoMemory();
        goto fail;
    }

    ins = NT_Ins_new();
    if (!ins)
    {
        goto fail;
    }

    scope = PyMem_Malloc(sizeof(PyObject *) * initial_scope_capacity);
    if (!scope)
    {
        goto fail;
    }

    c->constant_pool = NULL;
    c->constant_pool_size = 0;
    c->constant_pool_capacity = 0;

    c->scope = scope;
    c->scope_top = 0;
    c->scope_capacity = initial_scope_capacity;

    c->ins = ins;
    return c;

fail:
    PyMem_Free(ins);
    PyMem_Free(scope);
    PyMem_Free(c);
    return NULL;
}

void NT_Compiler_free(NT_Compiler *c)
{
    for (int i = 0; i < c->constant_pool_size; i++)
    {
        Py_XDECREF(c->constant_pool[i]);
        c->constant_pool[i] = NULL;
    }

    PyMem_Free(c->constant_pool);
    c->constant_pool = NULL;
    c->constant_pool_size = 0;
    c->constant_pool_capacity = 0;

    for (int i = 0; i < c->scope_top; i++)
    {
        Py_XDECREF(c->scope[i]);
        c->scope[i] = NULL;
    }

    PyMem_Free(c->scope);
    c->scope = NULL;
    c->scope_top = 0;
    c->scope_capacity = 0;

    NT_Ins_free(c->ins);
    PyMem_Free(c->ins);
    c->ins = NULL;

    PyMem_Free(c);
}

NT_Code *NT_Compiler_bytecode(NT_Compiler *c)
{
    NT_Code *code = PyMem_Malloc(sizeof(NT_Code));
    if (!code)
    {
        PyErr_NoMemory();
        return NULL;
    }

    // Move constants out of compiler.
    // TODO: Shrink capacity to size?
    code->constant_pool = c->constant_pool;
    code->constant_pool_size = c->constant_pool_size;
    c->constant_pool = NULL;
    c->constant_pool_size = 0;
    c->constant_pool_capacity = 0;

    // Move instructions out of c->ins.
    // TODO: Shrink capacity to size?
    code->instructions = c->ins->bytes;
    code->instructions_size = c->ins->size;
    c->ins->bytes = NULL;
    c->ins->size = 0;
    c->ins->capacity = 0;
    PyMem_free(c->ins);
    c->ins = NULL;

    return code;
}

void NT_Code_free(NT_Code *code)
{
    for (int i = 0; i < code->constant_pool_size; i++)
    {
        Py_XDECREF(code->constant_pool[i]);
        code->constant_pool[i] = NULL;
    }

    PyMem_Free(code->constant_pool);
    code->constant_pool = NULL;
    code->constant_pool_size = 0;

    PyMem_free(code->instructions);
    code->instructions = NULL;
    code->instructions_size = 0;

    PyMem_free(code);
}
