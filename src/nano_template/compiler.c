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

// TODO: We need selector token info for detailed error messages.

/// @brief Append `selector` to the selector pool and set `out_index` to its
/// index in the pool.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Compiler_add_selector(NT_Compiler *c, PyObject *selector,
                                    size_t *out_index);

/// @brief Append `text` to the text pool and set `out_index` to its
/// index in the pool.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Compiler_add_text(NT_Compiler *c, PyObject *text,
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
static int NT_Compiler_resolve(NT_Compiler *c, PyObject *name,
                               uint8_t *out_depth, uint8_t *out_offset);