from nano_template import _bytecode

source = "Hello, {{ you }}!"
bytecode = _bytecode(source)
print(bytecode.instructions)
print(bytecode.constants)
