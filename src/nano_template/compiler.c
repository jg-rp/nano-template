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
static int NT_Compiler_add_constant(NT_Compiler *c, PyObject *constant,
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

int NT_Compiler_compile(NT_Compiler *c, NT_Node *node)
{
    size_t jump_not_truthy_pos = 0;
    size_t after_block_pos = 0;
    size_t *jump_positions = NULL;
    size_t pos = 0;

    switch (node->kind)
    {
    case NODE_TEXT:
        int constant_index = NULL;
        if (NT_Compiler_add_constant(c, node->str, &constant_index) < 0)
        {
            goto fail;
        }

        if (NT_Compiler_emit1(c, NT_OP_TEXT, constant_index, &pos) < 0)
        {
            goto fail;
        }
        break;
    case NODE_OUPUT:
        if (NT_Compiler_compile_expression(c, node->expr) < 0)
        {
            goto fail;
        }

        if (NT_Compiler_emit(c, NT_OP_RENDER, &pos) < 0)
        {
            goto fail;
        }
        break;
    case NODE_IF_TAG:
        jump_positions = PyMem_Malloc(sizeof(int) * node->child_count);
        if (!jump_positions)
        {
            goto fail;
        }

        int child_index = 0;
        NT_Node *child = NULL;
        NT_NodePage *page = node->head;

        while (page)
        {
            for (Py_ssize_t i = 0; i < page->count; i++)
            {
                child = page->nodes[i];

                if (child->kind == NODE_ELSE_BLOCK)
                {
                    if (NT_Compiler_compile_block(c, child) < 0)
                    {
                        goto fail;
                    }
                    goto after_else;
                }

                if (NT_Compiler_compile_expression(c, child->expr) < 0)
                {
                    goto fail;
                }

                if (NT_Compiler_emit1(c, NT_OP_JUMP_IF_FALSY, 9999,
                                      &jump_not_truthy_pos) < 0)
                {
                    goto fail;
                }

                if (NT_Compiler_emit(c, NT_OP_POP, &pos) < 0)
                {
                    goto fail;
                }

                if (NT_Compiler_compile_block(c, child) < 0)
                {
                    goto fail;
                }

                jump_positions[child_index++] = pos;

                if (NT_Compiler_change_operand(c, jump_not_truthy_pos,
                                               c->ins->size) < 0)
                {
                    goto fail;
                }

                if (NT_Compiler_emit(c, NT_OP_POP, &pos) < 0)
                {
                    goto fail;
                }
            }

            page = page->next;
        }

    after_else:

        for (int i = 0; i < node->child_count; i++)
        {
            if (NT_Compiler_change_operand(c, jump_positions[i],
                                           c->ins->size) < 0)
            {
                goto fail;
            }
        }

        PyMem_Free(jump_positions);
        jump_positions = NULL;
        break;

    case NODE_FOR_TAG:
        // TODO:
        NTPY_TODO_I();
        break;
    default:
        // TODO:
        NTPY_TODO_I();
        break;
    }

fail:
    PyMem_Free(jump_positions);
    jump_positions = NULL;
    return -1;
}

int NT_Compiler_compile_expression(NT_Compiler *c, NT_Expr *expr)
{
    size_t pos = 0;
    size_t constant_index = 0;
    int local_depth = 0;
    int local_offset = 0;

    switch (expr->kind)
    {
    case EXPR_VAR:
        if (NT_Compiler_resolve(c, expr->head->objs[0], &local_depth,
                                &local_offset))
        {
            if (NT_Compiler_emit2(c, NT_OP_GET_LOCAL, local_depth,
                                  local_offset, &pos) < 0)
            {
                goto fail;
            }
        }
        else
        {
            if (NT_Compiler_add_constant(c, expr->head->objs[0],
                                         &constant_index) < 0)
            {
                goto fail;
            }

            if (NT_Compiler_emit1(c, NT_OP_GLOBAL, constant_index, &pos) < 0)
            {
                goto fail;
            }
        }

        // TODO: OP_SELECTOR for each subsequent selector.
        break;
    case EXPR_STR:
    case EXPR_AND:
    case EXPR_OR:
    case EXPR_NOT:
        // TODO:
        NTPY_TODO_I();
    default:
        // TODO:
        NTPY_TODO_I();
        break;
    }

    return 0;

fail:
    return -1;
}

static int NT_Compiler_enter_scope(NT_Compiler *c)
{
    if (c->scope_top >= c->scope_capacity)
    {
        size_t new_cap = c->scope_capacity * 2;
        PyObject **new_scope =
            PyMem_Realloc(c->scope, sizeof(PyObject *) * new_cap);

        if (!new_scope)
        {
            PyErr_NoMemory();
            return -1;
        }

        c->scope = new_scope;
        c->scope_capacity = new_cap;
    }

    PyObject *local_scope = PyDict_New();
    if (!local_scope)
    {
        return -1;
    }

    c->scope[c->scope_top++] = local_scope;
    return 0;
}

static void NT_Compiler_leave_scope(NT_Compiler *c)
{
    assert(c->scope_top > 0);
    c->scope_top -= 1;
    Py_XDECREF(c->scope[c->scope_top]);
    c->scope[c->scope_top] = NULL;
}

static int NT_Compiler_add_constant(NT_Compiler *c, PyObject *constant,
                                    size_t *out_index)
{
    if (c->constant_pool_size >= c->constant_pool_capacity)
    {
        size_t new_cap = c->constant_pool_capacity == 0
                             ? 64
                             : c->constant_pool_capacity * 2;

        PyObject **new_constant_pool =
            PyMem_Realloc(c->constant_pool, sizeof(PyObject *) * new_cap);

        if (!new_constant_pool)
        {
            PyErr_NoMemory();
            return -1;
        }

        c->constant_pool = new_constant_pool;
        c->constant_pool_capacity = new_cap;
    }

    size_t index = c->constant_pool_size;
    c->constant_pool[c->constant_pool_size++] = constant;
    Py_INCREF(constant);
    *out_index = index;
    return 0;
}

static int NT_Compiler_emit(NT_Compiler *c, NT_Op op, size_t *out_pos)
{
    *out_pos = c->ins->size;
    return NT_Ins_pack(c->ins, op);
}

static int NT_Compiler_emit1(NT_Compiler *c, NT_Op op, int operand,
                             size_t *out_pos)
{
    *out_pos = c->ins->size;
    return NT_Ins_pack1(c->ins, op, operand);
}

static int NT_Compiler_emit2(NT_Compiler *c, NT_Op op, int op1, int op2,
                             size_t *out_pos)
{
    *out_pos = c->ins->size;
    return NT_Ins_pack2(c->ins, op, op1, op2);
}

static int NT_Compiler_change_operand(NT_Compiler *c, size_t pos,
                                      int new_operand)
{
    uint8_t byte = c->ins->bytes[pos];
    assert(byte > 0 && byte <= NT_OP_TRUE);
    return NT_Ins_replace(c->ins, byte, new_operand, pos);
}

static int NT_Compiler_define(NT_Compiler *c, PyObject *name, int *out_offset)
{
    PyObject *offset = NULL;
    PyObject *scope = c->scope[c->scope_top - 1];
    Py_ssize_t scope_offset = PyDict_Size(scope);

    offset = PyLong_FromLong(scope_offset);
    if (!offset)
    {
        goto fail;
    }

    if (PyDict_SetItem(scope, name, offset) < 0)
    {
        goto fail;
    }

    *out_offset = scope_offset;
    Py_XDECREF(offset);
    return 0;

fail:

    Py_XDECREF(offset);
    return -1;
}

static int NT_Compiler_resolve(NT_Compiler *c, PyObject *name, int *out_depth,
                               int *out_offset)
{
    PyObject *offset = NULL;

    for (int i = c->scope_top - 1, depth = 0; i >= 0; i--, depth++)
    {
        offset = PyDict_GetItem(c->scope[i], name);
        if (offset)
        {
            *out_depth = depth;
            *out_offset = PyLong_AsInt(offset);
            return 1;
        }
    }

    return 0;
}
