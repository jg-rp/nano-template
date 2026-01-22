from enum import IntEnum
from typing import NamedTuple

from nano_template import _bytecode_definitions

# NOTE: This enum needs tp be kept in syn with `NT_Op` in instructions.h.


class Op(IntEnum):
    NULL = 0
    CONSTANT = 1
    ENTER_FRAME = 2
    FALSE = 3
    GET_LOCAL = 4
    GLOBAL = 5
    ITER_INIT = 6
    ITER_NEXT = 7
    JUMP_IF_FALSY = 8
    JUMP_IF_TRUTHY = 9
    JUMP = 10
    LEAVE_FRAME = 11
    NOT = 12
    POP = 13
    RENDER = 14
    SELECTOR = 15
    SET_LOCAL = 16
    TEXT = 17
    TRUE = 18


class OpDef(NamedTuple):
    name: str
    operand_widths: tuple[int, ...]
    operand_count: int
    total_width: int


OP_DEFS: list[OpDef] = [OpDef(*d) for d in _bytecode_definitions()]


def make(op: int, *operands: int) -> bytes:
    """Make a single bytecode instruction."""
    op_def = OP_DEFS[op]
    instruction: list[int] = [int(op)]

    if len(operands) != op_def.operand_count:
        raise ValueError(
            f"expected {op_def.operand_count} operands, got {len(operands)} for {op_def.name!r}"
        )

    for operand, byte_count in zip(
        operands, op_def.operand_widths[: op_def.operand_count]
    ):
        for byte_index in range(byte_count - 1, -1, -1):
            instruction.append((operand >> (byte_index * 8)) & 0xFF)

    return bytes(instruction)


def read(op_def: OpDef, instructions: bytes, offset: int) -> tuple[bytes, int]:
    """Read operands for instruction `op_def` from `instructions` starting at `offset`."""
    operands: list[int] = []

    for byte_count in op_def.operand_widths[: op_def.operand_count]:
        value = 0

        for i in range(0, byte_count):
            value = (value << 8) | instructions[offset + i]

        operands.append(value)
        offset += byte_count

    return bytes(operands), offset


def to_str(instructions: bytes) -> str:
    """Return a human readable string representation of `instructions`."""
    buf: list[str] = []
    offset = 0

    while offset < len(instructions):
        op_def = OP_DEFS[instructions[offset]]
        operands, new_offset = read(op_def, instructions, offset + 1)
        pretty_operands = " ".join(str(op) for op in operands)
        buf.append(f"{offset:04} {op_def.name} {pretty_operands}")
        offset = new_offset

    return "\n".join(buf)
