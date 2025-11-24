import json

from minijinja import render_str
from micro_liquid import render


with open("tests/fixtures/001/index.template") as fd:
    source = fd.read()

with open("tests/fixtures/001/data.json") as fd:
    data = json.load(fd)


assert render(source, data) == render_str(source, name=None, **data)


# print(render(source, data))


# TODO: rename to _tokenize
# TODO: rename project to micro-t or micro_t or nano-t
# TODO: rewrite java-style comments
# TODO: use int instead of Py_ssize_t for return codes
# TODO: be consistent with integer return codes. -1 and 0
