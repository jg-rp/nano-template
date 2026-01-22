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


def test_logical_or() -> None:
    bytecode = _bytecode("{{ a or b }}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.JUMP_IF_TRUTHY, 10),
            code.make(Op.POP),
            code.make(Op.GLOBAL, 1),
            code.make(Op.RENDER),
        ]
    )

    assert bytecode.constants == ["a", "b"]


def test_logical_and() -> None:
    bytecode = _bytecode("{{ a and b }}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.JUMP_IF_FALSY, 10),
            code.make(Op.POP),
            code.make(Op.GLOBAL, 1),
            code.make(Op.RENDER),
        ]
    )

    assert bytecode.constants == ["a", "b"]


def test_logical_not() -> None:
    bytecode = _bytecode("{{ not a }}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.NOT),
            code.make(Op.RENDER),
        ]
    )

    assert bytecode.constants == ["a"]


def test_logical_or_and() -> None:
    bytecode = _bytecode("{{ x or a and b }}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.JUMP_IF_TRUTHY, 17),
            code.make(Op.POP),
            code.make(Op.GLOBAL, 1),
            code.make(Op.JUMP_IF_FALSY, 17),
            code.make(Op.POP),
            code.make(Op.GLOBAL, 2),
            code.make(Op.RENDER),
        ]
    )

    assert bytecode.constants == ["x", "a", "b"]


def test_condition() -> None:
    bytecode = _bytecode("{% if a %}b{% endif %}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.JUMP_IF_FALSY, 13),
            code.make(Op.POP),
            code.make(Op.TEXT, 1),
            code.make(Op.JUMP, 14),
            code.make(Op.POP),
        ]
    )

    assert bytecode.constants == ["a", "b"]


def test_condition_alternative() -> None:
    bytecode = _bytecode("{% if a %}b{% else %}c{% endif %}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.JUMP_IF_FALSY, 13),
            code.make(Op.POP),
            code.make(Op.TEXT, 1),
            code.make(Op.JUMP, 17),
            code.make(Op.POP),
            code.make(Op.TEXT, 2),
        ]
    )

    assert bytecode.constants == ["a", "b", "c"]


def test_conditions() -> None:
    bytecode = _bytecode("{% if a %}b{% elif c %}d{% endif %}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.JUMP_IF_FALSY, 13),
            code.make(Op.POP),
            code.make(Op.TEXT, 1),
            code.make(Op.JUMP, 28),
            code.make(Op.POP),
            code.make(Op.GLOBAL, 2),
            code.make(Op.JUMP_IF_FALSY, 27),
            code.make(Op.POP),
            code.make(Op.TEXT, 3),
            code.make(Op.JUMP, 28),
            code.make(Op.POP),
        ]
    )

    assert bytecode.constants == ["a", "b", "c", "d"]


def test_conditions_and_alternative() -> None:
    bytecode = _bytecode("{% if a %}b{% elif c %}d{% else %}e{% endif %}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.GLOBAL, 0),
            code.make(Op.JUMP_IF_FALSY, 13),
            code.make(Op.POP),
            code.make(Op.TEXT, 1),
            code.make(Op.JUMP, 31),
            code.make(Op.POP),
            code.make(Op.GLOBAL, 2),
            code.make(Op.JUMP_IF_FALSY, 27),
            code.make(Op.POP),
            code.make(Op.TEXT, 3),
            code.make(Op.JUMP, 31),
            code.make(Op.POP),
            code.make(Op.TEXT, 4),
        ]
    )

    assert bytecode.constants == ["a", "b", "c", "d", "e"]


def test_loop() -> None:
    bytecode = _bytecode("{% for x in y %}{{ x }}{% endfor %}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.ENTER_FRAME, 1),
            code.make(Op.GLOBAL, 0),
            code.make(Op.ITER_INIT),
            code.make(Op.ITER_NEXT),
            code.make(Op.JUMP_IF_FALSY, 20),
            code.make(Op.POP),
            code.make(Op.SET_LOCAL, 0),
            code.make(Op.GET_LOCAL, 0, 0),
            code.make(Op.RENDER),
            code.make(Op.JUMP, 6),
            code.make(Op.POP),
            code.make(Op.POP),
            code.make(Op.LEAVE_FRAME),
        ]
    )

    assert bytecode.constants == ["y"]


def test_loop_nested() -> None:
    bytecode = _bytecode(
        "{% for x in y %}{% for a in x %}{{ a }}{% endfor %}{% endfor %}"
    )

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.ENTER_FRAME, 1),
            code.make(Op.GLOBAL, 0),
            code.make(Op.ITER_INIT),
            code.make(Op.ITER_NEXT),
            code.make(Op.JUMP_IF_FALSY, 39),
            code.make(Op.POP),
            code.make(Op.SET_LOCAL, 0),
            code.make(Op.ENTER_FRAME, 1),
            code.make(Op.GET_LOCAL, 1, 0),
            code.make(Op.ITER_INIT),
            code.make(Op.ITER_NEXT),
            code.make(Op.JUMP_IF_FALSY, 33),
            code.make(Op.POP),
            code.make(Op.SET_LOCAL, 0),
            code.make(Op.GET_LOCAL, 0, 0),
            code.make(Op.RENDER),
            code.make(Op.JUMP, 19),
            code.make(Op.POP),
            code.make(Op.POP),
            code.make(Op.LEAVE_FRAME),
            code.make(Op.JUMP, 6),
            code.make(Op.POP),
            code.make(Op.POP),
            code.make(Op.LEAVE_FRAME),
        ]
    )

    assert bytecode.constants == ["y"]


def test_loop_else() -> None:
    bytecode = _bytecode("{% for x in y %}{{ x }}{% else %}foo{% endfor %}")

    assert disassemble(bytecode.instructions) == disassemble(
        [
            code.make(Op.ENTER_FRAME, 2),
            code.make(Op.FALSE),
            code.make(Op.SET_LOCAL, 1),
            code.make(Op.GLOBAL, 0),
            code.make(Op.ITER_INIT),
            code.make(Op.ITER_NEXT),
            code.make(Op.JUMP_IF_FALSY, 26),
            code.make(Op.POP),
            code.make(Op.TRUE),
            code.make(Op.SET_LOCAL, 1),
            code.make(Op.SET_LOCAL, 0),
            code.make(Op.GET_LOCAL, 0, 0),
            code.make(Op.RENDER),
            code.make(Op.JUMP, 9),
            code.make(Op.POP),
            code.make(Op.POP),
            code.make(Op.GET_LOCAL, 0, 1),
            code.make(Op.JUMP_IF_TRUTHY, 38),
            code.make(Op.POP),
            code.make(Op.TEXT, 1),
            code.make(Op.POP),
            code.make(Op.LEAVE_FRAME),
        ]
    )

    assert bytecode.constants == ["y", "foo"]
