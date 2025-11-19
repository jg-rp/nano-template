import json
from micro_liquid import parse
from micro_liquid import tokenize
from micro_liquid import Undefined


def serialize(obj: object) -> str:
    return json.dumps(obj) if isinstance(obj, (list, dict, tuple)) else str(obj)


text = "Hello {{ you }}!"
tokens = tokenize(text)

for token in tokens:
    print(repr(token))

template = parse(text)
print(template)
print(template.render({"you": list("World")}, serialize, Undefined))

# TODO: rename to _tokenize
# TODO: rename project to micro-t or micro_t
# TODO: rewrite java-style comments
# TODO: use dealloc instead of destroy
# TODO: use int instead of Py_ssize_t for return codes
# TODO: rename object_list to not mention list (Maybe "ObjectBuffer")
# TODO: be consistent with integer return codes. -1 and 0
