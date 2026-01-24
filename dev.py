import nano_template as nt
from nano_template import _bytecode


import tests._bytecode as code

source = "Hello, {{ you }}!"
bytecode = _bytecode(source)
# print(bytecode.instructions)
# print(bytecode.constants)

print(code.to_str(bytecode.instructions))

template = nt.compile(source)
print(template.render({"you": "Sue"}))
