import json

from micro_liquid import StrictUndefined
from micro_liquid import render
from micro_liquid._native import render as native_render


with open("tests/fixtures/001/index.template", encoding="utf8") as fd:
    source = fd.read()

with open("tests/fixtures/001/data.json", encoding="utf8") as fd:
    data = json.load(fd)

# source = "Hello {{ no.such.thing  }}!"
# data: dict[str, Any] = {"x": False, "y": True, "no": {"foo": 42}}

# print(render(source, data))

assert render(source, data) == native_render(source, data)


# TODO: rename to _tokenize
# TODO: rename project to micro-t or micro_t or nano-t
# TODO: rewrite java-style comments
# TODO: use dealloc instead of destroy
# TODO: use int instead of Py_ssize_t for return codes
# TODO: rename object_list to not mention list (Maybe "ObjectBuffer")
# TODO: be consistent with integer return codes. -1 and 0
# TODO: benchmark after trim and unescape - before any more refactoring.
