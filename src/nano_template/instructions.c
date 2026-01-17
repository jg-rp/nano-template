// SPDX-License-Identifier: MIT

#include "nano_template/instructions.h"

/// @brief Write a single byte to bytecode instructions `ins`.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Code_write_byte(NT_Ins *ins, uint8_t byte);

/// @brief Write `operand` to `ins` as `byte_count` bytes with the most
/// significant byte first.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Code_write_op(NT_Ins *ins, int operand, uint8_t byte_count);

/// @brief A mapping of opcode constants to their human readable names, total
/// width in bytes and a null-terminated array of byte widths for each operand.
///
/// We use opcodes as indexes into this table. Enure entries are ordered to
/// match the `NT_OP` enum. By convention, `NT_OP_NULL` is zero, then op codes
/// are sorted alphabetically in ascending order.
static NT_OpDef defs[] = {
    [NT_OP_NULL] = {"OpNull", 1, {NULL}},
    [NT_OP_CONSTANT] = {"OpConstant", 3, {2, NULL}},
    [NT_OP_ENTER_FRAME] = {"OpEnterFrame", 2, {1, NULL}},
    [NT_OP_FALSE] = {"OpFalse", 1, {NULL}},
    [NT_OP_GET_LOCAL] = {"OpGetLocal", 3, {1, 1, NULL}},
    [NT_OP_GLOBAL] = {"OpGlobal", 3, {2, NULL}},
    [NT_OP_ITER_INIT] = {"OpIterInit", 1, {NULL}},
    [NT_OP_ITER_NEXT] = {"OpIterNext", 1, {NULL}},
    [NT_OP_JUMP_IF_FALSY] = {"OpJumpIfFalsy", 3, {2, NULL}},
    [NT_OP_JUMP_IF_TRUTHY] = {"OpJumpIfTruthy", 3, {2, NULL}},
    [NT_OP_JUMP] = {"OpJump", 1, {NULL}},
    [NT_OP_LEAVE_FRAME] = {"OpLeaveFrame", 1, {NULL}},
    [NT_OP_NOT] = {"OpNot", 1, {NULL}},
    [NT_OP_POP] = {"OpPop", 1, {NULL}},
    [NT_OP_RENDER] = {"OpRender", 1, {NULL}},
    [NT_OP_SELECTOR] = {"OpSelector", 3, {2, NULL}},
    [NT_OP_SET_LOCAL] = {"OpSetLocal", 2, {1, NULL}},
    [NT_OP_TEXT] = {"OpText", 3, {2, NULL}},
    [NT_OP_TRUE] = {"OpTrue", 1, {NULL}},
};

NT_Ins *NT_Ins_new()
{
    NT_Ins *code = PyMem_Malloc(sizeof(NT_Ins));
    if (!code)
    {
        PyErr_NoMemory();
        return NULL;
    }

    code->size = 0;
    code->capacity = 0;
    code->bytes = NULL;

    return code;
}

void NT_Ins_free(NT_Ins *ins)
{
    PyMem_Free(ins->bytes);
    ins->bytes = NULL;
    ins->size = 0;
    ins->capacity = 0;
    PyMem_Free(ins);
}

int NT_Ins_read_bytes(NT_Ins *ins, uint8_t n, size_t offset)
{
    assert(offset + n - 1 < ins->size);

    int value = 0;
    for (int i = 0; i < n; i++)
    {
        value = (value << 8) | ins->bytes[offset + i];
    }
    return value;
}

int NT_Ins_pack(NT_Ins *ins, NT_Op op)
{
    assert(op >= NT_OP_NULL && op <= NT_OP_TRUE);
    return NT_Code_write_byte(ins, op);
}

int NT_Ins_pack1(NT_Ins *ins, NT_Op op, int operand)
{
    assert(op >= NT_OP_NULL && op <= NT_OP_TRUE);
    NT_OpDef op_def = defs[op];

    if (NT_Code_write_byte(ins, op) < 0)
    {
        return -1;
    }

    assert(op_def.operand_widths[0] != NULL);
    uint8_t byte_count = op_def.operand_widths[0];

    // TODO: defensively check `operand` can fit into `byte_count` bytes?
    return NT_Code_write_op(ins, operand, byte_count);
}

int NT_Ins_pack2(NT_Ins *ins, NT_Op op, int op1, int op2)
{
    assert(op >= NT_OP_NULL && op <= NT_OP_TRUE);
    NT_OpDef op_def = defs[op];

    if (NT_Code_write_byte(ins, op) < 0)
    {
        return -1;
    }

    assert(op_def.operand_widths[0] != NULL);
    if (NT_Code_write_op(ins, op1, op_def.operand_widths[0]) < 0)
    {
        return -1;
    }

    assert(op_def.operand_widths[1] != NULL);
    return NT_Code_write_op(ins, op2, op_def.operand_widths[1]);
}

static int NT_Code_write_op(NT_Ins *ins, int operand, uint8_t byte_count)
{
    uint8_t byte = NULL;

    for (int byte_index = byte_count - 1; byte_index >= 0; byte_index--)
    {
        byte = (operand >> (byte_index * 8)) & 0xFF;
        if (NT_Code_write_byte(ins, byte) < 0)
        {
            return -1;
        }
    }

    return 0;
}

static int NT_Code_write_byte(NT_Ins *ins, uint8_t byte)
{
    if (ins->size >= ins->capacity)
    {
        Py_ssize_t new_cap = ins->capacity == 0 ? 64 : ins->capacity * 2;
        uint8_t *new_bytes =
            PyMem_Realloc(ins->bytes, sizeof(uint8_t) * new_cap);

        if (!new_bytes)
        {
            PyErr_NoMemory();
            return -1;
        }

        ins->bytes = new_bytes;
        ins->capacity = new_cap;
    }

    ins->bytes[ins->size++] = byte;
}