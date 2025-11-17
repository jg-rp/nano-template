from micro_liquid import tokenize

text = "Hello {{ 'you' }}!"
tokens = tokenize(text)

for token in tokens:
    print(repr(token))

# TODO: rename to _tokenize
# TODO: rename project to micro-t or micro_t
# TODO: rewrite java-style comments
# TODO: use dealloc instead of destroy
