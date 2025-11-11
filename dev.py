from micro_liquid import tokenize

text = "Hello {{ 'you' }}!"
tokens = tokenize(text)

for token in tokens:
    print(repr(token))

# TODO: rename to _tokenize
