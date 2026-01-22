from nano_template import _bytecode

from . import _bytecode as code
from ._bytecode import Op


def disassemble(instructions: list[bytes] | bytes) -> str:
    if isinstance(instructions, list):
        return code.to_str(b"".join(instructions))
    return code.to_str(instructions)


def test_just_text() -> None:
    bytecode = _bytecode("Hello, World!")
    assert disassemble(bytecode.instructions) == disassemble([code.make(Op.TEXT, 0)])
    assert bytecode.constants == ["Hello, World!"]


def test_empty_template() -> None:
    bytecode = _bytecode("")
    assert disassemble(bytecode.instructions) == disassemble([])
    assert bytecode.constants == []


def test_just_output() -> None:
    bytecode = _bytecode("{{ a }}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.RENDER),
        ]
    )

    assert bytecode.constants == ["a"]


def test_just_output_dotted_path() -> None:
    bytecode = _bytecode("{{ a.b }}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.SELECTOR, 1),
            code.make(Op.RENDER),
        ]
    )

    assert bytecode.constants == ["a", "b"]
