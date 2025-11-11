from micro_liquid import tokenize

source = "Hello, {{- you ~}}!"
tokens = tokenize(source)

for token in tokens:
    print(repr(token))
