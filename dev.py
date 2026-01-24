import json

import nano_template as nt
from nano_template import render

with open("tests/fixtures/001/template.txt") as fd:
    source = fd.read()

with open("tests/fixtures/001/data.json") as fd:
    data = json.load(fd)

template = nt.compile(source)
result = template.render(data)

assert result == render(source, data)
