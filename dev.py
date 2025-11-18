from micro_liquid import parse
from micro_liquid import tokenize
from micro_liquid import Undefined

text = "Hello {{ 'you' }}!"
tokens = tokenize(text)

for token in tokens:
    print(repr(token))

template = parse(text)
print(template)
print(template.render({}, lambda x: str(x), Undefined))

# TODO: rename to _tokenize
# TODO: rename project to micro-t or micro_t
# TODO: rewrite java-style comments
# TODO: use dealloc instead of destroy
