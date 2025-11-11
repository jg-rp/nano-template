from micro_liquid import TokenKind
from micro_liquid import tokenize

source = "Hello, {{- you ~}}!"
tokens = tokenize(source)

for token in tokens:
    print(repr(token))

# TODO: rename to _tokenize
