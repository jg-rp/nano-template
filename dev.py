import json
from nano_template import render
from nano_template import StrictUndefined


# with open("tests/fixtures/001/template.txt") as fd:
#     source = fd.read()

source = "Hello {{fooo}}!"

with open("tests/fixtures/001/data.json") as fd:
    data = json.load(fd)

print(render(source, data, undefined=StrictUndefined))


# print(render(source, data))

# TODO: don't use `self`
# TODO: setup ruff
# TODO: replace uv with hatch?

# TODO: compare valgrind output before and after arena

# ==15356== LEAK SUMMARY:
# ==15356==    definitely lost: 0 bytes in 0 blocks
# ==15356==    indirectly lost: 0 bytes in 0 blocks
# ==15356==      possibly lost: 395,096 bytes in 4 blocks
# ==15356==    still reachable: 2,979 bytes in 7 blocks
# ==15356==         suppressed: 0 bytes in 0 blocks
# ==15356==
# ==15356== ERROR SUMMARY: 4 errors from 4 contexts (suppressed: 0 from 0)
