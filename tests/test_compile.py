from nano_template import _bytecode

from . import _bytecode as code
from ._bytecode import Op


def test_just_text() -> None:
    source = "Hello, World!"

    expected_instructions = [code.make(Op.TEXT, 0)]
    expected_constants = ["Hello, World!"]

    bytecode = _bytecode(source)

    assert code.to_str(b"".join(expected_instructions)) == code.to_str(
        bytecode.instructions
    )

    assert expected_constants == bytecode.constants


def test_empty_template() -> None:
    source = ""

    expected_instructions: list[bytes] = []
    expected_constants = []

    bytecode = _bytecode(source)

    assert code.to_str(b"".join(expected_instructions)) == code.to_str(
        bytecode.instructions
    )

    assert expected_constants == bytecode.constants


def test_just_output() -> None:
    source = "{{ a }}"

    expected_instructions = [code.make(Op.GLOBAL, 0), code.make(Op.RENDER)]
    expected_constants = ["a"]

    bytecode = _bytecode(source)

    assert code.to_str(b"".join(expected_instructions)) == code.to_str(
        bytecode.instructions
    )

    assert expected_constants == bytecode.constants
