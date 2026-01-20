// SPDX-License-Identifier: MIT

#ifndef NT_BYTECODE_H
#define NT_BYTECODE_H

#include "nano_template/common.h"

#define MAX_OPERANDS 2

/// @brief Opcodes for our virtual machine.
typedef enum
{
    NT_OP_NULL = 0,
    NT_OP_CONSTANT,
    NT_OP_ENTER_FRAME,
    NT_OP_FALSE,
    NT_OP_GET_LOCAL,
    NT_OP_GLOBAL,
    NT_OP_ITER_INIT,
    NT_OP_ITER_NEXT,
    NT_OP_JUMP_IF_FALSY,
    NT_OP_JUMP_IF_TRUTHY,
    NT_OP_JUMP,
    NT_OP_LEAVE_FRAME,
    NT_OP_NOT,
    NT_OP_POP,
    NT_OP_RENDER,
    NT_OP_SELECTOR,
    NT_OP_SET_LOCAL,
    NT_OP_TEXT,
    NT_OP_TRUE,
} NT_Op;

/// @brief Operation definition.
typedef struct NT_OpDef
{
    const char *name;
    uint8_t operand_widths[MAX_OPERANDS];
    uint8_t operand_count;
    uint8_t width;
} NT_OpDef;

/// @brief Bytecode instructions.
typedef struct NT_Ins
{
    size_t size;
    size_t capacity;
    uint8_t *bytes;
} NT_Ins;

/// @brief Allocate and initialize a new growable array of instructions.
/// @return A pointer to the new instructions, or NULL on error with an
/// exception set.
NT_Ins *NT_Ins_new();
void NT_Ins_free(NT_Ins *ins);

/// @brief Append a new zero-operand instruction to `ins`.
/// @param ins Instructions
/// @return 0 on success, -1 on failure.
int NT_Ins_pack(NT_Ins *ins, NT_Op op);

/// @brief Append a new single-operand instruction to `ins`.
/// @param ins Instructions
/// @param operand  Operand for the instruction.
/// @return 0 on success, -1 on failure.
int NT_Ins_pack1(NT_Ins *ins, NT_Op op, int operand);

/// @brief Write a single-operand instruction to `ins` at position `pos`,
/// replacing bytes for the instruction at pos.
/// @param ins Instructions.
/// @param op Instruction op code
/// @param operand Operand for the instruction.
/// @param pos Index into `ins`.
/// @return 0 on success, -1 on failure.
int NT_Ins_replace(NT_Ins *ins, NT_Op op, int operand, size_t pos);

/// @brief Append a new two-operand instruction to `ins`.
/// @param ins Instructions.
/// @param op An op code.
/// @param op1 First operand for the instruction.
/// @param op2 Second operand for the instruction.
/// @return 0 on success, -1 on failure.
int NT_Ins_pack2(NT_Ins *ins, NT_Op op, int op1, int op2);

/// @brief Read a single operand from `ins`.
/// @param ins Instructions.
/// @param n Number of bytes for the operand.
/// @param offset Index into `ins` from which to start reading.
/// @return An operand.
int NT_Ins_read_bytes(NT_Ins *ins, uint8_t n, size_t offset);
#endif
