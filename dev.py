from micro_liquid import render


source = "{{ not x or y  }}"
data = {"x": False, "y": True}

print(render(source, data))


# TODO: rename to _tokenize
# TODO: rename project to micro-t or micro_t or nano-t
# TODO: rewrite java-style comments
# TODO: use dealloc instead of destroy
# TODO: use int instead of Py_ssize_t for return codes
# TODO: rename object_list to not mention list (Maybe "ObjectBuffer")
# TODO: be consistent with integer return codes. -1 and 0
