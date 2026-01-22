from nano_template import _bytecode

from tests._bytecode import Op
import tests._bytecode as code

source = "Hello, World"
bytecode = _bytecode(source)
print(bytecode.instructions)
print(bytecode.constants)

print(code.to_str(bytecode.instructions))


ins = [
    code.make(Op.TEXT, 0),
]

print(ins)
print(repr(b"".join(ins)))
