// SPDX-License-Identifier: MIT

#ifndef NT_COMPILER_H
#define NT_COMPILER_H

#include "nano_template/common.h"
#include "nano_template/expression.h"
#include "nano_template/instructions.h"
#include "nano_template/node.h"

typedef struct NT_Compiler
{
    // A pools of constants (just Python string objects in this case).
    PyObject *constant_pool;
    size_t constants_pool_size;
    size_t constants_pool_capacity;

    // A pool of potential keys/indexes into user data mappings/sequences.
    PyObject *selector_pool;
    size_t selectors_pool_size;
    size_t selectors_pool_capacity;

    // A pool of non-markup template text.
    PyObject *text_pool;
    size_t text_pool_size;
    size_t text_pool_capacity;

    // A stack of mappings (PyDict for now) for locally-scoped variables.
    PyObject *scope;
    size_t scope_top;
    size_t scope_capacity;

    NT_Ins *ins;
} NT_Compiler;

/// @brief The result of calling `NT_Compiler_bytecode()`.
typedef struct NT_Code
{
    PyObject *constant_pool;
    size_t constant_pool_size;

    PyObject *selector_pool;
    size_t selector_pool_size;

    PyObject *text_pool;
    size_t text_pool_size;

    uint8_t *instructions;
    size_t instructions_size;
} NT_Code;

/// @brief Allocate and initialize a new compiler.
/// @return A pointer to the new compiler, or NULL on failure with an exception
/// set.
NT_Compiler *NT_Compiler_new(void);

void NT_Compiler_free(NT_Compiler *c);

/// @brief Compile `node` and its children recursively.
/// @return 0 on success, -1 on failure with an exception set.
int NT_Compiler_compile(NT_Compiler *c, NT_Node *node);

/// @brief Compile `expr` and its children recursively.
/// @return 0 on success, -1 on failure with an exception set.
int NT_Compiler_compile_expression(NT_Compiler *c, NT_Expr *expr);

/// @brief Move instructions and various pools out of the compiler.
/// @return A pointer to a new NT_Code that now owns compiled instructions and
/// pools.
NT_Code *NT_Compiler_bytecode(NT_Compiler *c);

void NT_Code_free(NT_Code *code);

#endif
