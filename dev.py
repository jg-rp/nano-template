from nano_template import _bytecode

from tests._bytecode import Op
import tests._bytecode as code

source = "{% if a %}b{% endif %}"
bytecode = _bytecode(source)
print(bytecode.instructions)
print(bytecode.constants)

print(code.to_str(bytecode.instructions))
