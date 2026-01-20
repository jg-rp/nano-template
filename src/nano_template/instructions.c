// SPDX-License-Identifier: MIT

#include "nano_template/instructions.h"

/// @brief Append a single byte to bytecode instructions `ins`.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Code_write_byte(NT_Ins *ins, uint8_t byte);

/// @brief Write `operand` to `ins` as `byte_count` bytes with the most
/// significant byte first.
/// @return 0 on success, -1 on failure with an exception set.
static int NT_Code_write_operand(NT_Ins *ins, int operand, uint8_t byte_count);

/// @brief A table of opcode constants and their definitions.
static NT_OpDef defs[] = {
    [NT_OP_NULL] = {"OpNull", {0, 0}, 0, 1},
    [NT_OP_CONSTANT] = {"OpConstant", {2, 0}, 1, 3},
    [NT_OP_ENTER_FRAME] = {"OpEnterFrame", {1, 0}, 1, 2},
    [NT_OP_FALSE] = {"OpFalse", {0, 0}, 0, 1},
    [NT_OP_GET_LOCAL] = {"OpGetLocal", {1, 1}, 2, 3},
    [NT_OP_GLOBAL] = {"OpGlobal", {2, 0}, 1, 3},
    [NT_OP_ITER_INIT] = {"OpIterInit", {0, 0}, 0, 1},
    [NT_OP_ITER_NEXT] = {"OpIterNext", {0, 0}, 0, 1},
    [NT_OP_JUMP_IF_FALSY] = {"OpJumpIfFalsy", {2, 0}, 1, 3},
    [NT_OP_JUMP_IF_TRUTHY] = {"OpJumpIfTruthy", {2, 0}, 1, 3},
    [NT_OP_JUMP] = {"OpJump", {0, 0}, 0, 1},
    [NT_OP_LEAVE_FRAME] = {"OpLeaveFrame", {0, 0}, 0, 1},
    [NT_OP_NOT] = {"OpNot", {0, 0}, 0, 1},
    [NT_OP_POP] = {"OpPop", {0, 0}, 0, 1},
    [NT_OP_RENDER] = {"OpRender", {0, 0}, 0, 1},
    [NT_OP_SELECTOR] = {"OpSelector", {2, 0}, 1, 3},
    [NT_OP_SET_LOCAL] = {"OpSetLocal", {1, 0}, 1, 2},
    [NT_OP_TEXT] = {"OpText", {2, 0}, 1, 3},
    [NT_OP_TRUE] = {"OpTrue", {0, 0}, 0, 1},
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
    if (!ins)
    {
        return;
    }

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

    assert(op_def.operand_widths[0] != 0);
    assert(op_def.operand_count == 1);
    uint8_t byte_count = op_def.operand_widths[0];

    // TODO: defensively check `operand` can fit into `byte_count` bytes?
    return NT_Code_write_operand(ins, operand, byte_count);
}

int NT_Ins_replace(NT_Ins *ins, NT_Op op, int operand, size_t pos)
{
    assert(ins->bytes[pos] == op);
    NT_OpDef op_def = defs[op];

    assert(op_def.operand_widths[0] != 0);
    assert(op_def.operand_count == 1);
    uint8_t byte_count = op_def.operand_widths[0];
    uint8_t byte = 0;

    for (int i = 1, byte_index = byte_count - 1; byte_index >= 0;
         i++, byte_index--)
    {
        byte = (operand >> (byte_index * 8)) & 0xFF;
        ins->bytes[pos + i] = byte;
    }

    return 0;
}

int NT_Ins_pack2(NT_Ins *ins, NT_Op op, int op1, int op2)
{
    assert(op >= NT_OP_NULL && op <= NT_OP_TRUE);
    NT_OpDef op_def = defs[op];

    assert(op_def.operand_count == 2);

    if (NT_Code_write_byte(ins, op) < 0)
    {
        return -1;
    }

    assert(op_def.operand_widths[0] != 0);
    if (NT_Code_write_operand(ins, op1, op_def.operand_widths[0]) < 0)
    {
        return -1;
    }

    assert(op_def.operand_widths[1] != 0);
    return NT_Code_write_operand(ins, op2, op_def.operand_widths[1]);
}

static int NT_Code_write_operand(NT_Ins *ins, int operand, uint8_t byte_count)
{
    uint8_t byte = 0;

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
    return 0;
}