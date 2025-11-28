import json
from micro_liquid import tokenize
from micro_liquid import render


# with open("tests/fixtures/001/index.template") as fd:
#     source = fd.read()

source = "Hello {% if true %}{{ you }}{% endif %}!"

# with open("tests/fixtures/001/data.json") as fd:
#     data = json.load(fd)

data = {"you": "WORLD", "true": True}


for token in tokenize(source):
    print(token)

print(render(source, data))


# print(render(source, data))


# TODO: rename to _tokenize
# TODO: rename project to micro-t or micro_t or nano-t
# TODO: rewrite java-style comments
# TODO: use int instead of Py_ssize_t for return codes
# TODO: be consistent with integer return codes. -1 and 0


# TODO: model node children as a paged array
# TODO: move ML_Node_new to ML_Parser_make_node
# TODO: `ML_Node_add_child` becomes `ML_Node_add_child`
# TODO: move ML_Expr_new to `ML_Parser_make_expr`
# TODO: change ML_Expr to have `left` and `right`, not `children`
# TODO: change ML_Expr.objects to be a paged array


# TODO: compare valgrind output before and after arena

# ==15356== LEAK SUMMARY:
# ==15356==    definitely lost: 0 bytes in 0 blocks
# ==15356==    indirectly lost: 0 bytes in 0 blocks
# ==15356==      possibly lost: 395,096 bytes in 4 blocks
# ==15356==    still reachable: 2,979 bytes in 7 blocks
# ==15356==         suppressed: 0 bytes in 0 blocks
# ==15356==
# ==15356== ERROR SUMMARY: 4 errors from 4 contexts (suppressed: 0 from 0)
