from typing import Any

from micro_liquid import StrictUndefined
from micro_liquid import render


source = "Hello {{ no.such.thing  }}!"
data: dict[str, Any] = {"x": False, "y": True, "no": {"foo": 42}}

print(render(source, data, undefined=StrictUndefined))


# TODO: rename to _tokenize
# TODO: rename project to micro-t or micro_t or nano-t
# TODO: rewrite java-style comments
# TODO: use dealloc instead of destroy
# TODO: use int instead of Py_ssize_t for return codes
# TODO: rename object_list to not mention list (Maybe "ObjectBuffer")
# TODO: be consistent with integer return codes. -1 and 0
# TODO: benchmark after trim and unescape - before any more refactoring.
