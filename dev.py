from nano_template import StrictUndefined
from nano_template import parse
from nano_template import render

t = parse("{{ foo.nosuchthing }}", undefined=StrictUndefined)
print(t.render({"foo": {}}))

# print(render(source, data))

# TODO: don't use `self` unless object
# TODO: setup ruff

# TODO: compare valgrind output before and after arena

# ==15356== LEAK SUMMARY:
# ==15356==    definitely lost: 0 bytes in 0 blocks
# ==15356==    indirectly lost: 0 bytes in 0 blocks
# ==15356==      possibly lost: 395,096 bytes in 4 blocks
# ==15356==    still reachable: 2,979 bytes in 7 blocks
# ==15356==         suppressed: 0 bytes in 0 blocks
# ==15356==
# ==15356== ERROR SUMMARY: 4 errors from 4 contexts (suppressed: 0 from 0)
