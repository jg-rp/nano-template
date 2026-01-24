from typing import Any

import nano_template as nt
from nano_template import _bytecode


import tests._bytecode as code

source = "{% for x in y %}{{ x }}{% else %}foo{% endfor %}"
data: dict[str, Any] = {"y": 42}

bytecode = _bytecode(source)
# print(bytecode.instructions)
# print(bytecode.constants)

print(code.to_str(bytecode.instructions))

template = nt.compile(source)
print(template.render(data))
