from micro_liquid import render


source = "{% for x in y %}{{ x }}, {% endfor %}"
data = {"y": [1, 2, 3]}


print(render(source, data))


# TODO: rename to _tokenize
# TODO: rename project to micro-t or micro_t or nano-t
# TODO: rewrite java-style comments
# TODO: use int instead of Py_ssize_t for return codes
# TODO: be consistent with integer return codes. -1 and 0
