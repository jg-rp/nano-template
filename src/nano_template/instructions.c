// SPDX-License-Identifier: MIT

#include "nano_template/instructions.h"

/// @brief Append a single byte to bytecode instructions `ins`.
/// @return 0 on success, -1 on failure with an exception set.
static int code_write_byte(NT_Ins *ins, uint8_t byte);

/// @brief Write `operand` to `ins` as `byte_count` bytes with the most
/// significant byte first.
/// @return 0 on success, -1 on failure with an exception set.
static int code_write_operand(NT_Ins *ins, int operand, uint8_t byte_count);

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
    return code_write_byte(ins, op);
}

int NT_Ins_pack1(NT_Ins *ins, NT_Op op, int operand)
{
    assert(op >= NT_OP_NULL && op <= NT_OP_TRUE);
    NT_OpDef op_def = NT_Ins_defs[op];

    if (code_write_byte(ins, op) < 0)
    {
        return -1;
    }

    assert(op_def.operand_widths[0] != 0);
    assert(op_def.operand_count == 1);
    uint8_t byte_count = op_def.operand_widths[0];

    // TODO: defensively check `operand` can fit into `byte_count` bytes?
    return code_write_operand(ins, operand, byte_count);
}

int NT_Ins_replace(NT_Ins *ins, NT_Op op, int operand, size_t pos)
{
    assert(ins->bytes[pos] == op);
    NT_OpDef op_def = NT_Ins_defs[op];

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
    NT_OpDef op_def = NT_Ins_defs[op];

    assert(op_def.operand_count == 2);

    if (code_write_byte(ins, op) < 0)
    {
        return -1;
    }

    assert(op_def.operand_widths[0] != 0);
    if (code_write_operand(ins, op1, op_def.operand_widths[0]) < 0)
    {
        return -1;
    }

    assert(op_def.operand_widths[1] != 0);
    return code_write_operand(ins, op2, op_def.operand_widths[1]);
}

static int code_write_operand(NT_Ins *ins, int operand, uint8_t byte_count)
{
    uint8_t byte = 0;

    for (int byte_index = byte_count - 1; byte_index >= 0; byte_index--)
    {
        byte = (operand >> (byte_index * 8)) & 0xFF;
        if (code_write_byte(ins, byte) < 0)
        {
            return -1;
        }
    }

    return 0;
}

static int code_write_byte(NT_Ins *ins, uint8_t byte)
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