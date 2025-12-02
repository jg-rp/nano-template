from nano_template import StrictUndefined
from nano_template import parse
from nano_template import render

t = parse("{{ foo.nosuchthing }}", undefined=StrictUndefined)
print(t.render({"foo": {}}))

# print(render(source, data))

# TODO: setup ruff
