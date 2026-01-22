from nano_template import _bytecode

from tests._bytecode import Op
import tests._bytecode as code

source = "{{ a.b }}"
bytecode = _bytecode(source)
print(bytecode.instructions)
print(bytecode.constants)

print(code.to_str(bytecode.instructions))

expected_instructions = [
    code.make(Op.GLOBAL, 0),
    code.make(Op.SELECTOR, 1),
    code.make(Op.RENDER),
]
print("\n")
print(code.to_str(b"".join(expected_instructions)))
